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
#include "arm_load_store.h"
#include "arm_exception.h"
#include "arm_constants.h"
#include "util.h"
#include "debug.h"

int arm_load_store(arm_core p, uint32_t ins) {
    uint8_t byte;
 	uint32_t word;

    uint32_t rm = get_bits(ins, 3, 0);
    uint32_t offset = get_bits(ins, 11, 0);
    uint8_t rd = get_bits(ins, 15, 12);
    uint32_t rn = get_bits(ins, 19, 16);
    uint8_t L = get_bit(ins, 20);
    uint8_t W = get_bit(ins, 21);
    uint8_t B = get_bit(ins, 22);
    uint8_t U = get_bit(ins, 23);
    uint8_t P = get_bit(ins, 24);
    uint8_t I = get_bit(ins, 25);
    uint8_t id = get_bits(ins, 27, 25);
    uint8_t cond = get_bits(ins, 31, 28);
    rn = arm_read_register(p, rn);
    uint32_t address;
    // Part 1 : Immidiate offset/index
    if(id==2){ 
        // Immediate offset
        if(W! && P){
            if(U){ 
                rn += rn + offset;
            }
            else{
                rn += rn - offset;
            }
        }
    // Immediate pre-indexed
    else if (P && W) {
			if(U){ //
                rn += rn + offset;
            }
            else{
                rn += rn - offset;
            }
		}
    // Immediate post-indexed
	else if (P! && W!) {
		address = rn;
			if (U){
                rn = rn + offset;
            }	
			else {                    
                rn = rn - offset;
            }
					
		}
	}

    //Part 2 : Register offset/index
    if(id==3 && get_bits(ins, 11, 4) == 0){
    rm = arm_read_register(p, rm);

        // Register offset
        if(W! && P){
            if(U){ //
                address = rn + rm;
            }
            else{
                address = rn - rm;
            }
        }
        // Register pre-indexed
        else if (P && W) {
			if(U){ //
                address = rn + rm;
            }
            else{
                address = rn - rm;
            }
            rn = address;
		}
        // Immediate post-indexed
	    else if (P! && W!) {
		    address = rn;
		    	if (U){
                    rn += rm;
               }	
		    	else {                    
                 rn -= rm;
             }
					
		}
	}
    // Part 3 : Scaled register offset/index
	else if (id == 3 && get_bit(ins, 4)!) {
		uint8_t shift = get_bits(ins, 6, 5);
        uint8_t shift_imm = get_bits(ins, 11, 7);
		uint32_t index;
		rm = arm_read_register(p, rm);
		
		// Offset
		if (P && W!) {
			switch(shift) {
				case 0b00 : // LSL
                     index = rm << shift_imm; 
                break;

				case 0b01: // LSR 
					if (shift_imm) {
                        index = rm >> shift_imm;
                    }
					else { 
                        index = 0;
                    }
				break;

				case 0b10: // ASR
				if (shift_imm) { 
                     index = asr(rm, shift_imm);
					
				}
				else{
                   if (get_bit(rm, 31)) {
                        index = 0xFFFFFFFF;
                    }
					else{
                        index = 0;
                    } 
                }
				break;

				case 0b11:
					if (shift_imm){ // ROR  
						index = ror(rm, shift_imm);
                    }
					else { // RRX
                        index = (get_bit(arm_read_cpsr(p), C) << 31) | (rm >> 1); // C -> Carry Flag
                    }
				break;

				default: break;
			}
			if (U) {
                address = rn + index;
            }	
			else {
                address = rn - index;
            }
				
		}
		
		// Pre-indexed
		else if (P == 1 && W == 1) {
			switch(shift) {
				case 0b00 : // LSL
                     index = rm << shift_imm; 
                break;

				case 0b01: // LSR 
					if (shift_imm) {
                        index = rm >> shift_imm;
                    }
					else { 
                        index = 0;
                    }
				break;

				case 0b10: // ASR
				if (shift_imm) { 
                     index = asr(rm, shift_imm);
					
				}
				else{
                   if (get_bit(rm, 31)) {
                        index = 0xFFFFFFFF;
                    }
					else{
                        index = 0;
                    } 
                }
				break;

				case 0b11:
					if (shift_imm){ // ROR  
						index = ror(rm, shift_imm);
                    }
					else { // RRX
                        index = (get_bit(arm_read_cpsr(p), C) << 31) | (rm >> 1); // C -> Carry Flag
                    }
				break;

				default: break;
			}
		if (U) {
            address = rn + index;
        }
		else {
            address = rn - index;
        }
        rn = address;
	}

	 	// Post-indexed
	 	else if (P == 0 && W == 0) {
	 		address = rn;
            switch(shift) {
				case 0b00 : // LSL
                     index = rm << shift_imm; 
                break;

				case 0b01: // LSR 
					if (shift_imm) {
                        index = rm >> shift_imm;
                    }
					else { 
                        index = 0;
                    }
				break;

				case 0b10: // ASR
				if (shift_imm) { 
                     index = asr(rm, shift_imm);
					
				}
				else{
                   if (get_bit(rm, 31)) {
                        index = 0xFFFFFFFF;
                    }
					else{
                        index = 0;
                    } 
                }
				break;

				case 0b11:
					if (shift_imm){ // ROR  
						index = ror(rm, shift_imm);
                    }
					else { // RRX
                        index = (get_bit(arm_read_cpsr(p), C) << 31) | (rm >> 1); // C -> Carry Flag
                    }
				break;

				default: break;
			}
		    if (U) {
                rn += index;
            }
		    else {
                rn -= index;
            }
        }
 	}

 	if (L) { // Load
 		if (B) { // Byte
 			arm_read_byte(p, address, &byte);
 			arm_write_register(p, rd, (uint32_t) byte);
 		}
 		else { // Word
 			arm_read_word(p, address, &word);
 			arm_write_register(p, rd, word);
 		}
 	}
 	
 	else { // Store
 		if (B) { // Byte
 			byte = arm_read_register(p, rd);
 			arm_write_byte(p, address, byte);		
 		}
 		else { // Word
 			word = arm_read_register(p, rd);
 			arm_write_word(p, address, word);
 		}
 	}
    return 0;
}

