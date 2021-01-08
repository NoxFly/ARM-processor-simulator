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
#ifndef __ARM_LOAD_STORE_H__
#define __ARM_LOAD_STORE_H__
#include <stdint.h>
#include "arm_core.h"
uint8_t condition_passed(arm_core proc, uint32_t ins);
uint32_t op(uint32_t ins, uint32_t left_op, uint32_t right_op);
uint8_t nb_set_bits(uint16_t nb);
uint32_t set_offset(uint32_t ins);
uint32_t set_index(arm_core proc, uint32_t ins, uint8_t rm);
void load_store(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t add_value);
void load_store_multiple(arm_core proc, uint32_t ins, uint32_t *start_address,uint32_t *end_address, uint32_t *rn);
void load_store_miscellaneous(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t offset);
uint8_t executeInstr_miscellaneous(arm_core proc, uint32_t ins, uint32_t address);
uint8_t executeInstr_word_byte(arm_core proc, uint32_t ins, uint32_t address);
uint8_t executeInstr_multiple(arm_core proc, uint32_t ins, uint32_t *address, uint32_t start_address, uint32_t end_address);
int arm_load_store(arm_core p, uint32_t ins);
int arm_load_store_multiple(arm_core p, uint32_t ins);
int arm_coprocessor_load_store(arm_core p, uint32_t ins);


#endif
