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

#include <string.h>

// Calcul de la dernière retenue ou du dernier emprunt si soustraction (flag C)
int getCarryFlag(uint32_t left, uint32_t right, char operand) { 
	int cFlag = 0;

	for(int i = 0 ; i < 31 ; i++) {
		int leftBit = get_bit(left, i);
		int rightBit = get_bit(right, i);

		int res = leftBit + (operand=='+' ? rightBit+cFlag : -rightBit-cFlag);

		if(operand == '+' && res > 1) {
			cFlag = 1;
			res -= 2;
		} else if(operand == '-' && (res == 2 || res == -1)) {
			cFlag = 1;
			res = res == 2 ? 0 : 1;
		} else {
			cFlag = 0;
		}
	}
	
	return cFlag;
}

int getCarryFlagAdd(uint32_t left, uint32_t right) {
	return getCarryFlag(left, right, '+');
}

int getCarryFlagSub(uint32_t left, uint32_t right) {
	return getCarryFlag(left, right, '-');
}

//Calcul du flag V
int getOverflowFlag(int res, int a, int b, char operand) {
	int leftBit = get_bit(a, 31);
	int rightBit = get_bit(b, 31);

	return
		operand == '+' ?
			rightBit == leftBit && rightBit != get_bit(res, 31) :
		operand == '-' ?
			rightBit != leftBit && rightBit != get_bit(res, 31) :
		0;
}

int getOverflowFlagAdd(int res, int a, int b) {
	return getOverflowFlag(res, a, b, '+');
}

int getOverflowFlagSub(int res, int a, int b) {
	return getOverflowFlag(res, a, b, '-');
}

// Mise à jour des variables pour les instructions qui modifient uniquement les flags ZNCV
void updateForTest(int* updateCPSR, int* write_register) {
	*updateCPSR = 1;
	*write_register = 0;
}

// Exécution de l'instruction
int executeInst(int opcode, arm_core p, int rd, uint32_t valueRn, uint32_t shifter_operand, int updateCPSR) {
	int valueCpsr = arm_read_cpsr(p);
	int zFlag = get_bit(valueCpsr, Z);
	int nFlag = get_bit(valueCpsr, N);
	int cFlag = get_bit(valueCpsr, C);
	int vFlag = get_bit(valueCpsr, V);
	uint32_t res;
	int write_register = 1;
	int oldcFlag;
	
	switch(opcode) {
		case 0b0000: // AND
			res = valueRn && shifter_operand;
			break;
		case 0b0001: // EOR
			res = (valueRn || shifter_operand) && !(valueRn && shifter_operand);
			break;
		case 0b0010: // SUB
			res = valueRn - shifter_operand;
			cFlag = getCarryFlagSub(valueRn, shifter_operand);
			vFlag = getOverflowFlagSub(res, valueRn, shifter_operand);
			break;
		case 0b0011: // RSB
			res = shifter_operand - valueRn;
			cFlag = getCarryFlagSub(shifter_operand, valueRn);
			vFlag = getOverflowFlagSub(res, shifter_operand, valueRn);
			break;
		case 0b0100: // ADD
			res = shifter_operand + valueRn;
			cFlag = getCarryFlagAdd(shifter_operand, valueRn);
			vFlag = getOverflowFlagAdd(res, shifter_operand, valueRn);
			break;
		case 0b0101: // ADC
			res = shifter_operand + valueRn + cFlag;
			oldcFlag = cFlag;
			cFlag = getCarryFlagAdd(shifter_operand, valueRn);
			if(!cFlag && oldcFlag) {
				cFlag = getCarryFlagAdd(shifter_operand + valueRn, 1);
			}
			vFlag = getOverflowFlagAdd(shifter_operand + valueRn, shifter_operand, valueRn);
			if(!vFlag && oldcFlag) {
				vFlag = getOverflowFlagAdd(res, valueRn + shifter_operand, 1);
			}
			break;
		case 0b0110: // SBC
			res = valueRn - shifter_operand - (~cFlag);
			oldcFlag = cFlag;
			cFlag = getCarryFlagSub(valueRn, shifter_operand);
			if(!cFlag && !oldcFlag) {
				cFlag = getCarryFlagSub(valueRn - shifter_operand, 1);
			}
			vFlag = getOverflowFlagSub(valueRn - shifter_operand, valueRn, shifter_operand);
			if(!vFlag && !oldcFlag) {
				vFlag = getOverflowFlagSub(res, valueRn - shifter_operand, 1);
			}
			break;
		case 0b0111: // RSC
			res = shifter_operand - valueRn - (~cFlag);
			oldcFlag = cFlag;
			cFlag = getCarryFlagSub(shifter_operand, valueRn);
			if(!cFlag && !oldcFlag) {
				cFlag = getCarryFlagSub(shifter_operand - valueRn, 1);
			}
			vFlag = getOverflowFlagSub(shifter_operand - valueRn, shifter_operand, valueRn);
			if(!vFlag && !oldcFlag) {
				vFlag = getOverflowFlagSub(res, shifter_operand - valueRn, 1);
			}
			break;
		case 0b1000: // TST
			res = valueRn && shifter_operand;
			updateForTest(&updateCPSR, &write_register); // On modifie seulement les flags ZNCV
			break;
		case 0b1001: // TEQ
			res = (valueRn || shifter_operand) && !(valueRn && shifter_operand);
			updateForTest(&updateCPSR, &write_register);
			break;
		case 0b1010: // CMP
			res = valueRn - shifter_operand;
			updateForTest(&updateCPSR, &write_register);
			cFlag = getCarryFlagSub(valueRn, shifter_operand);
			vFlag = getOverflowFlagSub(res, valueRn, shifter_operand);
			break;
		case 0b1011: // CMN
			res = valueRn + shifter_operand;
			updateForTest(&updateCPSR, &write_register);
			cFlag = getCarryFlagAdd(shifter_operand, valueRn);
			vFlag = getOverflowFlagAdd(res, valueRn, shifter_operand);
			break;
		case 0b1100: // ORR
			res = valueRn || shifter_operand;
			break;
		case 0b1101: // MOV
			res = shifter_operand;
			break;
		case 0b1110: // BIC
			res = valueRn && (~shifter_operand);
			break;
		case 0b1111: // MVN
			res = ~shifter_operand;
			break;
		default:
			return UNDEFINED_INSTRUCTION;
	}
	
	zFlag = res == 0;
	nFlag = get_bit(res, 31);
	
	if(write_register) 
		arm_write_register(p, rd, res);
	
	int t[4][2] = { { zFlag, Z }, { nFlag, N }, { cFlag, C }, { vFlag, V } };

	if(updateCPSR) {
		if(write_register && rd == 15) { // voir doc ARM A4-5 | write_register permet de vérifier qu'on est bien dans une instruction qui ne modifie pas seulement les flags ZNCV
			if(arm_current_mode_has_spsr(p))
				arm_write_cpsr(p, arm_read_spsr(p));
		}
		else {
			for(int i=0; i < 4; i++) {
				valueCpsr = t[i][0]? set_bit(valueCpsr, t[i][1]) : clr_bit(valueCpsr, t[i][1]);
			}

			arm_write_cpsr(p, valueCpsr);
		}
	}
	
	return 0;
}

