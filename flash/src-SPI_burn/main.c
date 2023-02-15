#include <sys/time.h>
#ifdef __MINGW32__
#include <winsock2.h>  // for Sleep()
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"
#include "flash-rom.h"

#define DEF_CONNECT_HOST      "127.0.0.1"
#define DEF_CONNECT_PORT      2354
// TODO: select target-specific flash driver
#if SPI_BURN
extern flash_dev flash_MXIC;
#else
extern flash_dev flash_Micron;
extern flash_dev flash_IntelJ3;
#endif
flash_dev *gpFlash;

#ifdef __MINGW32__
extern SOCKET sock;
#else
extern int sock;
#endif

char *image_filename = NULL;
char *nds32_change_filepath(char *ori_filename);

int nds32_preserve(unsigned int *pFlashAddr, unsigned char **pDataStart, unsigned int *pDataSize);
int nds32_erase(unsigned int FlashAddr, unsigned int DataSize);
int nds32_burn(unsigned int FlashAddr, unsigned char *pDataStart, unsigned int DataSize);
int nds32_verify(unsigned int FlashAddr, unsigned char *pDataStart, unsigned int DataSize);
int nds32_usage(void);
int Record_content(unsigned int address, unsigned char* buffer, unsigned int size);
int Verification(unsigned int address, unsigned char* start, unsigned int size);
extern unsigned int platform_init(void);
extern unsigned int restore_ivb(void);
int nds32_idlm_testing(void);
int test_monitor_cmd_0(unsigned int address);
int check_delay_count(unsigned int FlashAddr, unsigned char *pDataStart, unsigned int DataSize);
extern int check_need_run_delay_count(void);

unsigned int guiCtrlBase = 0;
unsigned int guiMode16MB = 0;
/* In fast mode, it will skip some checks during burning */
unsigned int guiFastMode = 0;
/* multi-out */
unsigned int guiWithMultiout = 1;
/* Measure performance - burning time */
unsigned int guiMeasureTime = 0;
/* Log file descriptor */
FILE *pLogFile;
int coreid = 0;
extern unsigned int guiConstFastMode;
extern unsigned int guiUserDefConstFastMode;

/* option structure */
static struct option long_option[] = {
        {"help", no_argument, 0, 'h'},
        {"preserve", no_argument, 0, 'e'},
        {"reset-target", no_argument, 0, 't'},
        {"reset-and-run", no_argument, 0, 't'},
        {"reset-hold", no_argument, 0, 'o'},
        {"reset-and-hold", no_argument, 0, 'o'},
        {"target", required_argument, 0, 'N'},
        {"target", required_argument, 0, 'x'},
        {"lock", no_argument, 0, 'z'},
        {"unlock", no_argument, 0, 'y'},
        {"verify", no_argument, 0, 'v'},
        {"verify-only", no_argument, 0, 'c'},
        {"source", no_argument, 0, 's'},
        {"version", no_argument, 0, 'V'},
        //{"fast", no_argument, 0, 'g'},
        {"erase-all", no_argument, 0, 'u'},
        {"erase-all-only", no_argument, 0, 'U'},
        {"force", no_argument, 0, 'F'},
        {"log", required_argument, 0, 'l'},
        {"host", required_argument, 0, 'H'},
        {"port", required_argument, 0, 'p'},
        {"addr", required_argument, 0, 'a'},
        /*{"flash", required_argument, 0, 'f'},*/
        {"base", required_argument, 0, 'b'},
        {"image", required_argument, 0, 'i'},
        {"measure-time", no_argument, 0, 'm'},
        {"multiout", no_argument, 0, 'T'},
        {"const-fast", required_argument, 0, 'f'},
        {"core-num", required_argument, 0, 'n'},
        {"target-num", required_argument, 0, 'n'},
        {"test-custom-monitor", no_argument, 0, 'C'},
        {"idlm-testing", no_argument, 0, 'j'},
        {"debug", no_argument, 0, 'D'},
        {0, 0, 0, 0}
};

