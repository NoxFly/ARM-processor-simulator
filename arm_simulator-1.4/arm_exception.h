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
#ifndef __ARM_EXCEPTION_H__
#define __ARM_EXCEPTION_H__
#include <stdint.h>
#include "arm_core.h"


void branch_exception_vector(arm_core p, int32_t address);
static void execute_irq(arm_core p);
static void execute_reset(arm_core p)
static void execute_fast_irq(arm_core p);
static void execute_data_abort(arm_core p);
static void execute_prefetch_abort(arm_core p);
static void execute_software_interrupt(arm_core p);
static void execute_undefined_instruction(arm_core p);
void arm_exception(arm_core p, unsigned char exception);

#endif
