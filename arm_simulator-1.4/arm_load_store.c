/*
Armator - simulateur de jeu d'instruction ARMv5T à but pédagogique
Copyright (C) 2011 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique Générale GNU publiée par la Free Software
Foundation (version 2 ou bien toute autre version ultérieure choisie par vous).

Ce programme est distribué car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but spécifique. Reportez-vous à la
Licence Publique Générale GNU pour plus de détails.

Vous devez avoir reçu une copie de la Licence Publique Générale GNU en même
temps que ce programme ; si ce n'est pas le cas, écrivez à la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
États-Unis.

Contact: Guillaume.Huard@imag.fr
	 Bâtiment IMAG
	 700 avenue centrale, domaine universitaire
	 38401 Saint Martin d'Hères
*/
#include <assert.h>
#include "arm_load_store.h"
#include "arm_exception.h"
#include "arm_constants.h"
#include "util.h"
#include "debug.h"


/* Instructions implémentées :
avec arm_load_store :
	 word/byte : LDR, STR, LDRB, STRB
	miscellaneous : LDRH, STRH, LDRD, STRD
avec arm_load_store_multiple :
	LDM(1), STM(1)
*/

/* Retourne TRUE si l’état des flags N, Z, C et V 
remplit la condition encodée dans l’argument cond,
et retourne FALSE dans tous les autres cas */
uint8_t condition_passed(arm_core proc, uint32_t ins) {
	uint8_t cond,cpsr,z,n,c,v;
    
	cpsr = arm_read_cpsr(proc); 
	cond = get_bits(ins,31,28); // Les bits des FLAGS ZNCV

	z = get_bit(cpsr,30);
	n = get_bit(cpsr,31);
	c = get_bit(cpsr,29);
	v = get_bit(cpsr,28);

	switch(cond) {
		case 0:  return z; 				// EQ       Equal
		case 1:  return !z; 			// NE       Not equal
		case 2:  return c;				// CS / HS  Carry set / Higher or same
		case 3:  return !c;				// CC / LO  Carry clear / Lower
		case 4:  return n;				// MI       Minus / Negative
		case 5:  return !n;				// PL       Plus / Positive or Zero
		case 6:  return v;				// VS       Overflow
		case 7:  return !v;				// VC       !Overflow
		case 8:  return c & !z;			// HI       Higher
		case 9:  return !c || z;		// LS       Lower or same
		case 10: return n == v;			// GE       Greater than or equal
		case 11: return n != v;			// LT       Less than
		case 12: return !z && (n == v);	// GT       Greater than
		case 13: return z || (n != v);	// LE       Less than or Equal
		case 14: return 1;		        // AL       Always
		case 15: return 0;				// ?
		default: return 1;
	}
}

/*Effectue une opération en fonction du flag u */
uint32_t op(uint32_t ins, uint32_t left_op, uint32_t right_op) {
    return left_op + right_op * (get_bit(ins,23)? 1 : -1);
}

/* Permet de connaître le nombre de bits a 1 sur un nombre passé en argument*/
uint8_t nb_set_bits(uint16_t nb) {
	uint8_t count = 0;
	while(nb) {
	    count += nb % 2;
	    nb >>= 1;
	}
	return count;
}
/*Sert a trouver l'offset pour les instructions miscellaneous de addrmode_load_store - */
uint32_t set_offset(uint32_t ins) {
	return (get_bits(ins, 11, 8) << 4) | get_bits(ins, 3, 0);
}

