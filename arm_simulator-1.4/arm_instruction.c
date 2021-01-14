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
#include "arm_instruction.h"
#include "arm_exception.h"
#include "arm_data_processing.h"
#include "arm_load_store.h"
#include "arm_branch_other.h"
#include "arm_constants.h"
#include "util.h"

uint8_t arm_decode_condition[224]; //La taille est 224 car condCounter < 14 et flagCounter < 16. On accède à un élément de cette façon : 
								   //"arm_decode_condition[(condCounter<<4)|flagCounter]" Donc l'indice maximum est 223 soit 224 éléments maximum.
uint8_t arm_decode_condition_init = 0;//Variable permettant de savoir si le tableau ci-dessus a été initialisé

void init_decode_condition() { //Création d'un tableau permettant de récupérer directement le booléen correspondant à la condition et aux flags donnés
	if(!arm_decode_condition_init) { // Si le tableau n'a pas été initialisé
		arm_decode_condition_init = 1;
		
		for(int condCounter = 0 ; condCounter < 14 ; condCounter++) {
			for(int flagCounter = 0 ; flagCounter < 16 ; flagCounter++) {
				int res = 0;
				switch(condCounter) {
					case 0b0000: //EQ
						res = (flagCounter & 0b0100) != 0; // Z
						break;
					case 0b0001: //NE
						res = (flagCounter & 0b0100) == 0; // /Z
						break;
					case 0b0010: //CS/HS
						res = (flagCounter & 0b0010) != 0; // C
						break;
					case 0b0011: //CC/LO
						res = (flagCounter & 0b0010) == 0; // /C
						break;
					case 0b0100: //MI
						res = (flagCounter & 0b1000) != 0; // N
						break;
					case 0b0101: //PL
						res = (flagCounter & 0b1000) == 0; // /N
						break;
					case 0b0110: //VS
						res = (flagCounter & 0b0001) != 0; // V
						break;
					case 0b0111: //VC
						res = (flagCounter & 0b0001) == 0; // /V
						break;
					case 0b1000: //HI
						res = (flagCounter & 0b0110) == 0b0010; // C && /Z
						break;
					case 0b1001: //LS
						res = (flagCounter & 0b0100) != 0 || (flagCounter & 0b0010) == 0; // /C || Z
						break;
					case 0b1010: //GE
						res = ((flagCounter & 0b1000) >> 3) == (flagCounter & 0b0001); // N == V
						break;
					case 0b1011: //LT
						res = ((flagCounter & 0b1000) >> 3) != (flagCounter & 0b0001); // N != V
						break;
					case 0b1100: //GT
						res = ((flagCounter & 0b1000) >> 3) == (flagCounter & 0b0001) && (flagCounter & 0b0100) == 0; // /Z && N==V
						break;
					case 0b1101: //LE
						res = ((flagCounter & 0b1000) >> 3) != (flagCounter & 0b0001) || (flagCounter & 0b0100) != 0; // Z || N != V
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
	int res = arm_fetch(p, &inst); // On récupère l'instruction à éxécuter (PC est incrémenté dans cette fonction)
	
	if(res != 0)
		return PREFETCH_ABORT;
	
	cond =  get_bits(inst, 31, 28);
	
	if (cond == 0b1111) // Instruction inconnue
		return UNDEFINED_INSTRUCTION;
	
	if(cond != 0b1110) { // NOT ALWAYS
		flags = get_bits(arm_read_cpsr(p), 31, 28); // flags ZNCV
		init_decode_condition();
		
		if(!arm_decode_condition[(cond<<4)|flags]) { // Si on ne passe pas la condition, l'instruction n'est pas exécutée
			return 0;
		}
	}
	
	instType = get_bits(inst, 27, 25);
	switch(instType) {
		case 0:
			if(get_bits(inst, 24, 23) == 0b10 && (get_bits(inst, 21, 20) == 0b00 || get_bits(inst, 21, 20) == 0b10)) { // MRS et MSR
				return arm_miscellaneous(p, inst);
			}
			else if(get_bit(inst, 4) == 1 && get_bit(inst, 7) == 1) { // Extra load/stores (load and store halfword, doubleword, load signed byte)
				return arm_load_store(p, inst);
			}
			else {
				return arm_data_processing_shift(p, inst);
			}
		case 1:
			if(get_bits(inst, 24, 23) == 0b10 && get_bits(inst, 21, 20) == 0b10) { // MSR
				return arm_miscellaneous(p, inst);
            } else {
				return arm_data_processing_immediate_msr(p, inst);
			}
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
			return arm_coprocessor_others_swi(p, inst); // Fin de programme
	}
	return 0;
}


int arm_step(arm_core p) {
    int result = arm_execute_instruction(p);
    if (result)
        arm_exception(p, result);
    return result;
}