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
#ifndef __ARM_LOAD_STORE_H__
#define __ARM_LOAD_STORE_H__

#include <stdint.h>
#include "arm_core.h"

#define p(ins)(get_bit(ins, 24))
#define w(ins)(get_bit(ins, 21))

/* Instructions implémentées :
avec arm_load_store :
	 word/byte : LDR, STR, LDRB, STRB
	miscellaneous : LDRH, STRH, LDRD, STRD
avec arm_load_store_multiple :
	LDM(1), STM(1)
*/

/* Retourne TRUE si l’état des flags N, Z, C et V 
remplit la condition encodée dans l’argument cond,
et retourne FALSE dans tous les autres cas */
uint8_t condition_passed(arm_core proc, uint32_t ins);

/*Effectue une opération en fonction du flag u */
uint32_t op(uint32_t ins, uint32_t left_op, uint32_t right_op);

/* Permet de connaître le nombre de bits a 1 sur un nombre passé en argument*/
uint8_t nb_set_bits(uint16_t nb);

/*Sert a trouver l'offset pour les instructions miscellaneous de load_store*/
uint32_t set_offset(uint32_t ins);

/*Sert a trouver l'index pour les instructions scaled de load_store*/
uint32_t set_index(arm_core proc, uint32_t ins, uint8_t rm);

/* manipule le mode d'adressage des dites instructions load / store */
void addrmode_load_store(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t add_value);
void addrmode_load_store_multiple(arm_core proc, uint32_t ins, uint32_t *start_address,uint32_t *end_address, uint32_t *rn);
void addrmode_load_store_miscellaneous(arm_core proc, uint32_t ins, uint32_t *address, uint32_t *rn, uint32_t offset);

/* execute les dites instructions*/
uint8_t executeInstr_miscellaneous(arm_core proc, uint32_t ins, uint32_t address);
uint8_t executeInstr_word_byte(arm_core proc, uint32_t ins, uint32_t address);
uint8_t executeInstr_multiple(arm_core proc, uint32_t ins, uint32_t start_address, uint32_t end_address);

/* Fonctions main du fichier load_store appellées dans arm_instructions */
int arm_load_store(arm_core p, uint32_t ins);
int arm_load_store_multiple(arm_core p, uint32_t ins);
int arm_coprocessor_load_store(arm_core p, uint32_t ins);



#endif
