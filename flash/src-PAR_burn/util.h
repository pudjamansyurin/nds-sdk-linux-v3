/*******************************************************************************
 * File name: util.h
 * Description: Flash Burner
 * Author: Shiva
 * Revision history:
 * 2011/05/23	Create this file.
 * ****************************************************************************/
#ifndef __UTIL__
#define __UTIL__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <getopt.h>

#ifdef __MINGW32__
#include <w32api.h>
#define WINVER WindowsXP
#include <winsock2.h>
#include <ws2tcpip.h>
#undef socklen_t
#define socklen_t int
#define sleep(t) _sleep(t * 1000)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#ifdef __MINGW32__
#define    SEND(SOCKET, DATA, SIZE) send(SOCKET, DATA, SIZE, 0)
#define    RECV(SOCKET, DATA, SIZE) recv(SOCKET, DATA, SIZE, 0)
#else
#define    SEND(SOCKET, DATA, SIZE) write(SOCKET, DATA, SIZE)
#define    RECV(SOCKET, DATA, SIZE) read(SOCKET, DATA, SIZE)
#endif

#define MAX_MULTI_WRITE_PAIR  256

// PIPE MAX_SIZE = 65536 Bytes
#define MAX_SOCKET_BUF        65536



// Command code
#define WRITE_WORD   (0x1A)
#define READ_WORD    (0x1B)
#define WRITE_BYTE   (0x2A)
#define READ_BYTE    (0x2B)
#define WRITE_HALF   (0x4A)
#define READ_HALF    (0x4B)
#define FAST_READ    (0x1C)
#define FAST_READ_BYTE      (0x1E)
#define FAST_WRITE   (0x20)
#define FAST_WRITE_BYTE     (0x21)
#define BURNER_QUIT  (0x04)
#define MULTIPLE_WRITE_WORD (0x5A)
#define MULTIPLE_WRITE_HALF (0x5B)
#define MULTIPLE_WRITE_BYTE (0x5C)
#define MULTIPLE_READ_WORD  (0x5D)
#define MULTIPLE_READ_HALF  (0x5E)
#define MULTIPLE_READ_BYTE  (0x5F)
#define RESET_TARGET        (0x3A)
#define RESET_HOLD          (0x3B)
#define RESET_AICE          (0x3C)
#define HOLD_CORE           (0x1D)
#define BURNER_SELECT_CORE  (0x05)
#define BURNER_SELECT_TARGET  (0x08)
#define MONITOR_CMD         (0x06)
#define READ_EDM_SR         (0x60)
#define WRITE_EDM_SR        (0x61)
#define READ_EDM_JDP        (0x6E)
#define WRITE_EDM_JDP       (0x6F)
#define BURNER_DEBUG        (0x07)
#define READ_REG            (0x70)
#define WRITE_REG           (0x71)

enum nds_memory_select {
        NDS_MEMORY_SELECT_BUS = 0,
        NDS_MEMORY_SELECT_ILM,
        NDS_MEMORY_SELECT_DLM,
};

/* EDM misc registers */
enum nds_edm_misc_reg {
        NDS_EDM_MISC_DIMIR = 0x0,
        NDS_EDM_MISC_SBAR,
        NDS_EDM_MISC_EDM_CMDR,
        NDS_EDM_MISC_DBGER,
        NDS_EDM_MISC_ACC_CTL,
        NDS_EDM_MISC_EDM_PROBE,
        NDS_EDM_MISC_GEN_PORT0,
        NDS_EDM_MISC_GEN_PORT1,
};

/* JDP Instructions */
#define JDP_READ	  0x00
#define JDP_WRITE	  0x80

#define ACCESS_DIM          0x01        //4'b0001
#define ACCESS_DBG_SR       0x02        //4'b0010
#define ACCESS_DTR          0x03        //4'b0011
#define ACCESS_MEM_W        0x04        //4'b0100
#define ACCESS_MISC_REG     0x05        //4'b0101
#define FAST_ACCESS_MEM     0x06        //4'b0110
#define GET_DBG_EVENT       0x07        //4'b0111
#define EXECUTE             0x08        //4'b1000
#define IDCODE              0x09        //4'b1001
#define ACCESS_MEM_H        0x0A        //4'b1010
#define ACCESS_MEM_B        0x0B        //4'b1011
#define BYPASS              0x0F        //4'b1111