/* main function
 *  . parse parameters
 *  . initial socket
 *  . select target coreid
 *  . reset AICE
 *  . issue RESET_HOLD to let target board restore to reset status
 *    avoid boot code to damage flash registers' settings
 *  . call target-dependent initial function and calculate flash base address (Device Address)
 *     xc5()/p24()/ag101()/ag301p()
 *  . read image binary data from --image specified file
 *  . if user specifies --verify-only, verify the target with the image data and exit
 *  . preserve first/last blocks if needed (As start/end addresses are not block-aligned)
 *  . erase flash
 *  . burn image - call Flash_BurnImage()
 *  . verify    */
int main(int argc, char **argv)
{
        pLogFile = stdout;

        int c = 0;
        char host[64] = DEF_CONNECT_HOST;     /* default connecting host */
        unsigned int port = DEF_CONNECT_PORT; /* default connecting port */
        unsigned int base = 0;                /* flash base address */
        unsigned int address = 0;             /* default write to flash base */
        unsigned int size = 0;                /* image size */
        unsigned int erase_size = 0;          /* erase size */
        unsigned char *start = NULL;          /* image pointer */
        FILE *image = NULL;                   /* image to burn */
        char cmd = 0;
        char *name;// = (char*)malloc(1024*sizeof(char));
        int specify_base = 0, specify_base_flag = 0;
        //int specify_address = 0;
        int preserve = 0, verify = 0;
        int version_only = 0, verify_only = 0, erase_all = 0, erase_all_only = 0;
        int lock = 0, unlock = 0;
        int Ret;
        int test_monitor_cmd = 0;
        int test_idlm = 0;
        int debug_mode = 0;
        char target_name[50] = "";

        /* parameter handler */
        while (1) {
                int option_index;
                c = getopt_long(argc, argv, "uUvcVtoehzyTjml:N:x:k:H:p:a:b:i:n:CDf:", long_option, &option_index);
                if (c == EOF)
                        break;

                switch (c) {
                        case 'u':
                                erase_all = 1;
                                break;
                        case 'U':
                                erase_all_only = 1;
                                break;
                        case 'z':
                                lock = 1;
                                break;
                        //case 'g':
                        //        guiFastMode = 1;
                        //        break;
                        case 'y':
                                unlock = 1;
                                break;
                        case 'v':
                                verify = 1;
                                break;
                        case 'c':
                                verify_only = 1;
                                break;
                        case 'l':
                                name = strdup(optarg); //fopen format  need change /cygdrive/c to c:
                                if (strncmp(optarg, "/cygdrive/", 10) == 0) {
                                        name[0] = optarg[10];
                                        name[1] = ':';
                                        name[2] = '\0';
                                        strcat(name, optarg + 11);
                                }
                                if ((pLogFile = fopen(name, "w"))== NULL) {
                                        fprintf (pLogFile, "Error: can't open file %s\n", name);
                                        fflush(pLogFile);
                                        exit(1);
                                }
                                break;
                        case 's':
                                //fprintf (pLogFile, "Branch%s\n", BRANCH_NAME);
                                //fprintf (pLogFile, "%s\n", COMMIT_ID);
                                fflush(pLogFile);
                                exit(0);
                        case 'V':
                                version_only = 1;
                                break;
                        case 'x':
                                if (strncmp(optarg, "ag101p_16mb", 6) == 0)
                                        guiMode16MB = 1;
                                break;
                        case 't':
                                cmd = RESET_TARGET;
                                break;
                        case 'o':
                                cmd = RESET_HOLD;
                                break;
                        case 'e':
                                preserve = 1;
                                break;
                        case 'H':
                                strcpy(host, optarg);
                                break;
                        case 'p':
                                port = strtol(optarg, NULL, 0);
                                break;
                        case 'a':
                                sscanf(optarg,"0x%x", &address);
                                //specify_address = 1;
                                break;
                        case 'b':
                                sscanf(optarg,"0x%x", &specify_base);
                                specify_base_flag = 1;
                                break;
                        case 'i':
                                image_filename = strdup(optarg);
                                image_filename = nds32_change_filepath(image_filename);
                                name = strdup(optarg); //fopen format  need change /cygdrive/c to c:
                                if (strncmp(optarg, "/cygdrive/", 10) == 0) {
                                        name[0] = optarg[10];
                                        name[1] = ':';
                                        name[2] = '\0';
                                        strcat(name, optarg + 11);
                                }
                                if ((image = fopen(name, "rb"))== NULL) {
                                        fprintf (pLogFile, "Error: can't open file %s\n", name);
                                        fflush(pLogFile);
                                        exit(1);
                                }
                                /* get burn image content */
                                start = get_image (image, &size);
                                fclose (image);
                                fprintf(pLogFile, "image_filename: %s\n", name);
                                fflush(pLogFile);
                                break;
                        case 'm':
                                guiMeasureTime = 1;
                                break;
                        case 'T':
                                guiWithMultiout = 1;
                                break;
                        case 'f':
                                sscanf(optarg,"%d", &guiConstFastMode);
                                guiUserDefConstFastMode = 1;
                                break;
                        case 'n':
                                sscanf(optarg,"%d", &coreid);
                                break;
                        case 'C':
                                test_monitor_cmd = 1;
                                break;
                        case 'j':
                                test_idlm = 1;
                                break;
                        case 'D':
                                debug_mode = 1;
                                break;
                        case 'N':
                                fprintf(pLogFile, "target: %s\n", optarg);
                                fflush(pLogFile);
                                // process --target duplicate for -x and -N
                                if (strncmp(optarg, "ag101p_16mb", 6) == 0) {
                                        guiMode16MB = 1;
                                } else if (strncmp(optarg, "ag101", 5) == 0) {
                                } else if (strncmp(optarg, "xc5", 3) == 0) {
                                } else {
                                        strcpy(target_name, optarg);
                                }
                                break;
                        case 'h':
                        case '?':
                        default:
                                nds32_usage();
                                exit(0);
                                break;
                }  /* switch (c)  */
        }  /* while(1)  */

        fprintf (pLogFile, "NDS32 Burner BUILD_ID: %d\n", BUILD_ID);
        fflush(pLogFile);
        if (version_only || ((start == NULL) && (erase_all_only == 0))) {
                exit(0);
        }

        initial_socket(host, port);

#ifdef __MINGW32__
        Sleep (200);
#else
        usleep (200000);
#endif

        // set debug mode
        if ( debug_mode != 0 ) {
                if (send_cmd(BURNER_DEBUG) < 0) {
                        fprintf(pLogFile, "ERROR!! Set burner debug mode failed, Continue!! \n");
                }
        }

        // target priority: target name > target number
        if (strcmp(target_name, "") != 0) {
                coreid = -1;
                //fprintf (pLogFile, "target_name: %s(len:%d)\n", target_name, strlen(target_name));
                //fflush(pLogFile);
                if (send_targetname(target_name, strlen(target_name)) < 0) {
                        close_connection ();
                        exit(0);
                }
        }

        // select core
        if (coreid >= 0) {
                if (send_coreid(coreid) < 0 ) {
                        close_connection ();
                        exit(0);
                }
        }

        // init AICE
        if (send_cmd(RESET_AICE) < 0) {
                close_connection ();
                exit(0);
        }

        if (cmd != 0) {
                send_cmd (cmd);
                close_connection ();
                exit(0);
        }

        // reset-and-hold to init target
        send_cmd (RESET_HOLD);
        base = platform_init();
        if (specify_base_flag)
                base = specify_base;
        guiCtrlBase = base;

        if (test_idlm == 1) {
                nds32_idlm_testing();
                exit(0);
        }

        /* error check */
        /*
        if (specify_address == 0)
        {
          fprintf(pLogFile, "ERROR!! please add parameter --addr \n");
          fflush(pLogFile);
          terminate ();
          return 0;
        }*/
// TODO: target-specific flash check
#if SPI_BURN
        if (flash_MXIC.flash_chk() != 0) {
                fprintf(pLogFile, "ERROR!! flash type error \n");
                fflush(pLogFile);
                terminate ();
                return 0;
        }
        gpFlash = (flash_dev *)&flash_MXIC;
#else
        if (flash_Micron.flash_chk() != 0) {
                if (flash_IntelJ3.flash_chk() != 0) {
                        fprintf(pLogFile, "ERROR!! flash type error \n");
                        fflush(pLogFile);
                        terminate ();
                        return 0;
                }
                gpFlash = (flash_dev *)&flash_IntelJ3;
        } else {
#if 0
                if (guiWithMultiout == 1) {	/* multiout work abnormal on Micron flash */
                        fprintf(pLogFile, "Micron flash don't support multiout !!\n");
                        terminate();
                        return 0;
                }
#endif
                gpFlash = (flash_dev *)&flash_Micron;
        }
        if (guiMode16MB)
                gpFlash->flash_chipsize = 0x100000;
#endif

        /* handle signal */
        signal(SIGTERM, handle_int);
        signal(SIGINT, handle_int);
#ifndef __MINGW32__
        signal(SIGKILL, handle_int);
#endif
        //check offset + image size < ROM size
        if (start != NULL) {
            if ((address + size) > gpFlash->flash_chipsize) {
                    printf("ERROR!! (offset(--addr):%d + image size:%d) > ROM size:%d", address, size, gpFlash->flash_chipsize);
                    return 0;
            } else {
                    fprintf (pLogFile, "image size: %d bytes\n", size);
                    fflush(pLogFile);
            }
        }

        /* NOTE: burner cannot retry to write image data on OpenOCD, so need trainning to get access memory time */
        Ret = check_delay_count(address, start, size);
        if (Ret != 0) {
                fprintf(pLogFile, "please check iceman_log, maybe need to increase dmi_busy_delay_times to increase delay_count\n");
                fflush(pLogFile);
                terminate();
                return 0;
        }

		/* verify only */
        if (verify_only) {
                nds32_verify(address, start, size);
                terminate ();
                return 0;
        }

        /* unlock flash */
        if (unlock) {
                fprintf(pLogFile, "unlocking");
                fflush(pLogFile);
                gpFlash->flash_unlock(address, size);
                fprintf(pLogFile, "\n");
                fflush(pLogFile);
        }
        /* record the content in first and last erase block */
        if (preserve)
                nds32_preserve(&address, &start, &size);
        /*
        fprintf(pLogFile, "address=%x, start=%x, size=%x \n", address, (unsigned int)start, size);
        fflush(pLogFile);
        */

        /* block erase */
        if (erase_all || erase_all_only)
                erase_size = gpFlash->flash_chipsize;
        else
                erase_size = size;
        Ret = nds32_erase(address, erase_size);
        if ((Ret != 0) || (erase_all_only == 1)) {
                terminate();
                return 0;
        }

        /* burn image */
        nds32_burn(address, start, size);

        /* verify */
        if (verify) {
                Ret = nds32_verify(address, start, size);
                if (Ret == 0)
                        fprintf(pLogFile, "Flash burning done.\n");
                else
                        fprintf(pLogFile, "Please reburn again.\n");
        } else {
                fprintf(pLogFile, "Flash burning done.\n");
        }
        fflush(pLogFile);

        /* lock flash */
        if (lock) {
                fprintf(pLogFile, "locking");
                fflush(pLogFile);
                gpFlash->flash_lock(address, size);
                fprintf(pLogFile, "\n");
                fflush(pLogFile);
        }

        /// Test Monitor Command #0
        if ( test_monitor_cmd == 1 ) {
                fprintf(pLogFile, "Test Monitor Command #0\n");
                fflush(pLogFile);

#if SPI_BURN
                fprintf(pLogFile, "Monitor Command success.\n");
#else
                send_cmd (RESET_TARGET);    // reset-and-run

#ifdef __MINGW32__  // sleep 5s
                Sleep (5000);
#else
                usleep (5000000);
#endif

                test_monitor_cmd_0(address);
#endif
        }

		/* restore IVB to flash */
        restore_ivb();

		send_cmd (RESET_TARGET);

        /* send terminate signal */
        terminate();
        return 0;
}

