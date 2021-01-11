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
		case 14: return 1; break;		// AL       Always
		case 15: return 0;				// ?
		default: return 1;
	}
}

uint32_t op(uint32_t ins, uint32_t left_op, uint32_t right_op) {
	uint8_t u = get_bit(ins,23);
	if(u) {
        return left_op + right_op;
    }
    else {
        return left_op - right_op;
    }
}

uint8_t nb_set_bits(uint16_t nb) {
	uint8_t count = 0;
	while(nb) {
	    count = count + nb % 2;
	    nb = nb >> 1;
	}
	return count;
}

uint32_t set_offset(uint32_t ins) {
	uint32_t immedL = get_bits(ins, 3, 0);
	uint32_t immedH = get_bits(ins, 11, 8);
	return (immedH << 4) | immedL;
}

uint32_t set_index(arm_core proc, uint32_t ins, uint8_t rm) {
	uint8_t shift = get_bits(ins, 6, 5);
	uint8_t shift_imm = get_bits(ins, 11, 7);

	switch(shift) {
		case 0b00 : // LSL
            return rm << shift_imm;
        break;
		case 0b01: // LSR
			if(shift_imm) {
               return rm >> shift_imm;
            }
			else {
                return 0;
            }
		break;
		case 0b10: // ASR
			if(shift_imm) {
                return asr(rm, shift_imm);
			}
			else {
                if(get_bit(rm, 31)) {
                    return 0xFFFFFFFF;
                }
				else {
                	return 0;
           		}
            }
		break;
		case 0b11:
			if(shift_imm) { // ROR
				return ror(rm, shift_imm);
            }
			else { // RRX
                return (get_bit(arm_read_cpsr(proc), C) << 31) | (rm >> 1); // C -> Carry Flag
            }
		break;
		default:
			return -1;
		break;
	}
}

void load_store(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t add_value) {
	uint32_t cond = get_bits(ins, 31, 28);
	//uint8_t u = get_bit(ins, 23);
	if(!w(ins) && p(ins)) { //Offset
		*address = op(ins, *rn, add_value);
    }
    else if(p(ins) && w(ins)) { //Pre-indexed
		*address = op(ins, *rn, add_value);
		if(condition_passed(proc, cond)) {
			*rn = *address;
		}
	}
	else if(!p(ins) && !w(ins)) { //Post-indexed
		*address = *rn;
		if(condition_passed(proc, cond)) {
		*rn = op(ins, *rn, add_value);
		}
	}
}

void load_store_multiple(arm_core proc, uint32_t ins, uint32_t *start_address,uint32_t *end_address, uint32_t *rn) {
	// uint32_t cpsr = arm_read_cpsr(proc);
	uint16_t registers = get_bits(ins, 15, 0);
	uint8_t cond = get_bits(ins, 31, 28);
   	uint8_t u = get_bit(ins, 23);

	if(!p(ins) && !u) { // {LDM|STM} DA
		*start_address = *rn - (nb_set_bits(registers) * 4) + 4;
		*end_address = *rn;
		if(condition_passed(proc, cond) && w(ins))
			*rn = *rn - (nb_set_bits(registers) * 4);
	}

	else if(!p(ins) && u) {// {LDM|STM} IA
		*start_address = *rn;
		*end_address = *rn + (nb_set_bits(registers) * 4) - 4;
		if(condition_passed(proc, cond) && w(ins))
			*rn = *rn + (nb_set_bits(registers) * 4);
	}

	else if(p(ins) && !u) { // {LDM|STM} DB
		*start_address = *rn - (nb_set_bits(registers) * 4);
		*end_address = *rn - 4;
		if(condition_passed(proc, cond) && w(ins))
			*rn = *rn - (nb_set_bits(registers) * 4);
	}

	else if(p(ins) && u) { // {LDM|STM} IB
		*start_address = *rn + 4;
		*end_address = *rn + (nb_set_bits(registers) * 4);
		if(condition_passed(proc, cond) && w(ins))
			*rn = *rn + (nb_set_bits(registers) * 4);
	}
}

