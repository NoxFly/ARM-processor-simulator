/*
Armator - simulateur de jeu d'instruction ARMv5T ? but p?dagogique
Copyright (C) 2011 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique G?n?rale GNU publi?e par la Free Software
Foundation (version 2 ou bien toute autre version ult?rieure choisie par vous).

Ce programme est distribu? car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but sp?cifique. Reportez-vous ? la
Licence Publique G?n?rale GNU pour plus de d?tails.

Vous devez avoir re?u une copie de la Licence Publique G?n?rale GNU en m?me
temps que ce programme ; si ce n'est pas le cas, ?crivez ? la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
?tats-Unis.

Contact: Guillaume.Huard@imag.fr
	 B?timent IMAG
	 700 avenue centrale, domaine universitaire
	 38401 Saint Martin d'H?res
*/
#include "registers.h"
#include "arm_constants.h"
#include "util.h"
#include <stdlib.h>

#define NB_REGISTER 37

struct registers_data {
    uint32_t registers[NB_REGISTER];
};

registers registers_create() {
	registers r = malloc(sizeof(struct registers_data));
	
	int32_t cpsr = read_cpsr(r);
	cpsr = set_bits(cpsr, 4, 0, USR_MODE);
	write_cpsr(r, cpsr);

    return r;
}

void registers_destroy(registers r) {
    free(r);
}

uint8_t get_mode(registers r) {
	int32_t cpsr = read_cpsr(r);
	return get_bits(cpsr, 4, 0);
} 

int current_mode_has_spsr(registers r) {
	uint8_t mode = get_mode(r);
    return mode == SVC_MODE || mode == ABT_MODE || mode == UND_MODE || mode == IRQ_MODE || mode == FIQ_MODE;
}

int in_a_privileged_mode(registers r) {
	uint8_t mode = get_mode(r);
    return mode == SYS_MODE || mode == SVC_MODE || mode == ABT_MODE || mode == UND_MODE || mode == IRQ_MODE || mode == FIQ_MODE;
}

uint32_t read_register(registers r, uint8_t reg) {
    uint32_t value=0;
	
	uint8_t mode = get_mode(r);
	if(mode == FIQ_MODE && ((reg >= 8 && reg <= 14) || reg == 17)) {
		switch(reg) {
			case 8:
				value = r->registers[R8_FIQ];
				break;
			case 9:
				value = r->registers[R9_FIQ];
				break;
			case 10:
				value = r->registers[R10_FIQ];
				break;
			case 11:
				value = r->registers[R11_FIQ];
				break;
			case 12:
				value = r->registers[R12_FIQ];
				break;
			case 13:
				value = r->registers[R13_FIQ];
				break;
			case 14:
				value = r->registers[R14_FIQ];
				break;
			case 17:
				value = r->registers[SPSR_FIQ];
				break;
		}
	}
	else if((mode == SVC_MODE || mode == ABT_MODE || mode == UND_MODE || mode == IRQ_MODE) && (reg == 13 || reg == 14 || reg == 17)) {
		switch(reg) {
			case 13: {
				switch(mode) {
					case SVC_MODE:
						value = r->registers[R13_SVC];
						break;
					case ABT_MODE:
						value = r->registers[R13_ABT];
						break;
					case UND_MODE:
						value = r->registers[R13_UND];
						break;
					case IRQ_MODE:
						value = r->registers[R13_IRQ];
						break;
				}
				break;
			}
			case 14: {
				switch(mode) {
					case SVC_MODE:
						value = r->registers[R14_SVC];
						break;
					case ABT_MODE:
						value = r->registers[R14_ABT];
						break;
					case UND_MODE:
						value = r->registers[R14_UND];
						break;
					case IRQ_MODE:
						value = r->registers[R14_IRQ];
						break;
				}
				break;
			}
			case 17: {
				switch(mode) {
					case SVC_MODE:
						value = r->registers[SPSR_SVC];
						break;
					case ABT_MODE:
						value = r->registers[SPSR_ABT];
						break;
					case UND_MODE:
						value = r->registers[SPSR_UND];
						break;
					case IRQ_MODE:
						value = r->registers[SPSR_IRQ];
						break;
				}
				break;
			}
		}
	}
	else {
		value = r->registers[reg];
	}
	
    return value;
}

