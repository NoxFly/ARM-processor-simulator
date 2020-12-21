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
#include <stdio.h>
#include <assert.h>
#include "gdb_protocol.h"
#include "debug.h"
#include "csapp.h"
#include "util.h"
#include "arm_core.h"
#include "arm_constants.h"
#include "trace.h"

#define MAX_PACKET_SIZE 1024

struct gdb_protocol_data {
    arm_core arm;
    memory mem;
    int target_exception;
    int fd;
    pthread_mutex_t *lock;
    char packet[MAX_PACKET_SIZE];
    int len;
    char *buffer;
};

typedef void (*gdb_handler_t)(gdb_protocol_data_t, char *);
static gdb_handler_t handler[256];

static void gdb_send_ack(gdb_protocol_data_t gdb) {
    Rio_writen(gdb->fd, "+", 1);
}

static void gdb_send_buffer(gdb_protocol_data_t gdb) {
    unsigned char check=0;
    int i;

    gdb->packet[0] = '$';
    i = 1;
    while (gdb->packet[i] != '\0') {
        check += gdb->packet[i];
        i++;
    }
    gdb->packet[i++] = '#';
    sprintf(gdb->packet+i, "%02x", check);
    i += 2;
    gdb->packet[i] = '\0';
    gdb->len = i;
    gdb_transmit_packet(gdb);
}

static void gdb_send_data(gdb_protocol_data_t gdb, char *data) {
    strcpy(gdb->buffer, data);
    gdb_send_buffer(gdb);
}

/* Read and write to/from a string of bytes (in hexadecimal) in local byte
 * order */
static uint32_t read_uint32(char *data) {
    unsigned int i, step, value;
    int target_big_endian;
    union {
        unsigned char bytes[4];
        uint32_t integer;
    } mem;

    #ifdef BIG_ENDIAN_SIMULATOR
        target_big_endian = 1;
    #else
        target_big_endian = 0;
    #endif

    if (is_big_endian() == target_big_endian) {
        i = 0;
        step = 1;
    } else {
        i = 3;
        step = -1;
    }

    for (; (i<4) && (i>=0); i+=step) {
        sscanf(data, "%02x", &value);
        mem.bytes[i] = value;
        data += 2;
    }
    return mem.integer;
}

static void write_uint32(char *data, uint32_t value) {
    unsigned int i, step;
    int target_big_endian;
    union {
        unsigned char bytes[4];
        uint32_t integer;
    } mem;

    mem.integer = value;
    #ifdef BIG_ENDIAN_SIMULATOR
        target_big_endian = 1;
    #else
        target_big_endian = 0;
    #endif

    if (is_big_endian() == target_big_endian) {
        i = 0;
        step = 1;
    } else {
        i = 3;
        step = -1;
    }

    for (; (i<4) && (i>=0); i+=step) {
        sprintf(data, "%02x", mem.bytes[i]);
        data += 2;
    }
    *data = '\0';
}

/* Handling of exception raised in target */
void gdb_send_stop_reason(gdb_protocol_data_t gdb) {
    switch (gdb->target_exception) {
      case UNDEFINED_INSTRUCTION:
        gdb_send_data(gdb, "S04");
        break;
      case PREFETCH_ABORT:
      case DATA_ABORT:
        gdb_send_data(gdb, "S10");
        break;
      default:
        gdb_send_data(gdb, "S05");
    }
}

/* GDB Protocol commands handlers */

static void cont(gdb_protocol_data_t gdb, char *data) {
    /* When the simulator doesn't implement breakpoints (as it is the case
     * here), gdb implements soft breakpoints by placing an architecturally
     * undefined instruction at breakpoint position. Thus we implement the
     * continue command as a loop that waits for this instruction.
     */
    uint32_t instruction, r15;
    int end = 0;

    while (!end) {
        /* We read in anticipation the next instruction to handle our special
         * cases
         */
        trace_disable();
        r15 = arm_read_register(gdb->arm, 15) - 4;
        (void) arm_read_word(gdb->arm, r15, &instruction);
        trace_enable(); 
        switch (instruction & 0xFFF000F0) {
          case 0xE7F000F0:
            /* This is a breakpoint, we will not execute it because we don't
             * know whether exceptions are properly implemented or not.
             * At this point gdb should replace the offending instruction by
             * the original one. This is hack but should perform better than
             * other solution because of its few assumptions.
             */
            end = 1;
            break;
          default:
            gdb->target_exception = arm_step(gdb->arm);
            trace_arm_state(gdb->arm);
        }
    }

    gdb_send_stop_reason(gdb);
}

