/*******************************************************************************
 * File name: util.cpp
 * Description: Intel Flash burner
 * Author: Shiva
 * Revision history:
 * 2011/05/23	Create this file.
 * 2012/05/24   Add comments - Kai
 * ****************************************************************************/

#include "util.h"
#include <stdio.h>

#ifdef __MINGW32__
SOCKET sock;
#else
int sock;
#endif

extern FILE *pLogFile;                  /* log file */

#define TARGET_ERROR "Burning failed due to target errors."
#define CONNECT_ERROR "Burning failed due to protocol errors or command timeout. (Wrong Port/Service!?)"

/* send command to iceman
 * command code refer to util.h */
int send_cmd(char cmd)
{
        char send_data[2],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = cmd;

        retval = SEND(sock, send_data, 1);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == cmd) {
                if ( recv_data[1] == 0 ) {
                        return (int)(recv_data[1]);
                } else {
                        fprintf(pLogFile, "\n!! send_cmd: %s !!\n", TARGET_ERROR);
                        fflush(pLogFile);
                        close_connection ();
                        exit (0);
                }
        } else {
                fprintf(pLogFile, "\n!! send_cmd: %s !!\n", CONNECT_ERROR);
                fflush(pLogFile);
                close_connection ();
                exit (0);
        }
}