int nds32_preserve(unsigned int *pFlashAddr, unsigned char **pDataStart, unsigned int *pDataSize)
{
        unsigned int OriFlashAddr, OriDataSize;
        unsigned int NewFlashAddr, NewDataSize, UnalignBytes;
        unsigned int FirstBlkSize, LastBlkSize;
        unsigned char *pNewDataStart, *pOriDataStart, *pCurrData;

        OriFlashAddr = *pFlashAddr;
        OriDataSize = *pDataSize;
        pOriDataStart = *pDataStart;

        NewFlashAddr = (OriFlashAddr / gpFlash->flash_blksize) * gpFlash->flash_blksize;
        FirstBlkSize = (OriFlashAddr - NewFlashAddr);
        UnalignBytes = (OriDataSize % 4);
        if (UnalignBytes)
                LastBlkSize = (4 - UnalignBytes);
        else
                LastBlkSize = 0;
        NewDataSize = OriDataSize + FirstBlkSize + LastBlkSize;

        pNewDataStart = (unsigned char *) malloc (NewDataSize);
        pCurrData = pNewDataStart;
        /* copy the 1st blk from flash */
        if (FirstBlkSize)
                Record_content(NewFlashAddr, pCurrData, FirstBlkSize);
        pCurrData += FirstBlkSize;

        /* copy ori-image file */
        memcpy(pCurrData, pOriDataStart, OriDataSize);
        pCurrData += OriDataSize;

        /* copy last unalign bytes from flash */
        if (LastBlkSize)
                Record_content(OriFlashAddr + OriDataSize, pCurrData, LastBlkSize);

        *pFlashAddr = NewFlashAddr;
        *pDataSize = NewDataSize;
        *pDataStart = pNewDataStart;
        free (pOriDataStart);
        return 0;
}