/*Sert a trouver l'index pour les instructions scaled de addrmode_load_store - Voir doc A5.2.4 */
uint32_t set_index(arm_core proc, uint32_t ins, uint8_t rm) {
	uint8_t shift = get_bits(ins, 6, 5);
	uint8_t shift_imm = get_bits(ins, 11, 7);

	switch(shift) {
		case 0b00: // LSL
            return rm << shift_imm;
		case 0b01: // LSR
            return shift_imm? rm >> shift_imm : 0;
		case 0b10: // ASR
            return shift_imm? asr(rm, shift_imm) : get_bit(rm, 31)? 0xFFFFFFFF : 0;
		case 0b11:
            //                ROR                : RRX C -> Carry Flag
            return shift_imm? ror(rm, shift_imm) : (get_bit(arm_read_cpsr(proc), C) << 31) | (rm >> 1);
		default:
			return 0;
	}
}
/* Addressing mode multiple - Voir Documentation A5.2 */
void addrmode_load_store(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t add_word) {
	uint32_t cond = get_bits(ins, 31, 28);

    if(p(ins)) {
        *address = op(ins, *rn, add_word);
    }

    if(p(ins) == w(ins)) {
        *address = p(ins)? op(ins, *rn, add_word) : *rn;
        if(condition_passed(proc, cond)) {
            *rn = p(ins)? *address : op(ins, *rn, add_word);
        }
    }
}
/* Addressing mode multiple - Voir Documentation A5.4 */
void addrmode_load_store_multiple(arm_core proc, uint32_t ins, uint32_t *start_address,uint32_t *end_address, uint32_t *rn) {
   	uint8_t u = get_bit(ins, 23);
    uint8_t nbSetBits = nb_set_bits(get_bits(ins, 15, 0)) * 4;

    // {LDM|STM} IB : {LDM|STM} DB : {LDM|STM} IA : {LDM|STM} DA
    *start_address = *rn - (p(ins) ? (u ? -4 : nbSetBits) : (u ? 0: nbSetBits - 4));
    *end_address   = *rn + (p(ins) ? (u ? nbSetBits : -4) : (u ? nbSetBits - 4 : 0));

    if(condition_passed(proc, get_bits(ins, 31, 28)) && w(ins)) {
        *rn += nbSetBits * (u? 1 : -1);
    }
}
/* Addressing mode miscellaneous - Voir Documentation A5.3 */
void addrmode_load_store_miscellaneous(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t offset) {
    uint8_t b = get_bit(ins, 22);
    uint32_t rm = arm_read_register(proc, get_bits(ins, 3, 0));

   if(p(ins)) { // Immediate offset or Register offset or Immediate pre-indexed
       *address = op(ins, *rn, !b? rm : offset); // Register : Immediate
       if(w(ins) && condition_passed(proc, ins)) { // pre-indexed
           *rn = *address;
       }
   } else { // Immediate post-indexed or Register post-indexed
       *address = *rn;
       if(condition_passed(proc, ins)) {
           *rn = op(ins, *rn, b? offset : rm); // Immediate : Register
       }
   }
}

/* Voir doc A3.11.5 */
uint8_t executeInstr_miscellaneous(arm_core proc, uint32_t ins, uint32_t address) {
	uint32_t word;
	//uint8_t byte;
	uint16_t half;
	uint8_t rd = get_bits(ins, 15, 12); // Numéro du registre qui sera chargé ou changé
	uint8_t h = get_bit(ins, 5);
    uint8_t s = get_bit(ins, 6);
	uint8_t l = get_bit(ins, 20);
	uint8_t res = 0;
	
	switch ((l<<2)+(s<<1)+h) {
        case 1 : // STRH -  Voir doc A4-204
			if(condition_passed(proc, ins)) {
				half = (arm_read_register(proc, rd) & 0xFFFF);
    			return arm_write_half(proc, address, half);
			}
			return 0; 
        case 2 : // LDRD -  Voir doc A4-50
			if (!(rd % 2) || (rd == LR) || !(get_bits(address,1,0) )) {
       			return 0; // Unpredictable
    		}
   			res = arm_read_word(proc, address, &word);
   			if (!res) {
                arm_write_register(proc, rd, word);
    		    res = arm_read_word(proc, address+4, &word);
            }
   			return res? 0 : arm_write_register(proc, rd+1, word);
        case 3 : // STRD -  Voir doc A4-199
		 	if (!(rd % 2) || (rd == LR) || !(get_bits(address,1,0)) || !(get_bit(address,2))) {
       			return 0; // Unpredictable
   			}
			word = arm_read_register(proc, rd);
    		res = arm_write_word(proc, address, word);
    		if (!res) {
    			word = arm_read_register(proc, rd+1);
    			res = arm_write_word(proc, address+4, word);
    		}
    		return res;
        case 5 : //LDRH -  Voir doc A4-54
			res = arm_read_half(proc, address, &half);
    		if (!res) arm_write_register(proc, rd, (uint32_t)half);
    		return res;
        case 6 : // LDRSB
			return UNDEFINED_INSTRUCTION;  
        case 7 : // LDRSH
			return UNDEFINED_INSTRUCTION; 
        default: 
			return UNDEFINED_INSTRUCTION; 
    }
}
/* Voir doc A3.11.5 */
uint8_t executeInstr_word_byte(arm_core proc, uint32_t ins, uint32_t address) {
	uint8_t byte;
 	uint32_t word;
	uint8_t rd = get_bits(ins, 15, 12); // Numéro du registre qui sera chargé ou changé
	uint8_t l = get_bit(ins, 20); // Dit si on est en load ou en store
    uint8_t b = get_bit(ins, 22); // Dit si on veut un acces avec un word ou un byte

	if(l) {
 		if(b) { // LDRB - Voir doc A4-46 
		 	uint8_t res = arm_read_byte(proc, address, &byte);
			if (res == -1)
 				arm_write_register(proc, rd, (uint32_t) byte);
			return res;
 		}
 		else { // LDR -  Voir doc A4-43
 			if(arm_read_word(proc, address, &word) == 0){
				if(rd == 15){
					uint8_t temp = get_bit(word, 0);
					uint32_t cpsr = arm_read_cpsr(proc);
					if (temp != -1) {
        				cpsr = (temp == 1) ? set_bit(cpsr, 5) : clr_bit(cpsr, 5);
    					arm_write_cpsr(proc, cpsr);
					}
					word &= 0xFFFFFFFE;
				}
			}
 			return arm_write_register(proc, rd, word);
 		}
 	}
 	else {
 		if(b) { // STRB -  Voir doc A4-195 
		 	if(condition_passed(proc, ins)) {
 				arm_write_byte(proc, address, arm_read_register(proc, rd) & 0x00FF);
		 	}
 		}
 		else { // STR -  Voir doc A4-193
			if(condition_passed(proc, ins)) {
         	 	return arm_write_word(proc, address, arm_read_register(proc, rd));
       		}
 		}
 	}
	return 0;
}