static void kill_request(gdb_protocol_data_t gdb, char *data) {
    shutdown(gdb->fd, SHUT_WR);
}

static void query(gdb_protocol_data_t gdb, char *data) {
    if (strcmp(data, "Offsets") == 0)
        gdb_send_data(gdb, "Text=0;Data=0;Bss=0");
    else if (strncmp(data, "Supported", 9) == 0)
        gdb_send_data(gdb, "PacketSize=400");
    else if (strcmp(data, "TStatus") == 0)
        gdb_send_data(gdb, "T0;tnotrun:0");
    else if (strcmp(data, "Symbol::") == 0)
        gdb_send_data(gdb, "");
    else
        /* Unsupported query, giving an empty answer */
        gdb_send_data(gdb, "");
}

static void read_general_registers(gdb_protocol_data_t gdb, char *data) {
    char *position;
    int i, j;

    trace_disable();
    position = gdb->buffer;
    /* General register r0..r14 */
    for (i=0; i<15; i++) {
        write_uint32(position, arm_read_register(gdb->arm, i));
        position += 8;
    }
    /* Special case, the pc is one instruction in advance (before fetch) */
    write_uint32(position, arm_read_register(gdb->arm, i) - 4);
    position += 8;
    /* Floating point register f0..f7 */
    /* Not implemented */
    for (i=0; i<8; i++) {
        for (j=0; j<3; j++) {
            sprintf(position,"xxxxxxxx");
            position += 8;
        }
    }
    /* Status registers */
    /* fps not implemented */
    sprintf(position,"xxxxxxxx");
    position += 8;
    write_uint32(position, arm_read_cpsr(gdb->arm));
    trace_enable();
    gdb_send_buffer(gdb);
}

static void read_memory(gdb_protocol_data_t gdb, char *data) {
    unsigned int address, size;
    char *position;
    uint8_t value;

    sscanf(data,"%x,%x", &address, &size);
    position = gdb->buffer;
    position[0] = '\0';
    while (size-- && (memory_read_byte(gdb->mem, address++, &value) != -1)) {
        sprintf(position, "%02x", value);
        position += 2;
    }
    gdb_send_buffer(gdb);
}

static void read_register(gdb_protocol_data_t gdb, char *data) {
    unsigned int reg;
    reg = atoi(data);
    assert(reg < 16);
    trace_disable();
    write_uint32(gdb->buffer, arm_read_register(gdb->arm, reg) -
                              ((reg == 15) ? 4 : 0));
    trace_enable();
    gdb_send_buffer(gdb);
}

static void reason(gdb_protocol_data_t gdb, char *data) {
    gdb_send_stop_reason(gdb);
}

static void set_thread(gdb_protocol_data_t gdb, char *data) {
    char type = *data;
    int value = atoi(data+1);

    // No threads, so support standard selection for any or all threads
    if (((type == 'c') || (type == 'g')) && (value < 1) && (value > -2))
        gdb_send_data(gdb, "OK");
    else
        gdb_send_data(gdb, "E01");
}

static void step(gdb_protocol_data_t gdb, char *data) {
    gdb->target_exception = arm_step(gdb->arm);
    trace_arm_state(gdb->arm);
    gdb_send_stop_reason(gdb);
}

static void write_general_registers(gdb_protocol_data_t gdb, char *data) {
    uint32_t value;
    char *position;
    int i, j;

    trace_disable();
    position = data;
    /* General register r0..r15 */
    for (i=0; i<16; i++) {
        value = read_uint32(position);
        arm_write_register(gdb->arm, i, value);
        debug("r%02d = %08x   ", i, value);
        if (i % 4 == 3)
            debug_raw("\n");
        position += 8;
    }
    /* Floating point register f0..f7 */
    /* Not implemented */
    for (i=0; i<8; i++) {
        //printf("f%02d = ", i);
        for (j=0; j<3; j++) {
            value = read_uint32(position);
            //printf("%08x", value);
            position += 8;
        }
        //printf("   ");
        //if (i % 2 == 1)
            //printf("\n");
    }
    /* Status registers */
    /* fps not implemented */
    value = read_uint32(position);
    //printf("fps = %08x   ", value);
    position += 8;
    value = read_uint32(position);
    arm_write_cpsr(gdb->arm, value);
    debug("cpsr = %08x\n", value);
    trace_enable();

    gdb_send_data(gdb, "OK");
}

