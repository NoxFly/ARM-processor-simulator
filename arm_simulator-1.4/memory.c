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
#include <stdlib.h>
#include "memory.h"
#include "util.h"

struct memory_data {
    uint8_t* values;
    size_t size;
    int is_big_endian;
};

memory memory_create(size_t size, int is_big_endian) {
    memory mem=malloc(sizeof(struct memory_data));
    
    mem->size=size;
    mem->is_big_endian=is_big_endian;
    mem->values=malloc(sizeof(uint8_t)*size);
    
    return mem;
}

size_t memory_get_size(memory mem) {
    return mem->size;
}

void memory_destroy(memory mem) {
    free(mem->values);
    free(mem);
}

int memory_read_byte(memory mem, uint32_t address, uint8_t *value) {
    if(address < 0 || address >= mem->size){
        return -1;
    }
    *value = mem->values[address];
    return 0;
}

int memory_read_half(memory mem, uint32_t address, uint16_t *value) {
    if(address < 0 || address+1 >= mem->size){
        return -1;
    }
    if(mem->is_big_endian){
        *value = mem->values[address]<<8;
        *value = *value | mem->values[address + 1];
    }
    else{
        *value = mem->values[address + 1] << 8;
        *value = *value | mem->values[address];
    }
    return 0;
}

int memory_read_word(memory mem, uint32_t address, uint32_t *value) {
    if(address < 0 || address+3 >= mem->size){
        return -1;
    }    
    if(mem->is_big_endian){
        *value = mem->values[address]<<24 | mem->values[address + 1] << 16 | mem->values[address + 2] << 8 | mem->values[address+3];
    }
    else
    {
        *value = mem->values[address + 3]<<24 | mem->values[address + 2] << 16 | mem->values[address + 1] << 8 | mem->values[address];
    }
    return 0;
    
}

int memory_write_byte(memory mem, uint32_t address, uint8_t value) {
    if(address < 0 || address >= mem->size){
        return -1;
    }
    mem->values[address] = value;
    return 0;
}

int memory_write_half(memory mem, uint32_t address, uint16_t value) {
    if(address < 0 || address+1 >= mem->size){
        return -1;
    }
    if(mem->is_big_endian){
        mem->values[address] = (uint8_t) (value >> 8);
        mem->values[address+1] = (uint8_t) value;
    }
    else{
        mem->values[address] = (uint8_t) value;
        mem->values[address+1] = (uint8_t) (value >> 8);
    }
    return 0;
}

int memory_write_word(memory mem, uint32_t address, uint32_t value) {
    if(address < 0 || address+3 >= mem->size){
        return -1;
    }
    if(mem->is_big_endian){
        mem->values[address] = (uint8_t) (value >> 24);
        mem->values[address+1] = (uint8_t) (value >> 16);
        mem->values[address+2] = (uint8_t) (value >> 8);
        mem->values[address+3] = (uint8_t) value;
    }
    else{
        mem->values[address+3] = (uint8_t) (value >> 24);
        mem->values[address+2] = (uint8_t) (value >> 16);
        mem->values[address+1] = (uint8_t) (value >> 8);
        mem->values[address] = (uint8_t) value;
    }
    return 0;
}
