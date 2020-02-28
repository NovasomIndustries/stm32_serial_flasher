#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>


#define ACK             0x79
#define GET_CMD         0x00
#define GET_OPT         0x01
#define GET_ID          0x02
#define READ_CMD        0x11
#define EXECUTE_CMD     0x21
#define WRITE_CMD       0x31
#define ERASE_CMD       0x43
#define EXT_ERASE_CMD   0x44
#define WRITE_UNPROTECT_CMD   0x73

#define READ_PROTECT_CMD   0x82
#define READ_UNPROTECT_CMD 0x92
#define BUFSIZE         65536
#define STM32_BUFSIZE   256
#define FLASH_ADDR      0x08000000

extern  struct termios oldtio,newtio,tmptio;
extern  unsigned char device_flash[1024*1024];
extern  unsigned char tx_buffer[STM32_BUFSIZE*2];
extern  unsigned char rx_buffer[STM32_BUFSIZE*2];
extern  int  bytes_read, ext_erase , flash_size;
extern  int pid;

extern  unsigned char   write_array[BUFSIZE];
extern  int             array_len;
