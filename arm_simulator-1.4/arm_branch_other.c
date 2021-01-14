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
		adress = -(((~(adress << 8)) >> 8) + 1); // Si l'instruction est négative, il faut faire le complément à 2 des bits de poid fort
	}

    arm_write_register(p, 15, arm_read_register(p, 15) + (adress << 2)); // On écris l'adresse de la prochaine instruction dans PC

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
	int psr = R ? arm_read_spsr(p) : arm_read_cpsr(p); // Si R alors on opere sur SPSR, sinon sur CPSR
				
	if(get_bit(ins, 16)) //Si le bit 16 de l'instruction est a 1, alors on remplis les 8 bits de poind faibles de pcr avec les bits de value
		psr = set_bits(psr, 7, 0, value);
	
	if(get_bit(ins, 17)) //Si le bit 16 de l'instruction est a 1, alors on remplis les bits entre 8 et 15 de pcr avec les bits de value
		psr = set_bits(psr, 15, 8, value);
	
	if(get_bit(ins, 18))//Si le bit 16 de l'instruction est a 1, alors on remplis les bits entre 16 et 23 de pcr avec les bits de value
		psr = set_bits(psr, 23, 16, value);
	
	if(get_bit(ins, 19)) { //Si le bit 16 de l'instruction est a 1, alors on remplis les bits entre 24 et 30 de pcr avec les bits de value
		psr = set_bits(psr, 30, 24, value);// set_bits(psr, 31, 24, value) ne marche pas...
		int bit31 = get_bit(value, 7); // On récupère le bit de poid fort de value
		psr = bit31 ? set_bit(psr, 31) : clr_bit(psr, 31); // Si le bit de poid fort de psr sera la valeur du bit de poid fort de value
	}
	// On écris dans le registre sélctionné
	if(R) 
		arm_write_spsr(p, psr);
	else
		arm_write_cpsr(p, psr);
}

int arm_miscellaneous(arm_core p, uint32_t ins) { //Reference à A4.1.38 et A4.1.39 de la doc ARM
	if(get_bits(ins, 21, 20) == 0b10) { // Cas du MSR (A4.1.39)
		if(get_bit(ins, 25)) { // Cas du immediate operand
			int rotate_imm = get_bits(ins, 11, 8);
			uint8_t immediate = get_bits(ins, 7, 0); //On récupère la valeur à transférer dans SPSR ou CPSR 
			immediate = ror(immediate, 2*rotate_imm);
			
			write_psr(p, ins, immediate, get_bit(ins, 22)); // On stocke la valeur dans SPSR ou CPSR en fonction du bit 22
		}
		else { // Cas du register operand
			int rm = get_bits(ins, 3, 0);
			uint8_t value = arm_read_register(p, rm); //On récupère la valeur dans le registre indiqué

			write_psr(p, ins, value, get_bit(ins, 22)); // On stocke la valeur dans SPSR ou CPSR en fonction du bit 22
		}
	}
	else { // Cas dy MRS (A4.1.38)
		int rd = get_bits(ins, 15, 12); // On récupère le registre de destination

		if(get_bit(ins, 22)) { // Si R == 1
			arm_write_register(p, rd, arm_read_spsr(p)); // RD = SPSR
		}
		else { // Si R != 1
			arm_write_register(p, rd, arm_read_cpsr(p)); // RD = CPSR
		}
	}
	
    return 0;
}