uint8_t executeInstr_multiple(arm_core proc, uint32_t ins, uint32_t start_address, uint32_t end_address) {
	uint8_t i;
	uint8_t pc_nb = 15;
	uint32_t registers = get_bits(ins, pc_nb, 0);
	uint8_t l = get_bit(ins, 20);
    uint32_t address;
	uint8_t res = 0;

	if(l) { // LDM -  Voir doc A4-36
		uint32_t word;
		uint8_t pc = get_bit(registers, pc_nb);
		address = start_address;
		for(i = 0; i <= 14; i++) {
			if(!res){
				if(get_bit(registers, i)) {
					res = arm_read_word(proc, address, &word);
					if(!res) arm_write_register(proc, i, word);
			    	address += 4;
				}
			}		
		}
		if(pc && !res) {
			res = arm_read_word(proc, address, &word);
			uint8_t temp = get_bit(word, 0);
			uint32_t cpsr = arm_read_cpsr(proc);
			if (temp != -1) {
        		cpsr = (temp == 1) ? set_bit(cpsr, 5) : clr_bit(cpsr, 5);
    			arm_write_cpsr(proc, cpsr);
			}
			arm_write_register(proc, pc_nb, word & 0xFFFFFFFE);
			address += 4;
		}
		printf("%x",end_address);
		assert(end_address == (address - 4));

	}
	else { // STM -  Voir doc A4-189
		address = start_address;
		for(i = 0; i <= 15; i++) {
			if(!res){
				if(get_bit(registers, i)) {
					res = arm_write_word(proc, address, arm_read_register(proc, i));
					address += 4;
				}
			}
		}
		assert(end_address == (address - 4));
	}

    return res;
}



// Fonction main pour les instructions load et store :  word / byte et miscellaneous
int arm_load_store(arm_core proc, uint32_t ins) {
	uint8_t id;
	uint32_t offset, rn, address, rm, index;

	rn = arm_read_register(proc, get_bits(ins, 19, 16)); // Numéro du registre qui officie de premier opérande
	rm = arm_read_register(proc, get_bits(ins, 3, 0)); // Spécifie le numéro du registre qui contient l'offset à appliquer à rn

    offset = get_bits(ins, 11, 0);
	id = get_bits(ins, 27, 25);

    int a;

    switch(id) {
		case 0:  // Part 1 : Miscellaneous
			offset = set_offset(ins);
			addrmode_load_store_miscellaneous(proc, ins, &address, &rn, rm);
			return executeInstr_miscellaneous(proc, ins, address);
		case 2:   // Part 2 : Immidiate
			addrmode_load_store(proc, ins, &address, &rn, offset);
			return executeInstr_word_byte(proc, ins, address);
		case 3: 
            a = !get_bits(ins, 11, 4);
            if(a || (!get_bit(ins, 4) && (p(ins) || !w(ins)))) { // Part 3 : Register
                addrmode_load_store(proc, ins, &address, &rn, a? rm : (index = set_index(proc, ins, rm)));
				return executeInstr_word_byte(proc, ins, address);
            }
		    return 0;

		default:
			return UNDEFINED_INSTRUCTION;
	}
}


// Fonction main pour les instructions load et store multiple
int arm_load_store_multiple(arm_core proc, uint32_t ins) {
	uint32_t rn, start_address, end_address;
    rn = arm_read_register(proc, get_bits(ins, 19, 16));
	addrmode_load_store_multiple(proc, ins, &start_address, &end_address, &rn);
    
    return executeInstr_multiple(proc, ins, start_address, end_address);
}

int arm_coprocessor_load_store(arm_core proc, uint32_t ins) {
    /* Not implemented */
    return UNDEFINED_INSTRUCTION;
}

