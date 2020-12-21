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
#include "arm_core.h"
#include "registers.h"
#include "no_trace_location.h"
#include "arm_constants.h"
#include "arm_exception.h"
#include "util.h"
#include "trace.h"
#include <stdlib.h>

struct arm_core_data {
    uint32_t cycle_count;
    registers reg;
    memory mem;
};

arm_core arm_create(memory mem) {
    arm_core p;

    p = malloc(sizeof(struct arm_core_data));
    if (p) {
        p->mem = mem;
	p->reg = registers_create();
        arm_exception(p, RESET);
        p->cycle_count = 0;
    }
    return p;
}

void arm_destroy(arm_core p) {
    registers_destroy(p->reg);
    free(p);
}

int arm_current_mode_has_spsr(arm_core p) {
    return current_mode_has_spsr(p->reg);
}

int arm_in_a_privileged_mode(arm_core p) {
    return in_a_privileged_mode(p->reg);
}

uint32_t arm_get_cycle_count(arm_core p) {
    return p->cycle_count;
}

/* In this implementation, the program counter is incremented during the fetch.
 * Thus, to meet the specification (see manual A2-9), we add 4 whenever the
 * value of the pc is read, so that instructions read their own address + 8 when
 * reading the pc.
 */
uint32_t arm_read_register(arm_core p, uint8_t reg) {
    uint32_t value = read_register(p->reg, reg);
    if (reg == 15) {
        value += 4;
        value &= 0xFFFFFFFD;
    }
    trace_register(p->cycle_count, READ, reg, get_mode(p->reg), value);
    return value;
}

uint32_t arm_read_usr_register(arm_core p, uint8_t reg) {
    uint32_t value = read_usr_register(p->reg, reg);
    if (reg == 15) {
        value += 4;
        value &= 0xFFFFFFFD;
    }
    trace_register(p->cycle_count, READ, reg, USR, value);
    return value;
}

uint32_t arm_read_cpsr(arm_core p) {
    uint32_t value = read_cpsr(p->reg);
    trace_register(p->cycle_count, READ, CPSR, 0, value);
    return value;
}

uint32_t arm_read_spsr(arm_core p) {
    uint32_t value = read_spsr(p->reg);
    trace_register(p->cycle_count, READ, SPSR, get_mode(p->reg), value);
    return value;
}

void arm_write_register(arm_core p, uint8_t reg, uint32_t value) {
    write_register(p->reg, reg, value);
    trace_register(p->cycle_count, WRITE, reg, get_mode(p->reg), value);
}

void arm_write_usr_register(arm_core p, uint8_t reg, uint32_t value) {
    write_usr_register(p->reg, reg, value);
    trace_register(p->cycle_count, WRITE, reg, USR, value);
}

void arm_write_cpsr(arm_core p, uint32_t value) {
    write_cpsr(p->reg, value);
    trace_register(p->cycle_count, WRITE, CPSR, 0, value);
}

void arm_write_spsr(arm_core p, uint32_t value) {
    write_spsr(p->reg, value);
    trace_register(p->cycle_count, WRITE, SPSR, get_mode(p->reg), value);
}

/* According to the previous comment, the PC is read 8 byte after the address of the
 * instruction being executed and the fetch increments the PC (this makes the
 * implementation of branches easier).
 */
int arm_fetch(arm_core p, uint32_t *value) {
    int result;
    uint32_t address;

    p->cycle_count++;
    address = arm_read_register(p, 15) - 4;
    result = memory_read_word(p->mem, address, value);
    trace_memory(p->cycle_count, READ, 4, OPCODE_FETCH, address, *value);
    arm_write_register(p, 15, address + 4);
    return result;
}

int arm_read_byte(arm_core p, uint32_t address, uint8_t *value) {
    int result;

    result = memory_read_byte(p->mem, address, value);
    trace_memory(p->cycle_count, READ, 1, OTHER_ACCESS, address, *value);
    return result;
}

/* Data access endianess should comply with bit 9 of cpsr (E), see ARM
 * manual A4-129
 */
int arm_read_half(arm_core p, uint32_t address, uint16_t *value) {
    int result;

    result = memory_read_half(p->mem, address, value);
    trace_memory(p->cycle_count, READ, 2, OTHER_ACCESS, address, *value);
    return result;
}

int arm_read_word(arm_core p, uint32_t address, uint32_t *value) {
    int result;

    result = memory_read_word(p->mem, address, value);
    trace_memory(p->cycle_count, READ, 4, OTHER_ACCESS, address, *value);
    return result;
}

int arm_write_byte(arm_core p, uint32_t address, uint8_t value) {
    int result;

    result = memory_write_byte(p->mem, address, value);
    trace_memory(p->cycle_count, WRITE, 1, OTHER_ACCESS, address, value);
    return result;
}

int arm_write_half(arm_core p, uint32_t address, uint16_t value) {
    int result;

    result = memory_write_half(p->mem, address, value);
    trace_memory(p->cycle_count, WRITE, 2, OTHER_ACCESS, address, value);
    return result;
}

int arm_write_word(arm_core p, uint32_t address, uint32_t value) {
    int result;

    result = memory_write_word(p->mem, address, value);
    trace_memory(p->cycle_count, WRITE, 4, OTHER_ACCESS, address, value);
    return result;
}

void arm_print_state(arm_core p, FILE *out) {
    int mode, reg, count;

    for (mode = 0; mode < 32; mode++) {
        if (arm_get_mode_name(mode)) {
            if (mode != SYS)
                fprintf(out, "%s:", arm_get_mode_name(mode));
            count = 0;
            for (reg=0; reg<16; reg++) {
                if (mode == USR) {
                        if ((reg > 0) && (reg%5 == 0))
                            fprintf(out, "\n    ");
                        fprintf(out, "   %3s=%08X", arm_get_register_name(reg),
                                arm_read_usr_register(p, reg));
                } else //if ((p->registers[mode][reg] - p->registers_storage) > 15) 
                       {
                        if ((count > 0) && (count%5 == 0))
                            fprintf(out, "\n    ");
                        count++;
                        fprintf(out, "   %3s=%08X", arm_get_register_name(reg),
                                arm_read_register(p, reg));
                }
            }
            if (mode == USR)
                fprintf(out, "  CPSR=%08X", arm_read_cpsr(p));
            switch (mode) {
              case USR:
              case FIQ:
              case SVC:
              case UND:
                fprintf(out, "\n");
                break;
              case IRQ:
              case ABT:
                fprintf(out, "          ");
            }
        }
    }
}
