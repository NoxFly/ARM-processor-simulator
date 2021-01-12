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
#include "arm_branch_other.h"
#include "arm_constants.h"
#include "util.h"
#include <debug.h>
#include <stdlib.h>


int arm_branch(arm_core p, uint32_t ins) {
    int32_t adress;
    if(get_bit(ins, 24)){ // Bit 24 --> BL (voir doc ARM A4.1.5), on stocke la valeur dans LR (R14)
        arm_write_register(p, 14, arm_read_register(p, 15)-4); //MOV PC LR
    }
	adress = get_bits(ins, 23, 0);// voir operation dans la doc ARM A4.11
	if(get_bit(ins, 23)) {
		adress = -(((~(adress << 8)) >> 8) + 1);
	}

    arm_write_register(p, 15, arm_read_register(p, 15) + (adress << 2));

    return 0;
}

int arm_coprocessor_others_swi(arm_core p, uint32_t ins) {
    if (get_bit(ins, 24)) {
        /* Here we implement the end of the simulation as swi 0x123456 */
        if ((ins & 0xFFFFFF) == 0x123456)
            exit(0);
        return SOFTWARE_INTERRUPT;
    } 
    return UNDEFINED_INSTRUCTION;
}

void write_psr(arm_core p, uint32_t ins, uint8_t value, int R) {
	int psr = R ? arm_read_spsr(p) : arm_read_cpsr(p);
				
	if(get_bit(ins, 16))
		psr = set_bits(psr, 7, 0, value);
	
	if(get_bit(ins, 17))
		psr = set_bits(psr, 15, 8, value);
	
	if(get_bit(ins, 18))
		psr = set_bits(psr, 23, 16, value);
	
	if(get_bit(ins, 19)) {
		psr = set_bits(psr, 30, 24, value);// set_bits(psr, 31, 24, value) ne marche pas...
		int bit31 = get_bit(value, 7);
		psr = bit31 ? set_bit(psr, 31) : clr_bit(psr, 31);
	}
	
	if(R) 
		arm_write_spsr(p, psr);
	else
		arm_write_cpsr(p, psr);
}

int arm_miscellaneous(arm_core p, uint32_t ins) { //Reference à A4.1.38 et A4.1.39 de la doc ARM
	if(get_bits(ins, 21, 20) == 0b10) {
		if(get_bit(ins, 25)) {
			int rotate_imm = get_bits(ins, 11, 8);
			uint8_t immediate = get_bits(ins, 7, 0);
			immediate = ror(immediate, 2*rotate_imm);
			
			write_psr(p, ins, immediate, get_bit(ins, 22));
		}
		else {
			int rm = get_bits(ins, 3, 0);
			uint8_t value = arm_read_register(p, rm);
			
			write_psr(p, ins, value, get_bit(ins, 22));
		}
	}
	else {
		int rd = get_bits(ins, 15, 12);

		if(get_bit(ins, 22)) {
			arm_write_register(p, rd, arm_read_spsr(p));
		}
		else {
			arm_write_register(p, rd, arm_read_cpsr(p));
		}
	}
	
    return 0;
}
