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
#include "arm_instruction.h"
#include "arm_exception.h"
#include "arm_data_processing.h"
#include "arm_load_store.h"
#include "arm_branch_other.h"
#include "arm_constants.h"
#include "util.h"

uint8_t arm_decode_condition[224];
uint8_t arm_decode_condition_init = 0;

void init_decode_condition() {
	if(!arm_decode_condition_init) {
		arm_decode_condition_init = 1;
		
		for(int condCounter = 0 ; condCounter < 14 ; condCounter++) {
			for(int flagCounter = 0 ; flagCounter < 16 ; flagCounter++) {
				int res = 0;
				switch(condCounter) {
					case 0b0000:
						res = (flagCounter & 0b0100) != 0;
						break;
					case 0b0001:
						res = (flagCounter & 0b0100) == 0;
						break;
					case 0b0010:
						res = (flagCounter & 0b0010) != 0;
						break;
					case 0b0011:
						res = (flagCounter & 0b0010) == 0;
						break;
					case 0b0100:
						res = (flagCounter & 0b1000) != 0;
						break;
					case 0b0101:
						res = (flagCounter & 0b1000) == 0;
						break;
					case 0b0110:
						res = (flagCounter & 0b0001) != 0;
						break;
					case 0b0111:
						res = (flagCounter & 0b0001) == 0;
						break;
					case 0b1000:
						res = (flagCounter & 0b0110) == 0b0010;
						break;
					case 0b1001:
						res = (flagCounter & 0b0100) != 0 || (flagCounter & 0b0010) == 0;
						break;
					case 0b1010:
						res = ((flagCounter & 0b1000) >> 3) == (flagCounter & 0b0001);
						break;
					case 0b1011:
						res = ((flagCounter & 0b1000) >> 3) != (flagCounter & 0b0001);
						break;
					case 0b1100:
						res = ((flagCounter & 0b1000) >> 3) == (flagCounter & 0b0001) && (flagCounter & 0b0100) == 0;
						break;
					case 0b1101:
						res = ((flagCounter & 0b1000) >> 3) != (flagCounter & 0b0001) || (flagCounter & 0b0100) != 0;
						break;
				}
				arm_decode_condition[(condCounter<<4)|flagCounter] = res;
			}
		}
	}
}

static int arm_execute_instruction(arm_core p) {
	uint32_t inst;
	uint8_t instType, cond, flags;
	int res = arm_fetch(p, &inst);
	
	if(res != 0)
		return PREFETCH_ABORT;
	
	
	cond =  get_bits(inst, 31, 28);
	
	if (cond == 0b1111)
		return UNDEFINED_INSTRUCTION;
	
	if(cond != 0b1110) {
		flags = get_bits(arm_read_cpsr(p), 31, 28);
		init_decode_condition();
		
		if(!arm_decode_condition[(cond<<4)|flags]) {
			return 0;
		}
	}
	
	instType = get_bits(inst, 27, 25);
	switch(instType) {
		case 0:
			if(get_bits(inst, 24, 23) == 0b10 && get_bits(inst, 21, 20) == 0b00) {
				return arm_miscellaneous(p, inst);
			}
			else if(get_bit(inst, 4) == 1 && get_bit(inst, 7) == 1) {
				return arm_load_store(p, inst);
			}
			else {
				return arm_data_processing_shift(p, inst);
			}
		case 1:
			return arm_data_processing_immediate_msr(p, inst);
		case 2:
			return arm_load_store(p, inst);
		case 3:
			return arm_load_store(p, inst);
		case 4:
			return arm_load_store_multiple(p, inst);
		case 6:
			return arm_coprocessor_load_store(p, inst);
		case 5:
			return arm_branch(p, inst);
		case 7:
			return arm_coprocessor_others_swi(p, inst);
	}
	
    return 0;
}

int arm_step(arm_core p) {
    int result;

    result = arm_execute_instruction(p);
    if (result)
        arm_exception(p, result);
    return result;
}