uint32_t read_usr_register(registers r, uint8_t reg) {
    uint32_t value=r->registers[reg];
    return value;
}

uint32_t read_cpsr(registers r) {
    uint32_t value=r->registers[CPSR];
    return value;
}

uint32_t read_spsr(registers r) {
    uint32_t value=0;
	
	switch(get_mode(r)) {
		case SVC_MODE:
			value = r->registers[SPSR_SVC];
			break;
		case ABT_MODE:
			value = r->registers[SPSR_ABT];
			break;
		case UND_MODE:
			value = r->registers[SPSR_UND];
			break;
		case IRQ_MODE:
			value = r->registers[SPSR_IRQ];
			break;
		case FIQ_MODE:
			value = r->registers[SPSR_FIQ];
			break;
	}
	
    return value;
}

void write_register(registers r, uint8_t reg, uint32_t value) {
	uint8_t mode = get_mode(r);
	if(mode == FIQ_MODE && ((reg >= 8 && reg <= 14) || reg == 17)) {
		switch(reg) {
			case 8:
				r->registers[R8_FIQ] = value;
				break;
			case 9:
				r->registers[R9_FIQ] = value;
				break;
			case 10:
				r->registers[R10_FIQ] = value;
				break;
			case 11:
				r->registers[R11_FIQ] = value;
				break;
			case 12:
				r->registers[R12_FIQ] = value;
				break;
			case 13:
				r->registers[R13_FIQ] = value;
				break;
			case 14:
				r->registers[R14_FIQ] = value;
				break;
			case 17:
				r->registers[SPSR_FIQ] = value;
				break;
		}
	}
	else if((mode == SVC_MODE || mode == ABT_MODE || mode == UND_MODE || mode == IRQ_MODE) && (reg == 13 || reg == 14 || reg == 17)) {
		switch(reg) {
			case 13: {
				switch(mode) {
					case SVC_MODE:
						r->registers[R13_SVC] = value;
						break;
					case ABT_MODE:
						r->registers[R13_ABT] = value;
						break;
					case UND_MODE:
						r->registers[R13_UND] = value;
						break;
					case IRQ_MODE:
						r->registers[R13_IRQ] = value;
						break;
				}
				break;
			}
			case 14: {
				switch(mode) {
					case SVC_MODE:
						r->registers[R14_SVC] = value;
						break;
					case ABT_MODE:
						r->registers[R14_ABT] = value;
						break;
					case UND_MODE:
						r->registers[R14_UND] = value;
						break;
					case IRQ_MODE:
						r->registers[R14_IRQ] = value;
						break;
				}
				break;
			}
			case 17: {
				switch(mode) {
					case SVC_MODE:
						r->registers[SPSR_SVC] = value;
						break;
					case ABT_MODE:
						r->registers[SPSR_ABT] = value;
						break;
					case UND_MODE:
						r->registers[SPSR_UND] = value;
						break;
					case IRQ_MODE:
						r->registers[SPSR_IRQ] = value;
						break;
				}
				break;
			}
		}
	}
	else {
		r->registers[reg] = value;
	}
}

void write_usr_register(registers r, uint8_t reg, uint32_t value) {
	r->registers[reg] = value;
}

void write_cpsr(registers r, uint32_t value) {
	r->registers[CPSR] = value;
}

void write_spsr(registers r, uint32_t value) {
	switch(get_mode(r)) {
		case SVC_MODE:
			r->registers[SPSR_SVC] = value;
			break;
		case ABT_MODE:
			r->registers[SPSR_ABT] = value;
			break;
		case UND_MODE:
			r->registers[SPSR_UND] = value;
			break;
		case IRQ_MODE:
			r->registers[SPSR_IRQ] = value;
			break;
		case FIQ_MODE:
			r->registers[SPSR_FIQ] = value;
			break;
	}
}