int nds32_erase(unsigned int FlashAddr, unsigned int DataSize)
{
        struct timeval erase_begin_time;
        struct timeval erase_end_time;
        struct timeval erase_total_time;
        double erase_time_sec;
        int Ret;

        fprintf(pLogFile, "erase from address = 0x%x\n", FlashAddr);
        fflush(pLogFile);

        if (guiMeasureTime)
                gettimeofday (&erase_begin_time, NULL);

        Ret = gpFlash->flash_erase(FlashAddr, DataSize);

        if (guiMeasureTime) {
                gettimeofday (&erase_end_time, NULL);

                timeval_diff (&erase_total_time, &erase_begin_time, &erase_end_time);
                erase_time_sec = erase_total_time.tv_sec + (double)erase_total_time.tv_usec / 1000000;

                fprintf(pLogFile, "\nTotal erase time: %lu sec, %lu usec\n",
                        erase_total_time.tv_sec, erase_total_time.tv_usec);
                fprintf(pLogFile, "Average erase rate: %.2lf KBytes/s\n", (double)(DataSize >> 10) / erase_time_sec);
                fflush(pLogFile);
        }
        return Ret;
}

int nds32_burn(unsigned int FlashAddr, unsigned char *pDataStart, unsigned int DataSize)
{
        struct timeval burn_begin_time;
        struct timeval burn_end_time;
        struct timeval burn_total_time;
        double burn_time_sec;
        int Ret;

        fprintf(pLogFile, "burn flash from 0x%x to 0x%x\n", FlashAddr ,FlashAddr+DataSize);
        fflush(pLogFile);
        // for IDE: flash(not target burn) one dot is always 8k
        fprintf(pLogFile, "\nWRITESIZE: 8192\n");
        fflush(pLogFile);

        if (guiMeasureTime)
                gettimeofday (&burn_begin_time, NULL);

        Ret = gpFlash->flash_program(FlashAddr, pDataStart, DataSize);

        if (guiMeasureTime) {
                gettimeofday (&burn_end_time, NULL);

                timeval_diff (&burn_total_time, &burn_begin_time, &burn_end_time);
                burn_time_sec = burn_total_time.tv_sec + (double)burn_total_time.tv_usec / 1000000;

                fprintf(pLogFile, "\n\nTotal burn time: %lu sec, %lu usec\n",
                        burn_total_time.tv_sec, burn_total_time.tv_usec);
                fprintf(pLogFile, "Average burn rate: %.2lf KBytes/s\n", (double)(DataSize >> 10) / burn_time_sec);
                fflush(pLogFile);
        }
        if (Ret == 0) {
                fprintf(pLogFile, "\nburn success.\n");
                fflush(pLogFile);
        } else {
                fprintf(pLogFile, "\nburn fail !!!\n");
                fflush(pLogFile);
                close_connection ();
                exit (0);
        }
        return Ret;
}

