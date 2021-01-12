/*
Armator - simulateur de jeu d'instruction ARMv5T � but p�dagogique
Copyright (C) 2011 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique G�n�rale GNU publi�e par la Free Software
Foundation (version 2 ou bien toute autre version ult�rieure choisie par vous).

Ce programme est distribu� car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but sp�cifique. Reportez-vous � la
Licence Publique G�n�rale GNU pour plus de d�tails.

Vous devez avoir re�u une copie de la Licence Publique G�n�rale GNU en m�me
temps que ce programme ; si ce n'est pas le cas, �crivez � la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
�tats-Unis.

Contact: Guillaume.Huard@imag.fr
	 B�timent IMAG
	 700 avenue centrale, domaine universitaire
	 38401 Saint Martin d'H�res
*/
#include <assert.h>
#include "arm_load_store.h"
#include "arm_exception.h"
#include "arm_constants.h"
#include "util.h"
#include "debug.h"




uint8_t condition_passed(arm_core proc, uint32_t ins) {
	uint8_t cond,cpsr,z,n,c,v;
    
	cpsr = arm_read_cpsr(proc);
	cond = get_bits(ins,31,28);

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

uint32_t op(uint32_t ins, uint32_t left_op, uint32_t right_op) {
    return left_op + right_op * (get_bit(ins,23)? 1 : -1);
}

uint8_t nb_set_bits(uint16_t nb) {
	uint8_t count = 0;
	while(nb) {
	    count += nb % 2;
	    nb >>= 1;
	}
	return count;
}

uint32_t set_offset(uint32_t ins) {
	return (get_bits(ins, 11, 8) << 4) | get_bits(ins, 3, 0);
}

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

void load_store(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t add_word) {
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

void load_store_multiple(arm_core proc, uint32_t ins, uint32_t *start_address,uint32_t *end_address, uint32_t *rn) {
   	uint8_t u = get_bit(ins, 23);
    uint8_t nbSetBits = nb_set_bits(get_bits(ins, 15, 0)) * 4;

    // {LDM|STM} IB : {LDM|STM} DB : {LDM|STM} IA : {LDM|STM} DA
    *start_address = *rn - (p(ins) ? (u ? -4 : nbSetBits) : (u ? 0: nbSetBits - 4));
    *end_address   = *rn + (p(ins) ? (u ? nbSetBits : -4) : (u ? nbSetBits - 4 : 0));

    if(condition_passed(proc, get_bits(ins, 31, 28)) && w(ins)) {
        *rn += nbSetBits * (u? 1 : -1);
    }
}

void load_store_miscellaneous(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t offset) {
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
uint8_t executeInstr_miscellaneous(arm_core proc, uint32_t ins, uint32_t address) {
	uint32_t word;
	uint8_t byte;
	uint16_t half;
	uint8_t rd = get_bits(ins, 15, 12); // Numéro du registre qui sera chargé ou changé
	uint8_t h = get_bit(ins, 5);
    uint8_t s = get_bit(ins, 6);
	uint8_t l = get_bit(ins, 20);
	uint8_t res = 0;
	
	switch (l<<2+s<<1+h) {
        case 1 : // STRH
			if(condition_passed(proc, ins)) {
				half = (arm_read_register(p, rd) & 0xFFFF);
    			return arm_write_half(p, address, half);
			}
			return 0; 
		break;
        case 2 : // LDRD
			if (!(rd % 2) || (rd == LR) || !(get_bits(address,1,0) )) {
       			return 0; // Unpredictable
    		}
   			res = arm_read_word(p, address, &word);
   			if (!res) arm_write_register(p, rd, word);
    		if (!res) res = arm_read_word(p, address+4, &word);
   			if (!res) return arm_write_register(p, rd+1, word);
		break;
        case 3 : // STRD
		 	if (!(rd % 2) || (rd == LR) || !(get_bits(address,1,0)) || !(get_bit(address,2))) {
       			return 0; // Unpredictable
   			}
			word = arm_read_register(p, rd);
    		res = arm_write_word(p, address, word);
    		if (!res) {
    			word = arm_read_register(p, rd+1);
    			res = arm_write_word(p, address+4, word);
    		}
    		return res;
		break;
        case 5 : //LDRH
			res = arm_read_half(p, address, &half);
    		if (!res)
     		   arm_write_register(p, rd, (uint32_t)half);
    		return res;
		break;
        case 6 : // LDRSB
			return UNDEFINED_INSTRUCTION;  
		break;
        case 7 :  // LDRSH
			return UNDEFINED_INSTRUCTION; 
		break;
        default: 
			return UNDEFINED_INSTRUCTION; 
		break; 
    }
}

uint8_t executeInstr_word_byte(arm_core proc, uint32_t ins, uint32_t address) {
	uint8_t byte;
 	uint32_t word;
	uint8_t rd = get_bits(ins, 15, 12); // Numéro du registre qui sera chargé ou changé
	uint8_t l = get_bit(ins, 20); // Dit si on est en load ou en store
    uint8_t b = get_bit(ins, 22); // Dit si on veut un acces avec un word ou un byte

	if(l) {
 		if(b) { // LDRB
		 	uint8_t res = arm_read_byte(proc, address, &byte);
			if (res == -1)
 				arm_write_register(proc, rd, (uint32_t) byte);
			return res;
 		}
 		else { // LDR
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
 		if(b) { // STRB
		 	if(condition_passed(proc, ins)) {
 				arm_write_byte(proc, address, arm_read_register(proc, rd) & 0x00FF);
		 	}
 		}
 		else { // STR
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

	if(l) { // LDM
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
		assert(end_address == (address - 4));

	}
	else { // STM
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




int arm_load_store(arm_core proc, uint32_t ins) {
	uint8_t id;
	uint32_t offset, rn, address, rm, index;

	rn = arm_read_register(proc, get_bits(ins, 19, 16)); // Numéro du registre qui officie de premier opérande
	rm = arm_read_register(proc, get_bits(ins, 3, 0)); // Spécifie le numéro du registre qui contient l'offset à appliquer à rn

    offset = get_bits(ins, 11, 0);
	id = get_bits(ins, 27, 25);

    int a;

    switch(id) {
		case 0:  // Part 0 : Miscellaneous
			offset = set_offset(ins);
			load_store_miscellaneous(proc, ins, &address, &rn, rm);
			return executeInstr_miscellaneous(proc, ins, address);
		break;
		case 2:   // Part 1 : Immidiate
			load_store(proc, ins, &address, &rn, offset);
			return executeInstr_word_byte(proc, ins, address);
		break;
		case 3:
            a = !get_bits(ins, 11, 4);
            if(a || (!get_bit(ins, 4) && (p(ins) || !w(ins)))) {
                load_store(proc, ins, &address, &rn, a? rm : (index = set_index(proc, ins, rm)));
				return executeInstr_word_byte(proc, ins, address);
            }
		break;

		default:
			return UNDEFINED_INSTRUCTION;
	}
}



int arm_load_store_multiple(arm_core proc, uint32_t ins) {
	uint32_t rn, start_address, end_address;
    rn = arm_read_register(proc, get_bits(ins, 19, 16));
	load_store_multiple(proc, ins, &start_address, &end_address, &rn);

    return executeInstr_multiple(proc, ins, start_address, end_address);
}

int arm_coprocessor_load_store(arm_core proc, uint32_t ins) {
    /* Not implemented */
    return UNDEFINED_INSTRUCTION;
}