void load_store_miscellaneous(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t offset) {
    uint8_t b = get_bit(ins, 22);
	// uint8_t u = get_bit(ins, 23);
    uint32_t rm = arm_read_register(proc, get_bits(ins, 3, 0));

	if(p(ins) && b && !w(ins)) { // Immediate offset
		*address = op(ins, *rn, offset);
	}
	else if(p(ins) && !b && !w(ins)) { // Register offset
		*address = op(ins, *rn, rm);
	}
	else if(p(ins) && b && w(ins)) { // Immediate pre-indexed
		*address = op(ins, *rn, offset);
		if(condition_passed(proc, ins)) {
			*rn = *address;
		}
	}
	else if(p(ins) && !b && w(ins)) { // Register pre-indexed
		*address = op(ins, *rn, rm);
		if(condition_passed(proc, ins)) {
			*rn = *address;
		}
	}
	else if(!p(ins) && b && !w(ins)) { // Immediate post-indexed
		*address = *rn;
		if(condition_passed(proc, ins)) {
			*rn = op(ins, *rn, offset);
		}
	}
	else if(!p(ins) && !b && !w(ins)) { // Register post-indexed
		*address = *rn;
		if(condition_passed(proc, ins)) {
			*rn = op(ins, *rn, rm);
		}
	}
}
uint8_t executeInstr_miscellaneous(arm_core proc, uint32_t ins, uint32_t address) {
	uint32_t halfword;
 	//uint64_t doubleword;
	uint32_t simpleword;
	uint8_t byte_s;
	//int16_t halfword_s;
	uint8_t rd = get_bits(ins, 15, 12); // Numéro du registre qui sera chargé ou changé
	uint8_t l = get_bit(ins, 20);
    uint8_t s = get_bit(ins, 22);
	uint8_t h = get_bit(ins, 22);

	if(l) {
 		if(s && h) { // LDRSH
 			arm_read_word(proc, address, &halfword);
 			arm_write_register(proc, rd, halfword);
			return 0;
 		}
 		else if(s && !h) { // LDRSB
 			arm_read_byte(proc, address, &byte_s);
 			arm_write_register(proc, rd, byte_s);
			return 0;
 		}
		else if(!s && h) { // LDRH
 			arm_read_word(proc, address, &halfword);
 			arm_write_register(proc, rd, halfword);
			return 0;
 		}
 	}
 	else {
 		if(s && h) { // STRD
 			arm_read_register(proc, rd);
 			//arm_write_word(proc, address, doubleword);
			arm_write_word(proc, address, simpleword);
			return 0;
 		}
 		else if(!s && h) { // STRH
 			arm_read_register(proc, rd);
 			arm_write_word(proc, address, halfword);
			return 0;
 		}
		else if(s && !h) { // LDRD
			//arm_read_word(proc, address, &doubleword);
 			//arm_write_register(proc, rd, doubleword);
			arm_read_word(proc, address, &simpleword);
			arm_write_register(proc, rd, simpleword);
			return 0;
 		}
 	}
	return -1;
}

uint8_t executeInstr_word_byte(arm_core proc, uint32_t ins, uint32_t address) {
	uint8_t byte;
 	uint32_t word;
	uint8_t rd = get_bits(ins, 15, 12); // Numéro du registre qui sera chargé ou changé
	uint8_t l = get_bit(ins, 20); // Dit si on est en load ou en store
    uint8_t b = get_bit(ins, 22); // Dit si on veut un acces avec un word ou un byte

	if(l) {
 		if(b) { // LDRB
 			arm_read_byte(proc, address, &byte);
 			arm_write_register(proc, rd, (uint32_t) byte);
			return 0;
 		}
 		else { // LDR
 			arm_read_word(proc, address, &word);
 			arm_write_register(proc, rd, word);
			return 0;
 		}
 	}
 	else {
 		if(b) { // STRB
 			arm_read_register(proc, rd);
 			arm_write_byte(proc, address, byte);
			return 0;
 		}
 		else { // STR
 			arm_read_register(proc, rd);
 			arm_write_word(proc, address, word);
			return 0;
 		}
 	}
	return -1;
}

uint8_t executeInstr_multiple(arm_core proc, uint32_t ins, uint32_t start_address, uint32_t end_address) {
	uint8_t i;
	uint8_t pc_nb = 15;
	uint32_t registers = get_bits(ins, pc_nb, 0);
	uint8_t l = get_bit(ins, 20);
    uint32_t address;

	if(l) { // LDM
		uint32_t value;
		uint8_t pc = get_bit(registers, pc_nb);
		address = start_address;
		for(i = 0; i <= 14; i++) {
			if(get_bit(registers, i)) {
				arm_read_word(proc, address, &value);
				arm_write_register(proc, i, value);
			    address += 4;
			}
		}
		if(pc) {
			arm_read_word(proc, address, &value);
			arm_write_register(proc, pc_nb, value & 0xFFFFFFFE);
			address += 4;
		}
		assert(end_address == (address - 4));
	}
	else { // STM
		address = start_address;
		for(i = 0; i <= 15; i++) {
			if(get_bit(registers, i)) {
				arm_write_word(proc, address, arm_read_register(proc, i));
				address += 4;
			}
		}
		assert(end_address == (address - 4));
	}

    return 0; // ??
}




int arm_load_store(arm_core proc, uint32_t ins) {
	uint8_t id;
	uint32_t offset, rn, address, rm, index;

	rn = arm_read_register(proc, get_bits(ins, 19, 16)); // Numéro du registre qui officie de premier opérande
	rm = arm_read_register(proc, get_bits(ins, 3, 0)); // Spécifie le numéro du registre qui contient l'offset à appliquer à rn

    offset = get_bits(ins, 11, 0);
	id = get_bits(ins, 27, 25);

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
			if(!get_bits(ins, 11, 4)) { // Part 2 : Register
				load_store(proc, ins, &address, &rn, rm);
				return executeInstr_word_byte(proc, ins, address);
			}
			else if(!get_bit(ins, 4)) {
				if(!p(ins) && w(ins)) {}
				else { // Part 3 : Scaled register
					index = set_index(proc, ins, rm);
					load_store(proc, ins, &address, &rn, index);
					return executeInstr_word_byte(proc, ins, address);
				}
			}
		break;

		default:
			return -1;
		break;
	}
	return 0;

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