int nds32_verify(unsigned int FlashAddr, unsigned char *pDataStart, unsigned int DataSize)
{
        struct timeval verify_begin_time;
        struct timeval verify_end_time;
        struct timeval verify_total_time;
        double verify_time_sec;
        int Ret;

        fprintf(pLogFile, "verifying\n");
        fflush(pLogFile);

        if (guiMeasureTime)
                gettimeofday (&verify_begin_time, NULL);

        Ret = Verification(FlashAddr, pDataStart, DataSize);

        if (guiMeasureTime) {
                gettimeofday (&verify_end_time, NULL);

                timeval_diff (&verify_total_time, &verify_begin_time, &verify_end_time);
                verify_time_sec = verify_total_time.tv_sec + (double)verify_total_time.tv_usec / 1000000;

                fprintf(pLogFile, "\n\nTotal verify time: %lu sec, %lu usec\n",
                        verify_total_time.tv_sec, verify_total_time.tv_usec);
                fprintf(pLogFile, "Average verify rate: %.2lf KBytes/s\n", (double)(DataSize >> 10) / verify_time_sec);
                fflush(pLogFile);
        }

        if (Ret == 0)
                fprintf(pLogFile, "\nVerify success.\n");
        else
                fprintf(pLogFile, "\nVerify fail !!!\n");

        fflush(pLogFile);
        return Ret;
}