int arm_load_store_multiple(arm_core p, uint32_t ins) {
       uint32_t i,rn;
       uint32_t cpsr = arm_read_cpsr(p);
       uint32_t register_list = get_bits(ins, 15, 0);
       uint8_t P = get_bit(ins, 24);
       uint8_t U = get_bit(ins, 23);
       uint8_t S = get_bit(ins, 22); // Don't care about this 
       uint8_t W = get_bit(ins, 21);
       uint8_t L = get_bit(ins, 20);
       uint8_t cond = get_bits(ins, 31, 28);
       rn = arm_read_register(p, get_bits(ins, 19, 16));

        uint32_t start_address, end_address;

	//  Increment after
	if (P! && U) {
		start_address = rn;
		end_address = rn + (number_set_bits_in(register_list) * 4) - 4;
			rn = rn + (number_set_bits_in(register_list) * 4);
	}
	// Increment before
	else if (P && U) {
		start_address = rn + 4;
		end_address = rn + (number_set_bits_in(register_list) * 4);
			rn = rn + (number_set_bits_in(register_list) * 4);
	}
	// Decrement after
	else if (P! && U!) {
		start_address = rn - (number_set_bits_in(register_list) * 4) + 4;
		end_address = rn;
			rn = rn - (number_set_bits_in(register_list) * 4);
	}
	// Decrement before
	else if (P && U!) {
		start_address = rn - (number_set_bits_in(register_list) * 4);
		end_address = rn - 4;
			rn = rn - (number_set_bits_in(register_list) * 4);
	}
	uint32_t address;
	
	if (L) { // LDM
		uint32_t value;

			address = start_address;
			for (i = 0; i <= 14; i++) {
				if (get_bit(register_list, i) == 1) {
					arm_read_word(p, address, &value);
					arm_write_register(p, i, value);
					address = address + 4;
				}
			}
			if (get_bit(register_list, 15) == 1) {
				value = arm_read_word(p, address, &value);
				arm_write_register(p, 15, value & 0xFFFFFFFE);
				address = address + 4;
			}
			
	}
	
	else { // STM
			address = start_address;
			for (i = 0; i <= 15; i++) {
				if (get_bit(register_list, i) == 1) {
					arm_write_word(p, address, arm_read_register(p, i));
					address = address + 4;
				}
			}
		}
	}
    return 0;
}

int arm_coprocessor_load_store(arm_core p, uint32_t ins) {
    /* Not implemented */
    return UNDEFINED_INSTRUCTION;
}