/* JDP Read Instructions */
#define JDP_R_DIM       (JDP_READ | ACCESS_DIM)
#define JDP_R_DBG_SR    (JDP_READ | ACCESS_DBG_SR)
#define JDP_R_DTR       (JDP_READ | ACCESS_DTR)
#define JDP_R_MEM_W     (JDP_READ | ACCESS_MEM_W)
#define JDP_R_MISC_REG  (JDP_READ | ACCESS_MISC_REG)
#define JDP_R_FAST_MEM  (JDP_READ | FAST_ACCESS_MEM)
#define JDP_R_DBG_EVENT (JDP_READ | GET_DBG_EVENT)
#define JDP_R_IDCODE    (JDP_READ | IDCODE)
#define JDP_R_MEM_H     (JDP_READ | ACCESS_MEM_H)
#define JDP_R_MEM_B     (JDP_READ | ACCESS_MEM_B)


/* JDP Write Instructions */
#define JDP_W_DIM       (JDP_WRITE | ACCESS_DIM)
#define JDP_W_DBG_SR    (JDP_WRITE | ACCESS_DBG_SR)
#define JDP_W_DTR       (JDP_WRITE | ACCESS_DTR)
#define JDP_W_MEM_W     (JDP_WRITE | ACCESS_MEM_W)
#define JDP_W_MISC_REG  (JDP_WRITE | ACCESS_MISC_REG)
#define JDP_W_FAST_MEM  (JDP_WRITE | FAST_ACCESS_MEM)
#define JDP_W_EXECUTE   (JDP_WRITE | EXECUTE)
#define JDP_W_MEM_H     (JDP_WRITE | ACCESS_MEM_H)
#define JDP_W_MEM_B     (JDP_WRITE | ACCESS_MEM_B)

#define NDS_EDM_SR_EDM_CFG  0x28

#define set_u32(buffer, value) h_u32_to_le((uint8_t *)buffer, value)
#define get_u32(buffer) le_to_h_u32((const uint8_t *)buffer)

/* type define */
typedef unsigned long long      UINT64;
typedef long long               INT64;
typedef unsigned int            UINT32;
typedef int                     INT32;
typedef unsigned short          UINT16;
typedef short                   INT16;
typedef unsigned char           UINT8;
//typedef char                    INT8;


/* read/write API */
int outw(unsigned int address, unsigned int data);
int outh(unsigned int address, unsigned short data);
int outb(unsigned int address, unsigned char data);
int out_io(unsigned int address, unsigned int size, char * buffer);
char inb(unsigned int address);
short inh(unsigned int address);
int inw(unsigned int address);
int fastin(unsigned int address, unsigned int size, char * buffer);
int fastbytein(unsigned int address, unsigned int size, char * buffer, char if_const_addr);
int fastout(unsigned int address, unsigned int size, char * buffer);
int fastbyteout(unsigned int address, unsigned int size, char * buffer, char if_const_addr);
int multiout_w(unsigned int *address, unsigned int *data, const unsigned int number_of_pairs);
int multiout_h(unsigned int *address, unsigned short *data, const unsigned int number_of_pairs);
int multiout_b(unsigned int *address, unsigned char *data, const unsigned int number_of_pairs);
int multiin_w(unsigned int *address, unsigned int *data, const unsigned int number_of_pairs);
int multiin_h(unsigned int *address, unsigned short *data, const unsigned int number_of_pairs);
int multiin_b(unsigned int *address, unsigned char *data, const unsigned int number_of_pairs);
void close_connection (void);
void terminate(void);
void handle_int(int signo);
int send_cmd(char cmd);
int send_targetname(char* target_name, unsigned int size);
int send_coreid(int coreid);
int send_monitor_cmd( char* monitor_cmd, unsigned int size, char** ret_data );
int mem_select_mode(unsigned int mem_mode);
int read_edm_cfg(unsigned int *pread_data);
int inr(unsigned int num);
int outr(unsigned int num, unsigned int data);

void initial_socket(const char *host, unsigned int port);
/* get image from file */
unsigned char* get_image(FILE* image, unsigned int* size);
void timeval_diff (struct timeval *a_result, struct timeval *a_timeval_begin, struct timeval *a_timeval_end);


static inline void h_u32_to_le(uint8_t* buf, int val)
{
        buf[3] = (uint8_t) (val >> 24);
        buf[2] = (uint8_t) (val >> 16);
        buf[1] = (uint8_t) (val >> 8);
        buf[0] = (uint8_t) (val >> 0);
}

static inline uint32_t le_to_h_u32(const uint8_t* buf)
{
        return (uint32_t)(buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24);
}


#endif

