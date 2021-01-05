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
#include "arm_data_processing.h"
#include "arm_exception.h"
#include "arm_constants.h"
#include "arm_branch_other.h"
#include "util.h"
#include "debug.h"

int executeInst(int opcode, arm_core p, int rd, int valueRn, int shifter_operand) {
	int carryFlag;
	
	switch(opcode) {
		case 0b0000://AND
			arm_write_register(p, rd, valueRn && shifter_operand);
			break;
		case 0b0001://EOR
			arm_write_register(p, rd, (valueRn || shifter_operand) && !(valueRn && shifter_operand));
			break;
		case 0b0010://SUB
			arm_write_register(p, rd, valueRn - shifter_operand);
			break;
		case 0b0011://RSB
			arm_write_register(p, rd, shifter_operand - valueRn);
			break;
		case 0b0100://ADD
			arm_write_register(p, rd, shifter_operand + valueRn);
			break;
		case 0b0101://ADC
			carryFlag = (arm_read_cpsr(p) >> 29) && 0x1;
			arm_write_register(p, rd, shifter_operand - valueRn + carryFlag);
			break;
		case 0b0110://SBC
			carryFlag = (arm_read_cpsr(p) >> 29) && 0x1;
			arm_write_register(p, rd, valueRn - shifter_operand - (~carryFlag));
			break;
		case 0b0111://RSC
			carryFlag = (arm_read_cpsr(p) >> 29) && 0x1;
			arm_write_register(p, rd, shifter_operand - valueRn - (~carryFlag));
			break;
		case 0b1000://TST
			//Update flags after Rn AND shifter_operand
			break;
		case 0b1001://TEQ
			//Update flags after Rn EOR shifter_operand
			break;
		case 0b1010://CMP
			// Update flags after Rn - shifter_operand
			break;
		case 0b1011://CMN
			//Update flags after Rn + shifter_operand
			break;
		case 0b1100://ORR
			arm_write_register(p, rd, valueRn || shifter_operand);
			break;
		case 0b1101://MOV
			arm_write_register(p, rd, shifter_operand);
			break;
		case 0b1110://BIC
			arm_write_register(p, rd, valueRn && (~shifter_operand));
			break;
		case 0b1111://MVN
			arm_write_register(p, rd, ~shifter_operand);
			break;
	}
	
	return UNDEFINED_INSTRUCTION;
}

/* Decoding functions for different classes of instructions */
int arm_data_processing_shift(arm_core p, uint32_t ins) {
	int opcode = get_bits(ins, 24, 21);
	//int updateCPSR = get_bit(ins, 20);  CHECK FOR FLAGS
	int rn = get_bits(ins, 19, 16);
	int rd = get_bits(ins, 15, 12);
	
	int rm = get_bits(ins, 3, 0);
	int shifter_operand = arm_read_register(p, rm);
	int valueRn = arm_read_register(p, rn);
	
	if(rm == 15) {
		shifter_operand += 8;
	}
	if(get_bits(ins, 6, 4) == 0b000) { //LSL immediate
		int shift_imm = get_bits(ins, 11, 7);
		if(shift_imm > 0) {
			shifter_operand = shifter_operand << shift_imm;
		}
	}
	else if(get_bits(ins, 7, 4) == 0b0001) { //LSL register
		int rs = get_bits(ins, 11, 8);
		int shift_imm = arm_read_register(p, rs);
		if(shift_imm > 0) {
			shifter_operand = shifter_operand << shift_imm;
		}
	}
	else if(get_bits(ins, 6, 4) == 0b010) { //LSR immediate
		int shift_imm = get_bits(ins, 11, 7);
		if(shift_imm > 0) {
			shifter_operand = shifter_operand >> shift_imm;
		}
	}
	else if(get_bits(ins, 7, 4) == 0b0011) { //LSR register
		int rs = get_bits(ins, 11, 8);
		int shift_imm = arm_read_register(p, rs);
		if(shift_imm > 0) {
			shifter_operand = shifter_operand >> shift_imm;
		}
	}
	else if(get_bits(ins, 6, 4) == 0b100) { //ASR immediate
		int shift_imm = get_bits(ins, 11, 7);
		if(shift_imm > 0) {
			shifter_operand = asr(shifter_operand, shift_imm);
		}
	}
	else if(get_bits(ins, 7, 4) == 0b0101) { //ASR register
		int rs = get_bits(ins, 11, 8);
		int shift_imm = arm_read_register(p, rs);
		if(shift_imm > 0) {
			shifter_operand = asr(shifter_operand, shift_imm);
		}
	}
	else if(get_bits(ins, 6, 4) == 0b110) { //ROR immediate
		int shift_imm = get_bits(ins, 11, 7);
		if(shift_imm > 0) {
			shifter_operand = ror(shifter_operand, shift_imm);
		}
	}
	else if(get_bits(ins, 7, 4) == 0b0111) { //ROR register
		int rs = get_bits(ins, 11, 8);
		int shift_imm = arm_read_register(p, rs);
		if(shift_imm > 0) {
			shifter_operand = ror(shifter_operand, shift_imm);
		}
	}
	
    return executeInst(opcode, p, rd, valueRn, shifter_operand);
}

int arm_data_processing_immediate_msr(arm_core p, uint32_t ins) {
    int opcode = get_bits(ins, 24, 21);
	//int updateCPSR = get_bit(ins, 20); CHECK FOR FLAGS
	int rn = get_bits(ins, 19, 16);
	int rd = get_bits(ins, 15, 12);
	int rotate_imm = get_bits(ins, 11, 8);
	int shifter_operand = get_bits(ins, 7, 0);
	
	shifter_operand = ror(shifter_operand, 2*rotate_imm);
	int valueRn = arm_read_register(p, rn);
	
    return executeInst(opcode, p, rd, valueRn, shifter_operand);
}