static void write_memory_binary(gdb_protocol_data_t gdb, char *data) {
    unsigned int address, size, i, write_ok;
    char *content;
    uint8_t value;

    sscanf(data,"%x,%x", &address, &size);
    content = index(data, ':') + 1;
    debug("Writing %d bytes at address %08x : ", size, address);
    write_ok = address < memory_get_size(gdb->mem);
    for (i=0; (i<size) && write_ok; i++) {
        if (*content == 0x7d) {
            content++;
            value = *content ^ (char) 0x20;
        } else {
            value = *content;
        }
        write_ok = memory_write_byte(gdb->mem, address++, value) == 0;
        if (i<32)
            debug_raw("%02x", value);
        content++;
    }
    debug_raw("...\n");
    if (write_ok)
        gdb_send_data(gdb, "OK");
    else
        gdb_send_data(gdb, "E02");
}

static void write_register(gdb_protocol_data_t gdb, char *data) {
    unsigned int reg, value;

    sscanf(data,"%x", &reg);
    data = index(data, '=') + 1;
    value = read_uint32(data);
    assert(reg < 16);
    trace_disable();
    arm_write_register(gdb->arm, reg, value);
    trace_enable();
    debug("Writing %d to register %d\n", value, reg);
    gdb_send_data(gdb, "OK");
}

/* End of GDB Protocol commands handlers */

gdb_protocol_data_t gdb_init_data(arm_core arm, memory mem, int fd,
                                  pthread_mutex_t *lock) {
    gdb_protocol_data_t gdb;

    gdb = malloc(sizeof(struct gdb_protocol_data));
    if (gdb) {
        gdb->arm = arm;
        gdb->mem = mem;
        gdb->target_exception = 0;
        gdb->fd = fd;
        gdb->lock = lock;
        gdb->len = 0;
        gdb->buffer = gdb->packet+1;
    }
    return gdb;
}

void gdb_init() {
    int i;

    debug("gdb protocol handlers initialization\n");
    for (i=0; i<256; i++)
        handler[i] = NULL;
    handler['c'] = cont;
    handler['k'] = kill_request;
    handler['q'] = query;
    handler['g'] = read_general_registers;
    handler['m'] = read_memory;
    handler['p'] = read_register;
    handler['?'] = reason;
    handler['H'] = set_thread;
    handler['s'] = step;
    handler['G'] = write_general_registers;
    handler['X'] = write_memory_binary;
    handler['P'] = write_register;
}

void gdb_require_retransmission(gdb_protocol_data_t gdb) {
    Rio_writen(gdb->fd, "-", 1);
}

void gdb_packet_analysis(gdb_protocol_data_t gdb, char *packet, int length) {
    int i;
    unsigned char check=0;
    unsigned int given;
    unsigned char index;

    for (i=1; i<length-3; i++)
        check += packet[i];
    sscanf(packet+i+1, "%x", &given);
    debug("Received packet : ");
    debug_raw_binary(packet, min(16, strlen(packet)));
    if (check == given) {
        debug_raw(", checksum ok\n");
        gdb_send_ack(gdb);
    } else {
        debug_raw(", checksum failed, expected %02x got %02x\n", given, check);
        debug("Requiring retransmission\n");
        gdb_require_retransmission(gdb);
        return;
    }
    packet[i] = '\0';
    index = packet[1];
    if (handler[index]) {
        pthread_mutex_lock(gdb->lock);
        handler[index](gdb, packet+2);
        pthread_mutex_unlock(gdb->lock);
    } else {
        debug("Unsupported request, sending empty answer\n");
        gdb_send_data(gdb, "");
    }
}

void gdb_transmit_packet(gdb_protocol_data_t gdb) {
    debug("Transmitting packet: %s\n", gdb->packet);
    Rio_writen(gdb->fd, gdb->packet, gdb->len);
}