/* write word */
/* could use outw write word to target memory */
int outw(unsigned int address, unsigned int data)
{
        char send_data[10],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = WRITE_WORD;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);
        send_data[6] = (char)((data & 0x000000FF) >> 0);
        send_data[7] = (char)((data & 0x0000FF00) >> 8);
        send_data[8] = (char)((data & 0x00FF0000) >> 16);
        send_data[9] = (char)((data & 0xFF000000) >> 24);

        retval = SEND(sock, send_data, 10);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == WRITE_WORD) {
                return (int)(recv_data[1]);
        } else {
                fprintf(pLogFile, "\n!! outw: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* write byte */
/* could use outb write byte to target memory */
int outb(unsigned int address, unsigned char data)
{
        char send_data[7],recv_data[2];
        int retval __attribute__((unused));

        send_data[0] = WRITE_BYTE;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);
        send_data[6] = (char)data;

        retval = SEND(sock, send_data, 7);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == WRITE_BYTE) {
                return (int)(recv_data[1]);
        } else {
                fprintf(pLogFile, "\n!! outb: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* write half */
/* could use outh write half to target memory */
int outh(unsigned int address, unsigned short data)
{
        char send_data[8],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = WRITE_HALF;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);
        send_data[6] = (char)(data & 0x00FF);
        send_data[7] = (char)((data & 0xFF00) >> 8);

        retval = SEND(sock, send_data, 8);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == WRITE_HALF) {
                return (int)(recv_data[1]);
        } else {
                fprintf(pLogFile, "\n!! outh: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* fast read */
/* could use fastin read mult-word from target memory */
int fastin(unsigned int address, unsigned int size, char * buffer)
{
        char send_data[10],recv_data[2048];
        int retval __attribute__((unused));
        unsigned int i;
        send_data[0] = FAST_READ;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);
        send_data[6] = (char)((size & 0x000000FF) >> 0);
        send_data[7] = (char)((size & 0x0000FF00) >> 8);
        send_data[8] = (char)((size & 0x00FF0000) >> 16);
        send_data[9] = (char)((size & 0xFF000000) >> 24);

        retval = SEND(sock, send_data, 10);
        retval = RECV(sock, recv_data, size + 2);

        for (i = 0; i < size; i++) buffer[i] = recv_data[2+i];

        if (recv_data[0] == FAST_READ) {
                return 0;
        } else {
                fprintf(pLogFile, "\n!! fastin: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* fast read byte: only for V5*/
/* could use fastbytein read mult-byte from target memory */
int fastbytein(unsigned int address, unsigned int size, char * buffer, char if_const_addr)
{
        char send_data[11],recv_data[2048];
        int retval __attribute__((unused));
        unsigned int i;
        send_data[0] = FAST_READ_BYTE;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);
        send_data[6] = (char)((size & 0x000000FF) >> 0);
        send_data[7] = (char)((size & 0x0000FF00) >> 8);
        send_data[8] = (char)((size & 0x00FF0000) >> 16);
        send_data[9] = (char)((size & 0xFF000000) >> 24);
        send_data[10] = if_const_addr;

        retval = SEND(sock, send_data, 11);
        retval = RECV(sock, recv_data, size + 2);

        for (i = 0; i < size; i++) buffer[i] = recv_data[2+i];

        if (recv_data[0] == FAST_READ_BYTE) {
                return 0;
        } else {
                fprintf(pLogFile, "\n!! fastbytein: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* fast write */
/* could use fastout write mult-word from target memory */
int fastout(unsigned int address, unsigned int size, char * buffer)
{
        char send_data[2048],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = FAST_WRITE;
        *(int *)(send_data + 2) = address;
        *(int *)(send_data + 6) = size;

        memcpy(send_data + 12, buffer, size);

        retval = SEND(sock, send_data, size+12);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == FAST_WRITE) {
                return 0;
        } else {
                fprintf(pLogFile, "\n!! fastout: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* fast write byte: only for V5 */
/* could use fastbyteout write mult-byte from target memory */
int fastbyteout(unsigned int address, unsigned int size, char * buffer, char if_const_addr)
{
        char send_data[2048],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = FAST_WRITE_BYTE;
        *(int *)(send_data + 2) = address;
        *(int *)(send_data + 6) = size;
        *(send_data + 10) = if_const_addr;

        memcpy(send_data + 12, buffer, size);

        retval = SEND(sock, send_data, size+12);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == FAST_WRITE_BYTE) {
                return 0;
        } else {
                fprintf(pLogFile, "\n!! fastbyteout: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* multiple write: only for V3 */
/* could use multiout write multi-addr/word pairs from target memory */
unsigned int multi_out(unsigned int multiCmmd, unsigned int *p_address, unsigned char *p_data, const unsigned int number_of_pairs)
{
        char send_data[2048], recv_data[2];
        int retval __attribute__((unused));
        unsigned int i, WriteAddr=0, WriteData=0;
        unsigned int *p_int_data = (unsigned int *)p_data;
        unsigned short *p_short_data = (unsigned short *)p_data;
        unsigned char *p_char_data = (unsigned char *)p_data;

        if (number_of_pairs > MAX_MULTI_WRITE_PAIR)
                fprintf(pLogFile, "\n!! multi_out: num of pairs is too large %s !!\n", TARGET_ERROR);

        send_data[0] = multiCmmd;
        send_data[1] = number_of_pairs & 0xFF;

        for (i = 0 ; i < number_of_pairs ; i++) {
                if (multiCmmd == MULTIPLE_WRITE_WORD)
                        WriteData = *p_int_data++;
                else if (multiCmmd == MULTIPLE_WRITE_HALF)
                        WriteData = *p_short_data++;
                else if (multiCmmd == MULTIPLE_WRITE_BYTE)
                        WriteData = *p_char_data++;

                WriteAddr = *p_address++;
                send_data[i*8 + 2] = (char)((WriteAddr & 0x000000FF) >> 0);
                send_data[i*8 + 3] = (char)((WriteAddr & 0x0000FF00) >> 8);
                send_data[i*8 + 4] = (char)((WriteAddr & 0x00FF0000) >> 16);
                send_data[i*8 + 5] = (char)((WriteAddr & 0xFF000000) >> 24);

                send_data[i*8 + 6] = (char)((WriteData & 0x000000FF) >> 0);
                send_data[i*8 + 7] = (char)((WriteData & 0x0000FF00) >> 8);
                send_data[i*8 + 8] = (char)((WriteData & 0x00FF0000) >> 16);
                send_data[i*8 + 9] = (char)((WriteData & 0xFF000000) >> 24);
        }

        retval = SEND(sock, send_data, 2 + number_of_pairs * 8);
        retval = RECV(sock, recv_data, 2);
        if (recv_data[0] != (char)multiCmmd) {
                fprintf(pLogFile, "\n!! multi_out: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);
                close_connection ();
                exit (0);
        }
        return 0;
}
/* only for V3 */
int multiout_w(unsigned int *p_address, unsigned int *p_data, const unsigned int number_of_pairs)
{
        unsigned int RetValue;

        RetValue = multi_out(MULTIPLE_WRITE_WORD, p_address, (unsigned char *)p_data, number_of_pairs);
        return (int)RetValue;
}
/* only for V3 */
int multiout_h(unsigned int *p_address, unsigned short *p_data, const unsigned int number_of_pairs)
{
        unsigned int RetValue;

        RetValue = multi_out(MULTIPLE_WRITE_HALF, p_address, (unsigned char *)p_data, number_of_pairs);
        return (int)RetValue;
}
/* only for V3 */
int multiout_b(unsigned int *p_address, unsigned char *p_data, const unsigned int number_of_pairs)
{
        unsigned int RetValue;

        RetValue = multi_out(MULTIPLE_WRITE_BYTE, p_address, (unsigned char *)p_data, number_of_pairs);
        return (int)RetValue;
}

/* multiple read: only for V3 */
/* could use multiin read multi-addr/word pairs from target memory */
unsigned int multi_in(unsigned int multiCmmd, unsigned int *p_address, unsigned char *p_data, const unsigned int number_of_pairs)
{
        char send_data[2048], recv_data[2048];
        int retval __attribute__((unused));
        unsigned int i, ReadAddr=0, ReadData=0;
        unsigned int *p_int_data = (unsigned int *)p_data;
        unsigned short *p_short_data = (unsigned short *)p_data;
        unsigned char *p_char_data = (unsigned char *)p_data;

        if (number_of_pairs > MAX_MULTI_WRITE_PAIR)
                fprintf(pLogFile, "\n!! multi_in: num of pairs is too large %s !!\n", TARGET_ERROR);

        send_data[0] = multiCmmd;
        send_data[1] = number_of_pairs & 0xFF;

        for (i = 0 ; i < number_of_pairs ; i++) {
                ReadAddr = *p_address++;
                send_data[i*4 + 2] = (char)((ReadAddr & 0x000000FF) >> 0);
                send_data[i*4 + 3] = (char)((ReadAddr & 0x0000FF00) >> 8);
                send_data[i*4 + 4] = (char)((ReadAddr & 0x00FF0000) >> 16);
                send_data[i*4 + 5] = (char)((ReadAddr & 0xFF000000) >> 24);
        }

        retval = SEND(sock, send_data, 2 + number_of_pairs * 4);
        retval = RECV(sock, recv_data, 2 + number_of_pairs * 4);

        if (recv_data[0] != (char)multiCmmd) {
                fprintf(pLogFile, "\n!! multi_in: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);
                close_connection ();
                exit (0);
        }

        if (multiCmmd == MULTIPLE_READ_WORD) {
                for (i = 0 ; i < number_of_pairs ; i++) {
                        ReadData = (unsigned char)recv_data[i*4 + 2];
                        ReadData |= ((unsigned char)recv_data[i*4 + 3] << 8);
                        ReadData |= ((unsigned char)recv_data[i*4 + 4] << 16);
                        ReadData |= ((unsigned char)recv_data[i*4 + 5] << 24);
                        *p_int_data++ = ReadData;
                        //printf("MULTIPLE_READ: %x %x %x %x\n", recv_data[i*4 + 2], recv_data[i*4 + 3], recv_data[i*4 + 4], ReadData);
                }
        } else if (multiCmmd == MULTIPLE_READ_HALF) {
                for (i = 0 ; i < number_of_pairs ; i++) {
                        ReadData = (unsigned char)recv_data[i*2 + 2];
                        ReadData |= ((unsigned char)recv_data[i*2 + 3] << 8);
                        *p_short_data++ = ReadData;
                }
        } else if (multiCmmd == MULTIPLE_READ_BYTE) {
                for (i = 0 ; i < number_of_pairs ; i++) {
                        ReadData = (unsigned char)recv_data[i + 2];
                        *p_char_data++ = ReadData;
                }
        }
        return 0;
}
/* only for V3 */
int multiin_w(unsigned int *p_address, unsigned int *p_data, const unsigned int number_of_pairs)
{
        unsigned int RetValue;

        RetValue = multi_in(MULTIPLE_READ_WORD, p_address, (unsigned char *)p_data, number_of_pairs);
        return (int)RetValue;
}
/* only for V3 */
int multiin_h(unsigned int *p_address, unsigned short *p_data, const unsigned int number_of_pairs)
{
        unsigned int RetValue;

        RetValue = multi_in(MULTIPLE_READ_HALF, p_address, (unsigned char *)p_data, number_of_pairs);
        return (int)RetValue;
}
/* only for V3 */
int multiin_b(unsigned int *p_address, unsigned char *p_data, const unsigned int number_of_pairs)
{
        unsigned int RetValue;

        RetValue = multi_in(MULTIPLE_READ_BYTE, p_address, (unsigned char *)p_data, number_of_pairs);
        return (int)RetValue;
}

/* read byte */
/* could use inb read byte from target memory */
char inb(unsigned int address)
{
        char data;
        char send_data[6],recv_data[3];
        int retval __attribute__((unused));
        send_data[0] = READ_BYTE;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);

        retval = SEND(sock, send_data, 6);
        retval = RECV(sock, recv_data, 3);

        data = recv_data[2] & 0x000000FF;

        if (recv_data[0] == READ_BYTE)
                return data;
        else {
                fprintf(pLogFile, "\n!! inb: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* read half */
/* could use inh read half from target memory */
short inh(unsigned int address)
{
        short data;
        char send_data[6],recv_data[4];
        int retval __attribute__((unused));
        send_data[0] = READ_HALF;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);

        retval = SEND(sock, send_data, 6);
        retval = RECV(sock, recv_data, 4);

        data = (((recv_data[2]<< 8) & 0x0000FF00) |
                ((recv_data[3]<< 0) & 0x000000FF));

        if (recv_data[0] == READ_HALF)
                return data;
        else {
                fprintf(pLogFile, "\n!! inh: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* read word */
/* could use inw read word from target memory */
int inw(unsigned int address)
{
        int data;
        char send_data[6],recv_data[6];
        int retval __attribute__((unused));
        send_data[0] = READ_WORD;
        send_data[2] = (char)((address & 0x000000FF) >> 0);
        send_data[3] = (char)((address & 0x0000FF00) >> 8);
        send_data[4] = (char)((address & 0x00FF0000) >> 16);
        send_data[5] = (char)((address & 0xFF000000) >> 24);

        retval = SEND(sock, send_data, 6);
        retval = RECV(sock, recv_data, 6);

        data = (((recv_data[2]<< 24) & 0xFF000000) |
                ((recv_data[3]<< 16) & 0x00FF0000) |
                ((recv_data[4]<< 8) & 0x0000FF00) |
                ((recv_data[5]<< 0) & 0x000000FF));

        if (recv_data[0] == READ_WORD)
                return data;
        else {
                fprintf(pLogFile, "\n!! inw: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* only for V3 */
int inr(unsigned int num)
{
        int data;
        char send_data[6],recv_data[6];
        int retval __attribute__((unused));
        send_data[0] = READ_REG;
        send_data[2] = (char)((num & 0x000000FF) >> 0);
        send_data[3] = (char)((num & 0x0000FF00) >> 8);
        send_data[4] = (char)((num & 0x00FF0000) >> 16);
        send_data[5] = (char)((num & 0xFF000000) >> 24);

        retval = SEND(sock, send_data, 6);
        retval = RECV(sock, recv_data, 6);

        data = (((recv_data[2]<< 24) & 0xFF000000) |
                ((recv_data[3]<< 16) & 0x00FF0000) |
                ((recv_data[4]<< 8) & 0x0000FF00) |
                ((recv_data[5]<< 0) & 0x000000FF));

        if (recv_data[0] == READ_REG)
                return data;
        else {
                // SKIP error, force continue!!
                return 0;
        }
}

/* only for V3 */
int outr(unsigned int num, unsigned int data)
{
	char send_data[10],recv_data[2];
	int retval __attribute__((unused));
	send_data[0] = WRITE_REG;
	send_data[2] = (char)((num & 0x000000FF) >> 0);
	send_data[3] = (char)((num & 0x0000FF00) >> 8);
	send_data[4] = (char)((num & 0x00FF0000) >> 16);
	send_data[5] = (char)((num & 0xFF000000) >> 24);
	send_data[6] = (char)((data & 0x000000FF) >> 0);
	send_data[7] = (char)((data & 0x0000FF00) >> 8);
	send_data[8] = (char)((data & 0x00FF0000) >> 16);
	send_data[9] = (char)((data & 0xFF000000) >> 24);

	retval = SEND(sock, send_data, 10);
	retval = RECV(sock, recv_data, 2);

	if (recv_data[0] == WRITE_REG) {
		return (int)(recv_data[1]);
	} else {
		fprintf(pLogFile, "\n!! outw: %s !!\n", TARGET_ERROR);
		fflush(pLogFile);

		close_connection ();
		exit (0);
	}
}

/* Close connection with ICEman */
void close_connection (void)
{
        char send_data[2];
        int retval __attribute__((unused));

        send_data[0] = 4; // Termination command code
        retval = SEND(sock, send_data, 1);

        // free socket resource
#ifdef __MINGW32__
        closesocket (sock);
        do {
                WSACleanup ();
        } while (WSAGetLastError () != WSANOTINITIALISED);
#else
        close (sock);
#endif
}

/* Need send terminate to ICEman before quit Burner */
void terminate(void)
{
//#ifndef SPI_BURN
        // reset-and-run
        //send_cmd (RESET_TARGET); // If you want target to free run after burning done.
        // send_cmd (RESET_HOLD); // If you want target to hold after burning done.
//#endif

        close_connection ();
}

/* SIGINT handling function */
void handle_int(int signo)
{
        terminate();
        fprintf(pLogFile, "\n!! burn not done yet !!\n");
        fflush(pLogFile);
        exit(0);
}

#ifdef __MINGW32__

#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2

int inet_pton4(const char *src, char *dst)
{
	uint8_t tmp[NS_INADDRSZ], *tp;

	int saw_digit = 0;
	int octets = 0;
	*(tp = tmp) = 0;

	int ch;
	while ((ch = *src++) != '\0') {
		if (ch >= '0' && ch <= '9') {
			uint32_t n = *tp * 10 + (ch - '0');
			if (saw_digit && *tp == 0)
				return 0;

			if (n > 255)
				return 0;

			*tp = n;
			if (!saw_digit) {
				if (++octets > 4)
					return 0;
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return 0;
			*++tp = 0;
			saw_digit = 0;
		} else
			return 0;
	}
	if (octets < 4)
		return 0;

	memcpy(dst, tmp, NS_INADDRSZ);
	return 1;
}

#include <ctype.h>
int inet_pton6(const char *src, char *dst)
{
	static const char xdigits[] = "0123456789abcdef";
	uint8_t tmp[NS_IN6ADDRSZ];

	uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
	uint8_t *endp = tp + NS_IN6ADDRSZ;
	uint8_t *colonp = NULL;

	/* Leading :: requires some special handling. */
	if (*src == ':') {
		if (*++src != ':')
			return 0;
	}

	const char *curtok = src;
	int saw_xdigit = 0;
	uint32_t val = 0;
	int ch;
	while ((ch = tolower(*src++)) != '\0') {
		const char *pch = strchr(xdigits, ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff)
				return 0;
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					return 0;
				colonp = tp;
				continue;
			} else if (*src == '\0') {
				return 0;
			}
			if (tp + NS_INT16SZ > endp)
				return 0;
			*tp++ = (uint8_t) (val >> 8) & 0xff;
			*tp++ = (uint8_t) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
				inet_pton4(curtok, (char*) tp) > 0) {
			tp += NS_INADDRSZ;
			saw_xdigit = 0;
			break; /* '\0' was seen by inet_pton4(). */
		}
		return 0;
	}
	if (saw_xdigit) {
		if (tp + NS_INT16SZ > endp)
			return 0;
		*tp++ = (uint8_t) (val >> 8) & 0xff;
		*tp++ = (uint8_t) val & 0xff;
	}
	if (colonp != NULL) {
		/*
		 ** Since some memmove()'s erroneously fail to handle
		 ** overlapping regions, we'll do the shift by hand.
		 **/
		const int n = tp - colonp;

		if (tp == endp)
			return 0;

		int i;
		for (i = 1; i <= n; i++) {
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return 0;

	memcpy(dst, tmp, NS_IN6ADDRSZ);
	return 1;
}

int inet_pton(int af, const char *src, char *dst)
{
	switch (af)
	{
		case AF_INET:
			return inet_pton4(src, dst);
		case AF_INET6:
			return inet_pton6(src, dst);
		default:
			return -1;
	}
}
#endif

/* Prepare socket communication */
void initial_socket(const char *host, unsigned int port)
{
	struct in6_addr serveraddr;
	struct addrinfo hints, *res=NULL;
	char portArray[10] = {0};
	int rc;

#ifdef __MINGW32__
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
                fprintf(pLogFile, "Error creating socket.");
                fflush(pLogFile);
                fflush(stdout);
		WSACleanup();
		exit(1);
        }
#endif

	memset(&hints, 0x0, sizeof(hints));
	//hints.ai_flags    = AI_NUMERICSERV;
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rc = inet_pton(AF_INET, host, &serveraddr);
	if(rc == 1) {	/* valid IPv4 text address? */
		hints.ai_family = AF_INET;
		//hints.ai_flags |= AI_NUMERICHOST;
	} else {
		rc = inet_pton(AF_INET6, host, &serveraddr);
		if (rc == 1) { /* valid IPv6 text address? */
			hints.ai_family = AF_INET6;
			//hints.ai_flags |= AI_NUMERICHOST;
		}
	}

	sprintf(portArray, "%u", port);
	fprintf(pLogFile, "Attempting to connect to %s, port %d\n", host, port);
	fflush(pLogFile);
	if( (rc = getaddrinfo(host, portArray, &hints, &res)) != 0 ) {
#ifdef __MINGW32__
                fprintf(pLogFile, "Host not found --> %d\n", rc);
#else
                fprintf(pLogFile, "Host not found --> %s\n", gai_strerror(rc));
#endif
                fflush(pLogFile);
		exit(1);
	}

#ifdef __MINGW32__
	struct addrinfo *ai;
	for( ai = res; ai != NULL; ai = ai->ai_next ) {
		if ((sock = socket(ai->ai_family, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			continue;
		}
		if (connect(sock, ai->ai_addr, ai->ai_addrlen) != SOCKET_ERROR) {
			fprintf(pLogFile, "Socket call with family: %d socktype: %d, protocol: %d\n",
				ai->ai_family, ai->ai_socktype, ai->ai_protocol);
			fprintf(pLogFile, "Connect successful!!\n");
			fflush(pLogFile);
			break;
		}
	}

	if( ai == NULL ) {
                fprintf(pLogFile, "Can't create socket\n");
                fprintf(pLogFile, "Connecting fail !!\n");
                fflush(pLogFile);
                exit(1);
	}
#else
	if ((sock = socket(res->ai_family, SOCK_STREAM, 0)) < 0) {
                fprintf(pLogFile, "Can't create socket\n");
                fflush(pLogFile);
                exit(1);
	}

	if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
                fprintf(pLogFile, "connecting fail !!\n");
                fflush(pLogFile);
                exit(1);
        }
#endif

        // Set RCV/SND Timeout
        struct timeval timeout;

#ifdef __MINGW32__
        timeout.tv_sec = 30000;  // 30s
        timeout.tv_usec = 0;
#else
        timeout.tv_sec = 30;  // 30s
        timeout.tv_usec = 0;
#endif

        if ( setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ) {
                fprintf(pLogFile, "setsockopt SO_RCVTIMEO fail !!\n");
                fflush(pLogFile);
                exit(1);
        }

        if ( setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ) {
                fprintf(pLogFile, "setsockopt SO_SNDTIMEO fail !!\n");
                fflush(pLogFile);
                exit(1);
        }
}

/* Read image binary data */
unsigned char* get_image(FILE* image, unsigned int* size)
{
        unsigned char* flash;
        unsigned int word_aligned_size = 0;
        int retval __attribute__((unused));
        if (image == NULL) {
                fprintf (pLogFile, "Error: Burn image have to specify.\n");
                fflush(pLogFile);
                exit(1);
        }

        fseek(image, 0L, SEEK_END);
        (*size) = (ftell(image));
        fseek(image, 0L, SEEK_SET);

        //allocate word-aligned memory size
        word_aligned_size = (((*size) + 3) / 4) * 4;
       
        if ((flash = (UINT8*)malloc(word_aligned_size * (sizeof(UINT8)))) == NULL) {
                fprintf(pLogFile, "Error: can't allocate memory (%d bytes) for flash file\n", word_aligned_size);
                fflush(pLogFile);
                return 0;
        }
        retval = fread(flash, (*size), 1, image);
        return flash;
}

/* Performance utility functions - to calculate the difference of a_timeval_begin and a_timeval_end */
void timeval_diff (struct timeval *a_result, struct timeval *a_timeval_begin, struct timeval *a_timeval_end)
{
        if (a_timeval_end->tv_usec >= a_timeval_begin->tv_usec) {
                a_result->tv_usec = (a_timeval_end->tv_usec - a_timeval_begin->tv_usec);
                a_result->tv_sec = (a_timeval_end->tv_sec - a_timeval_begin->tv_sec);
                if (a_result->tv_usec >= 1000000) {
                        a_result->tv_usec -= 1000000;
                        a_result->tv_sec += 1;
                }
        } else {
                a_result->tv_usec = (a_timeval_end->tv_usec + (1000000 - a_timeval_begin->tv_usec));
                a_result->tv_sec = (a_timeval_end->tv_sec - a_timeval_begin->tv_sec - 1);
        }
}

/* send command to iceman
 * command code refer to util.h */
int send_targetname(char *target_name, unsigned int size)
{
        char* send_data = (char *)malloc(size*sizeof(char)+5);
        char recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = BURNER_SELECT_TARGET;

        set_u32(send_data+1, size);
        memcpy(send_data+5, target_name, size);

        retval = SEND(sock, send_data, size+5);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == BURNER_SELECT_TARGET && recv_data[1] == 0) {
                return (int)(recv_data[1]);
        } else {
                fprintf(pLogFile, "\n!! target %s is unknow !!\n", target_name);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* send command to iceman
 * command code refer to util.h */
int send_coreid(int coreid)
{
        char send_data[6],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = BURNER_SELECT_CORE;

        set_u32(send_data+1, coreid);

        retval = SEND(sock, send_data, 5);
        retval = RECV(sock, recv_data, 2);

        if (recv_data[0] == BURNER_SELECT_CORE && recv_data[1] == 0) {
                return (int)(recv_data[1]);
        } else {
                fprintf(pLogFile, "\n!! send_coreid: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }
}

/* only for V3 */
int send_monitor_cmd( char* monitor_cmd, unsigned int size, char** ret_data )
{
        char recv_data[MAX_SOCKET_BUF];
        char* send_data = (char *)malloc(size*sizeof(char)+5);
        int ret_size = 0;
        int retval __attribute__((unused));

        send_data[0] = MONITOR_CMD;

        set_u32(send_data+1, size);
        memcpy(send_data+5, monitor_cmd, size);

        retval = SEND(sock, send_data, size+5);
        retval = RECV(sock, recv_data, MAX_SOCKET_BUF);

        if ( recv_data[0] == MONITOR_CMD && recv_data[1] == 0) {
                ret_size = get_u32(recv_data+2);
                *ret_data = (char*)malloc((ret_size+4)*sizeof(char));
                memcpy(*ret_data, recv_data+2,ret_size+4);  // copy data including length
                return 0;
        } else {
                fprintf(pLogFile, "\n!! send_monitor_cmd failed: len=%d, %s !!\n", (int)strlen(monitor_cmd), monitor_cmd);
                fflush(pLogFile);

                close_connection ();
                exit (0);
        }

        return -1;
}

/* only for V3 */
int read_edm_jdp(unsigned int JDPInst, unsigned int address, unsigned int *pread_data)
{
        char send_data[10],recv_data[8];
        int retval __attribute__((unused));
        unsigned int get_data=0;

        send_data[0] = READ_EDM_JDP;
        send_data[2] = JDPInst;
        send_data[3] = address;
        send_data[4] = 0;
        send_data[5] = 0;

        retval = SEND(sock, send_data, 6);
        retval = RECV(sock, recv_data, 6);

        get_data = (((recv_data[2]<< 24) & 0xFF000000) |
                    ((recv_data[3]<< 16) & 0x00FF0000) |
                    ((recv_data[4]<< 8) & 0x0000FF00) |
                    ((recv_data[5]<< 0) & 0x000000FF));
        if (recv_data[0] == READ_EDM_JDP) {
                *pread_data = get_data;
                return 0;
        } else {
                fprintf(pLogFile, "\n!! read_edm_jdp: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);
                close_connection ();
                exit (0);
        }
        return -1;
}

/* only for V3 */
int write_edm_jdp(unsigned int JDPInst, unsigned int address, unsigned int write_data)
{
        char send_data[10],recv_data[2];
        int retval __attribute__((unused));
        send_data[0] = WRITE_EDM_JDP;
        send_data[2] = JDPInst;
        send_data[3] = address;
        send_data[4] = 0;
        send_data[5] = 0;
        send_data[6] = (write_data & 0xFF);
        send_data[7] = (write_data & 0xFF00) >> 8;
        send_data[8] = (write_data & 0xFF0000) >> 16;
        send_data[9] = (write_data & 0xFF000000) >> 24;

        retval = SEND(sock, send_data, 10);
        retval = RECV(sock, recv_data, 2);
        if (recv_data[0] == WRITE_EDM_JDP) {
                return 0;
        } else {
                fprintf(pLogFile, "\n!! write_edm_jdp: %s !!\n", TARGET_ERROR);
                fflush(pLogFile);
                close_connection ();
                exit (0);
        }
        return -1;
}

/* select_mode */
/* Index to select which memory device to be accessed in Direct Access Memory mode */
int mem_select_mode(unsigned int mem_mode)
{
        return write_edm_jdp(JDP_W_MISC_REG, NDS_EDM_MISC_ACC_CTL, mem_mode);
}

/* read EDM config */
int read_edm_cfg(unsigned int *pread_data)
{
        return read_edm_jdp(JDP_R_DBG_SR, NDS_EDM_SR_EDM_CFG, pread_data);
}
