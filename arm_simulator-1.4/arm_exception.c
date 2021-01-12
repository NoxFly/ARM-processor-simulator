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
#include "arm_exception.h"
#include "arm_constants.h"
#include "arm_core.h"
#include "util.h"
#include <string.h>

#define CP15_reg1_EEbit 0
#define HIGH_VECTOR_ADDRESS 0
#define Exception_bit_9 (CP15_reg1_EEbit << 9)

#define SP 13
#define LR 14
#define PC 15


void branch_exception_vector(arm_core p, int32_t address) {
    if (HIGH_VECTOR_ADDRESS) address |= 0xFFFF0000;
	else address &= 0xFFFF;
    arm_write_register(p, PC, address);
}


static void execute_(arm_core p, int needSpsr, int cpsr_bits_value, int cpsr_values_count, char* state, int16_t exception_vector) {
    int32_t cpsr = arm_read_cpsr(p);
    uint32_t sp = arm_read_register(p, SP);

    int addr =
          (strcmp(state, "current") == 0)? arm_read_register(p, 15)-4
        : (strcmp(state, "next") == 0)? arm_read_register(p, 15)
        : 0;
    
    cpsr = set_bits(cpsr, 4, 0, cpsr_bits_value);
    cpsr = clr_bit(cpsr, 5);
    if(cpsr_values_count == 3) cpsr = set_bit(cpsr, 6);
    cpsr = set_bit(cpsr, 7);
    cpsr = CP15_reg1_EEbit ? set_bit(cpsr, 9) : clr_bit(cpsr, 9);

    arm_write_cpsr(p, cpsr);
    arm_write_register(p, SP, sp);
    arm_write_register(p, LR, addr);

    if(needSpsr) {
        int32_t spsr = cpsr;
        arm_write_spsr(p, spsr);
    }

    branch_exception_vector(p, exception_vector);
}


static void execute_reset(arm_core p) {
    execute_(p, 0, 0b10011, 3, "next", 0);
}

static void execute_undefined_instruction(arm_core p) {
    execute_(p, 1, 0b11011, 2, "next", 0x4);
}

static void execute_software_interrupt(arm_core p) {
    execute_(p, 1, 0b10011, 2, "next", 0x8);
}

static void execute_prefetch_abort(arm_core p) {
    execute_(p, 1, 0b10111, 2, "current", 0xC);
}

static void execute_data_abort(arm_core p) {
    execute_(p, 1, 0b10111, 2, "current", 0x10);
}

static void execute_irq(arm_core p) {
    execute_(p, 1, 0b10010, 2, "next", 0x18);
}

static void execute_fast_irq(arm_core p) {
    execute_(p, 1, 0b10001, 3, "next", 0x1C);
}


void arm_exception(arm_core p, unsigned char exception) {
    switch (exception) {
        case RESET:                 execute_reset(p); break;
        case UNDEFINED_INSTRUCTION: execute_undefined_instruction(p); break;
        case SOFTWARE_INTERRUPT:    execute_software_interrupt(p); break;
        case PREFETCH_ABORT:        execute_prefetch_abort(p); break;
        case DATA_ABORT:            execute_data_abort(p); break;
        case INTERRUPT:             execute_irq(p); break;
        case FAST_INTERRUPT:        execute_fast_irq(p); break;
        default: break;
    }
}
