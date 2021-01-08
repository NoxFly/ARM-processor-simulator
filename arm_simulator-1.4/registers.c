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
#include "registers.h"
#include "arm_constants.h"
#include <stdlib.h>
#include <stdio.h>

#define NB_REGISTER 37

uint8_t arm_mode_register[208];
uint8_t arm_mode_register_init = 0;

void init() {
    if(!arm_mode_register_init) {
		arm_mode_register_init = 1;
		
		for(int mode = 0 ; mode < 7 ; mode++) {
			for(int reg = 0 ; reg < 17 ; reg++) { // register = 16 -> CPSR ; register = 17 -> SPSR
				uint8_t val = 0;
				
				switch(mode) {
					case USR:
					case SYS:
						val = reg;
						break;
					case SVC:
					case ABT:
					case UND:
					case IRQ: {
						if(reg <= 12 || reg == 15 || reg == 16) {
							val = reg;
						}
						else { 
							int i = reg <= 14 ? reg - 13 : 2;
							val =   mode == SVC ? 
										R13_SVC : 
									mode == ABT ? 
										R13_ABT : 
									mode == UND ? 
										R13_UND : 
									R13_IRQ; 
							val += i;
						}
						break;
					}
					case FIQ: {
						if(reg <= 7 || reg == 15 || reg == 16) {
							val = reg;
						}
						else { 
							int i = reg <= 14 ? reg - 8 : 7; 
							val = R8_FIQ + i;
						}
						break;
					}
				}

				arm_mode_register[(mode << 5)|reg] = val;
			}
		}
	}
}

struct registers_data {
    uint32_t registers[NB_REGISTER];
	uint8_t mode;
};

registers registers_create() {
	registers r = malloc(sizeof(struct registers_data));
	r->mode = USR;
    return r;
}

void registers_destroy(registers r) {
    free(r);
}

uint8_t is_valid(uint8_t reg) {
	return reg >= 0 && reg <= 15;
}

uint8_t has_spsr(registers r) {
	return r->mode != USR && r->mode != SYS;
}

uint8_t get_mode(registers r) {
    return r->mode;
} 

int current_mode_has_spsr(registers r) {
    return r->mode == SVC || r->mode == ABT || r->mode == UND || r->mode == IRQ || r->mode == FIQ;
}

int in_a_privileged_mode(registers r) {
    return r->mode == SYS || r->mode == SVC || r->mode == ABT || r->mode == UND || r->mode == IRQ || r->mode == FIQ;
}

uint32_t read_register(registers r, uint8_t reg) {
	if(!is_valid(reg)) return 0;
	init();
	return r->registers[arm_mode_register[(r->mode << 5)|reg]];
}

uint32_t read_usr_register(registers r, uint8_t reg) {
    return r->registers[reg];
}

uint32_t read_cpsr(registers r) {
    return r->registers[CPSR];
}

uint32_t read_spsr(registers r) {
	if(!has_spsr(r)) return 0;
	init();
	return r->registers[arm_mode_register[(r->mode << 5)|17]];
}

void write_register(registers r, uint8_t reg, uint32_t value) {
	if(!is_valid(reg)) return;
	init();
	r->registers[arm_mode_register[(r->mode << 5)|reg]] = value;
}

void write_usr_register(registers r, uint8_t reg, uint32_t value) {
	r->registers[reg] = value;
}

void write_cpsr(registers r, uint32_t value) {
	r->registers[CPSR] = value;
}

void write_spsr(registers r, uint32_t value) {
	if(!has_spsr(r)) return;
	init();
	r->registers[arm_mode_register[(r->mode << 5)|17]] = value;
}