// On effectue l'opération lsl, lsr, asr ou ror
int getShifterOperand(int so, int si, char* operand) {
	char* operands[4] = { "lsl", "lsr", "asr", "ror" };
	
	int k = -1;

	for(int i=0; i < 4; i++) {
		if(strcmp(operands[i], operand) == 0) {
			k = i;
		}
	}

	if(si > 0) {
		switch(k) {
			case 0: so = so << si; break;
			case 1: so = so >> si; break;
			case 2: so = asr(so, si); break;
			case 3: so = ror(so, si);
		}
	}

	return so;
}

int getShifterOperandImmediate(uint32_t ins, int so, char* operand) {
	return getShifterOperand(so, get_bits(ins, 11, 7), operand);
}

int getShifterOperandRegister(arm_core p, uint32_t ins, int so, char* operand) {
	return getShifterOperand(so, arm_read_register(p, get_bits(ins, 11, 8)), operand);
}

/* Decoding functions for different classes of instructions */
int arm_data_processing_shift(arm_core p, uint32_t ins) {
	int opcode = get_bits(ins, 24, 21); // Opération à effectuer
	int rn = get_bits(ins, 19, 16);
	int rd = get_bits(ins, 15, 12);
	int updateCPSR = get_bit(ins, 20); // Doit-on mettre à jour les flags ZNCV ?
	
	int rm = get_bits(ins, 3, 0);
	uint32_t shifter_operand = arm_read_register(p, rm);
	uint32_t valueRn = arm_read_register(p, rn);
	
	if(rm == 15) { // voir doc ARM A5-8
		shifter_operand += 8;
	}
	else if(rn == 15) {
		shifter_operand = valueRn + 8;
	}

	for(int i=0; i < 7; i++) { // On effectue l'opération lsl, lsr, asr ou ror sur shifter_operand
		if(get_bits(ins, 6, 4) == i) {
			char* op = (i < 2)? "lsl" : (i < 4)? "lsr" : (i < 6)? "asr" : "ror";
			shifter_operand = (i%2 == 0)?
				getShifterOperandImmediate(ins, shifter_operand, op) :
				getShifterOperandRegister(p, ins, shifter_operand, op);
			break;
		}
	}
	
    return executeInst(opcode, p, rd, valueRn, shifter_operand, updateCPSR);
}

int arm_data_processing_immediate_msr(arm_core p, uint32_t ins) {
    int opcode = get_bits(ins, 24, 21); // Opération à effectuer
	int rn = get_bits(ins, 19, 16);
	int rd = get_bits(ins, 15, 12);
	int updateCPSR = get_bit(ins, 20); // Doit-on mettre à jour les flags ZNCV ?
	int rotate_imm = get_bits(ins, 11, 8);
	uint32_t shifter_operand = get_bits(ins, 7, 0);
	
	shifter_operand = ror(shifter_operand, 2*rotate_imm); // voir doc ARM A5-6
	uint32_t valueRn = arm_read_register(p, rn);
	
    return executeInst(opcode, p, rd, valueRn, shifter_operand, updateCPSR);
}