/* Help menu */
int nds32_usage(void)
{
        printf( "--host(-H):\t\t\tHost name/ip to connect with ICEman\n");
        printf( "--port(-p):\t\t\tPort number to connect with ICEman (default to 2354)\n");
        //printf( "--base(-b):\t\t\tFlash base address (default to 0x0)\n");
        printf( "--addr(-a):\t\t\tFlash target address to write (default to 0x0)\n");
        printf( "--image(-i):\t\t\tImage name to burn\n");

        //printf( "--preserve(-e):\t\tPreserve the content in first and last erase blocks\n");
        printf( "--log(-l):\t\t\tThe log file to store output message (default to stdout)\n");
        printf( "--reset-and-run(-t):\t\tReset target\n");
        printf( "--reset-and-hold(-o):\t\tReset target and stop at $IVB\n");
        //printf( "--target(-x):\t\tBoard type for address mapping (ag101p_16mb)\n");
        printf( "--verify(-v):\t\t\tVerify after flash burning\n");
        printf( "--verify-only(-c):\t\tUse image file to verify content of ROM, no burn\n");
        printf( "--version(-V):\t\t\tShow flash burning version\n");
        printf( "--unlock(-y):\t\t\tUnlock flash before burning\n");
        printf( "--lock(-z):\t\t\tLock flash after burning\n");
#if SPI_BURN
#else
        //printf( "--fast:\t\t\tBurning flash in fast mode\n");
#endif
        printf( "--erase-all(-u):\t\tErase entire flash before burning\n");
        printf( "--help(-h):\t\t\tThe usage for NDS32 Burner\n");
        printf( "--measure-time(-m):\t\tEstimate total time to burn image to ROM\n");
        printf( "--core-num(-n):\t\t\tSelect Target number\n");
        printf( "--target-num(-n):\t\tSelect Target number\n");
        printf( "--target(-N):\t\t\tSelect Target name\n");
        //printf( "--test-custom-monitor(-C):\tTest custom monitor command\n");
        //printf( "--multiout(-T):\t\tUse multiout mode\n");
        return 0;
}

