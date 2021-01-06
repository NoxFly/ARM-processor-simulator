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

int carryFlagAdd(uint32_t left, uint32_t right) {
	int cFlag = 0;
	for(int i = 0 ; i < 31 ; i++) {
		int leftBit = get_bit(left, i);
		int rightBit = get_bit(right, i);
		int res = leftBit + rightBit + cFlag;
		if(res > 1) {
			cFlag = 1;
			res -= 2;
		}
		else 
			cFlag = 0;
	}
	
	return cFlag;
}

int carryFlagSub(uint32_t left, uint32_t right) {
	int cFlag = 0;
	for(int i = 0 ; i < 31 ; i++) {
		int leftBit = get_bit(left, i);
		int rightBit = get_bit(right, i);
		int res = leftBit - rightBit + cFlag;
		if(res == 2 || res == -1) {
			cFlag = 1;
			res = res == 2 ? 0 : 1;
		}
		else
			cFlag = 0;
	}
	
	return cFlag;
}

int executeInst(int opcode, arm_core p, int rd, uint32_t valueRn, uint32_t shifter_operand, int updateCPSR) {
	int valueCpsr = arm_read_cpsr(p);
	int zFlag = get_bit(valueCpsr, 30);
	int nFlag = get_bit(valueCpsr, 31);
	int cFlag = get_bit(valueCpsr, 29);
	int vFlag = get_bit(valueCpsr, 28);
	uint32_t res;
	int write_register = 1;
	int leftBit;
	int rightBit;
	int oldcFlag;
	
	switch(opcode) {
		case 0b0000://AND
			res = valueRn && shifter_operand;
			break;
		case 0b0001://EOR
			res = (valueRn || shifter_operand) && !(valueRn && shifter_operand);
			break;
		case 0b0010://SUB
			res = valueRn - shifter_operand;
			cFlag = carryFlagSub(valueRn, shifter_operand);
			leftBit = get_bit(valueRn, 31);
			rightBit = get_bit(shifter_operand, 31);
			if(rightBit != leftBit && rightBit != get_bit(res, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			break;
		case 0b0011://RSB
			res = shifter_operand - valueRn;
			cFlag = carryFlagSub(shifter_operand, valueRn);
			leftBit = get_bit(shifter_operand, 31);
			rightBit = get_bit(valueRn, 31);
			if(rightBit != leftBit && rightBit != get_bit(res, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			break;
		case 0b0100://ADD
			res = shifter_operand + valueRn;
			cFlag = carryFlagAdd(shifter_operand, valueRn);
			leftBit = get_bit(valueRn, 31);
			rightBit = get_bit(shifter_operand, 31);
			if(rightBit == leftBit && rightBit != get_bit(res, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			break;
		case 0b0101://ADC
			res = shifter_operand + valueRn + cFlag;
			oldcFlag = cFlag;
			cFlag = carryFlagAdd(shifter_operand, valueRn);
			if(cFlag == 0 && oldcFlag == 1) {
				cFlag = carryFlagAdd(shifter_operand + valueRn, 1);
			}
			leftBit = get_bit(valueRn, 31);
			rightBit = get_bit(shifter_operand, 31);
			if(rightBit == leftBit && rightBit != get_bit(shifter_operand + valueRn, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			if(vFlag == 0 && oldcFlag == 1) {
				leftBit = get_bit(valueRn + shifter_operand, 31);
				rightBit = 0;
				if(rightBit == leftBit && rightBit != get_bit(res, 31)) {
					vFlag = 1;
				}
				else {
					vFlag = 0;
				}
			}
			break;
		case 0b0110://SBC
			res = valueRn - shifter_operand - (~cFlag);
			oldcFlag = cFlag;
			cFlag = carryFlagSub(valueRn, shifter_operand);
			if(cFlag == 0 && oldcFlag == 0) {
				cFlag = carryFlagSub(valueRn - shifter_operand, 1);
			}
			leftBit = get_bit(valueRn, 31);
			rightBit = get_bit(shifter_operand, 31);
			if(rightBit != leftBit && rightBit != get_bit(valueRn - shifter_operand, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			if(vFlag == 0 && oldcFlag == 0) {
				leftBit = get_bit(valueRn - shifter_operand, 31);
				rightBit = 0;
				if(rightBit != leftBit && rightBit != get_bit(res, 31)) {
					vFlag = 1;
				}
				else {
					vFlag = 0;
				}
			}
			break;
		case 0b0111://RSC
			res = shifter_operand - valueRn - (~cFlag);
			oldcFlag = cFlag;
			cFlag = carryFlagSub(shifter_operand, valueRn);
			if(cFlag == 0 && oldcFlag == 0) {
				cFlag = carryFlagSub(shifter_operand - valueRn, 1);
			}
			leftBit = get_bit(shifter_operand, 31);
			rightBit = get_bit(valueRn, 31);
			if(rightBit != leftBit && rightBit != get_bit(shifter_operand - valueRn, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			if(vFlag == 0 && oldcFlag == 0) {
				leftBit = get_bit(shifter_operand - valueRn, 31);
				rightBit = 0;
				if(rightBit != leftBit && rightBit != get_bit(res, 31)) {
					vFlag = 1;
				}
				else {
					vFlag = 0;
				}
			}
			break;
		case 0b1000://TST
			res = valueRn && shifter_operand;
			updateCPSR = 1;
			write_register = 0;
			break;
		case 0b1001://TEQ
			res = (valueRn || shifter_operand) && !(valueRn && shifter_operand);
			updateCPSR = 1;
			write_register = 0;
			break;
		case 0b1010://CMP
			res = valueRn - shifter_operand;
			updateCPSR = 1;
			write_register = 0;
			cFlag = carryFlagSub(valueRn, shifter_operand);
			leftBit = get_bit(valueRn, 31);
			rightBit = get_bit(shifter_operand, 31);
			if(rightBit != leftBit && rightBit != get_bit(res, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			break;
		case 0b1011://CMN
			res = valueRn + shifter_operand;
			updateCPSR = 1;
			write_register = 0;
			cFlag = carryFlagAdd(shifter_operand, valueRn);
			leftBit = get_bit(valueRn, 31);
			rightBit = get_bit(shifter_operand, 31);
			if(rightBit == leftBit && rightBit != get_bit(res, 31)) {
				vFlag = 1;
			}
			else {
				vFlag = 0;
			}
			break;
		case 0b1100://ORR
			res = valueRn || shifter_operand;
			break;
		case 0b1101://MOV
			res = shifter_operand;
			break;
		case 0b1110://BIC
			res = valueRn && (~shifter_operand);
			break;
		case 0b1111://MVN
			res = ~shifter_operand;
			break;
		default:
			return UNDEFINED_INSTRUCTION;
	}
	
	zFlag = res == 0;
	nFlag = get_bit(res, 31);
	
	if(write_register) 
		arm_write_register(p, rd, res);
	
	if(updateCPSR) {
		if(zFlag)
			valueCpsr = set_bit(valueCpsr, 30);
		else
			valueCpsr = clr_bit(valueCpsr, 30);
		if(nFlag)
			valueCpsr = set_bit(valueCpsr, 31);
		else
			valueCpsr = clr_bit(valueCpsr, 31);
		if(cFlag)
			valueCpsr = set_bit(valueCpsr, 29);
		else
			valueCpsr = clr_bit(valueCpsr, 29);
		if(vFlag)
			valueCpsr = set_bit(valueCpsr, 28);
		else
			valueCpsr = clr_bit(valueCpsr, 28);
		
		arm_write_cpsr(p, valueCpsr);
	}
	
	return 0;
}

/* Decoding functions for different classes of instructions */
int arm_data_processing_shift(arm_core p, uint32_t ins) {
	int opcode = get_bits(ins, 24, 21);
	int rn = get_bits(ins, 19, 16);
	int rd = get_bits(ins, 15, 12);
	int updateCPSR = get_bit(ins, 20) && rd != 15;
	
	int rm = get_bits(ins, 3, 0);
	uint32_t shifter_operand = arm_read_register(p, rm);
	uint32_t valueRn = arm_read_register(p, rn);
	
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
	
    return executeInst(opcode, p, rd, valueRn, shifter_operand, updateCPSR);
}

int arm_data_processing_immediate_msr(arm_core p, uint32_t ins) {
    int opcode = get_bits(ins, 24, 21);
	int rn = get_bits(ins, 19, 16);
	int rd = get_bits(ins, 15, 12);
	int updateCPSR = get_bit(ins, 20) && rd != 15;
	int rotate_imm = get_bits(ins, 11, 8);
	uint32_t shifter_operand = get_bits(ins, 7, 0);
	
	shifter_operand = ror(shifter_operand, 2*rotate_imm);
	uint32_t valueRn = arm_read_register(p, rn);
	
    return executeInst(opcode, p, rd, valueRn, shifter_operand, updateCPSR);
}
