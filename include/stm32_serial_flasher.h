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