/* record the flash memory content to buffer */
int Record_content(unsigned int address, unsigned char* buffer, unsigned int size)
{
        unsigned char content[2048];
        unsigned int buffer_size, i, j = 0;

        while (size > 0) {
                if (size > 1024)
                        buffer_size = 1024;
                else
                        buffer_size = size;

                /* fastin(address, buffer_size, content); */
                gpFlash->flash_read(address, &content[0], buffer_size);
                for (i = 0; i < buffer_size; i++) {
                        buffer[j++] = (unsigned char)content[i];
                }
                size = size - buffer_size;
                address = address + buffer_size;
        }
        return 0;
}

/* verify after burn */
/* return 1 if varify fail */
int Verification(unsigned int address, unsigned char* start, unsigned int size)
{
        unsigned char content[2048];
        unsigned int buffer_size, i;

        while (size > 0) {
                if (size > 1024)
                        buffer_size = 1024;
                else
                        buffer_size = size;
                /* fastin(address, buffer_size, content); */
                gpFlash->flash_read(address, &content[0], buffer_size);
                for (i = 0; i < buffer_size; i++) {
                        if ((unsigned char)content[i] != *(start++)) {
                                start--;
                                printf("Verification: ERROR i=%x, address=%x, start=%x content=%x \n", i, address, *start, content[i]);
                                return 1;
                        }
                }
                size = size - buffer_size;
                address = address + buffer_size;

                if ((address % (8*1024)) == 0) {	//IDE use 8K/dot
                        fprintf(pLogFile, ".");
                        fflush(pLogFile);
                }
        }
        return 0;
}

int check_delay_count(unsigned int address, unsigned char* start, unsigned int size)
{
        unsigned char content[1024];
        unsigned int buffer_size;
        
        if (size < 0) {
            printf("check_delay_count: ERROR image size < 0 \n");
            return 1;
        }
        if (size > 1024)
            buffer_size = 1024;
        else
            buffer_size = size;
        gpFlash->flash_read(address, &content[0], buffer_size);
     
        return 0;
}

int nds32_idlm_testing(void)
{
        unsigned int testData, testAddr, testSize, i;

        testSize = 0x100;
        // ILM testing
        testAddr = 0x0;
        mem_select_mode(NDS_MEMORY_SELECT_ILM);
        for (i = 0; i < testSize; i++) {
                outw(testAddr, i);
                testData = inw(testAddr);
                if (testData != i) {
                        printf("ilm_testing: ERROR, src:%x, exp:%x \n", testData, i);
                        return 1;
                } else {
                        printf(".");
                }
                testAddr += 4;
        }
        printf("ilm_testing: success\n");

        // DLM testing
        testAddr = 0x200000;
        mem_select_mode(NDS_MEMORY_SELECT_DLM);
        for (i = 0; i < testSize; i++) {
                outw(testAddr, i);
                testData = inw(testAddr);
                if (testData != i) {
                        printf("dlm_testing: ERROR, src:%x, exp:%x \n", testData, i);
                        return 1;
                } else {
                        printf(".");
                }
                testAddr += 4;
        }
        printf("dlm_testing: success\n");
        mem_select_mode(NDS_MEMORY_SELECT_BUS);
        return 0;
}

int test_monitor_cmd_0(unsigned int address)
{
        char *monitor_cmd = NULL;
        char *ret_data = NULL;
        int i;
        int Ret = 0;

        //Prepare monitor command #0: from address, write/read 100 Bytes
        monitor_cmd = malloc(113*sizeof(char)); //< CMD 1B, COREID 4B, ADDRESS 4B, SIZE 4B, DATA 100B
        if ( monitor_cmd != NULL ) {
                monitor_cmd[0] = 0;                 //CMD=0x0
                set_u32(monitor_cmd+1, coreid) ;    //COREID
                set_u32(monitor_cmd+5, address);    //MEM ADDRESS
                set_u32(monitor_cmd+9, 100);        //LENGTH: write 100 Bytes

                srand(time(NULL));
                for ( i = 0; i < 100; i++ )
                        monitor_cmd[13+i] = rand()%100+1;    // Prepare data in 1~100


                //issue monitor command
                if ( send_monitor_cmd(monitor_cmd, 113, &ret_data) < 0 ) {
                        Ret = 1;  //ERROR
                        goto MONITOR_ERROR;
                }

                //compare to bin_file
                int ret_size = get_u32(ret_data);
                fprintf(pLogFile, "Compare size: %d Bytes\n", ret_size);
                fflush(pLogFile);

                for ( i = 0; i < ret_size; i++ ) {
                        if ( (unsigned char)ret_data[4+i] != (unsigned char)monitor_cmd[13+i] )  {
                                printf("Monitor Verification ERROR: i=0x%X, address=0x%X, ori_data=0x%X now_data=0x%X \n",
                                       i, address, monitor_cmd[13+i], ret_data[4+i]);

                                Ret = 1; //ERROR
                                break;
                        }
                }

                free(monitor_cmd);
                monitor_cmd = NULL;
                free(ret_data);
                ret_data = NULL;
        } else {
                fprintf(pLogFile, "Test Monitor Command #0: failed to allocate command buffer\n");
                fflush(pLogFile);
                Ret = 1;
        }

MONITOR_ERROR:
        if ( Ret == 0 ) {
                fprintf(pLogFile, "Monitor Command success.\n");
        } else {
                fprintf(pLogFile, "Monitor Command fail !!!\n");
        }
        fflush(pLogFile);

        return Ret;
}

char *nds32_change_filepath(char *ori_filename)
{
        unsigned int filename_length, i;
        char *cur_str, *resolved_path = NULL, *new_filename = NULL;

        if (ori_filename == NULL)
                return NULL;

        filename_length = strlen(ori_filename);
        resolved_path = (char *)malloc(filename_length + 512);
        if (resolved_path == NULL)
                return NULL;

       //_fullpath cannot process /cygdrive path,so skip
       //_fullpath issue: cannot process on windows path: ../../d/test.bin(/cygdrive/d/test.bin)
        if (strncmp(ori_filename, "/cygdrive/", 10) != 0) {
    #ifdef __MINGW32__
                if (_fullpath(resolved_path, ori_filename, filename_length + 512) == NULL ) {
                        fprintf(pLogFile, "Error : file %s is not exist\n", ori_filename);
                        exit(1);
                }
    #else
                if (realpath(ori_filename, resolved_path) == NULL) {
                        fprintf(pLogFile, "Error : file %s is not exist\n", ori_filename);
                        exit(1);
                }
    #endif
        } else {
                resolved_path = strdup(ori_filename);
		}

        free(ori_filename);
        //fprintf(pLogFile, "resolved_file_path: %s \n", resolved_path);
        
        filename_length = strlen(resolved_path);
        new_filename = (char *)malloc(filename_length + 128);
		if (new_filename == NULL)
                return NULL;
		cur_str = new_filename;
        
        for (i = 0; i < filename_length; i++) {
            if (resolved_path[i] == 0x5c) {  //  0x5c => '\'
                *cur_str++ = '/';
            } else if (resolved_path[i] == ' ') {
                *cur_str++ = 0x5c;
                *cur_str++ = ' ';
            } else {
                *cur_str++ = resolved_path[i];
            }
        }
        *cur_str++ = 0;
        free(resolved_path);

        //fprintf(pLogFile, "new_filename: %s \n", new_filename); 
        return new_filename;
}
