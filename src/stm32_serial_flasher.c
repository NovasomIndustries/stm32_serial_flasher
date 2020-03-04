#include "stm32_serial_flasher.h"
#include "serial.h"
#include "stm32_cmd.h"

struct termios oldtio,newtio,tmptio;
unsigned char device_flash[1024*1024];
unsigned char tx_buffer[STM32_BUFSIZE*2];
unsigned char rx_buffer[STM32_BUFSIZE*2];
int  bytes_read, ext_erase=0 , flash_size;
int pid;

unsigned char   write_array[BUFSIZE];
int             array_len;

int get_file(char *filename)
{
FILE *fp;
    fp = fopen(filename,"rb");
    if ( fp )
    {
        array_len = fread(write_array,1,BUFSIZE,fp);
        fclose(fp);
        return 0;
    }
    return 1;
}

void usage(void)
{
    printf("stm32_serial_flash [-ex] [-w <filename>]\n");
    printf("    -e : erase\n");
    printf("    -w <file name> : writes device with file <filename>, binary only\n");
    printf("    -x : execute from 0x08000000\n");
    printf("    -p <serial device>: use <serial device> instead/dev/ttyUSB1\n");
    printf("    -u : unprotect\n");
}

int main(int argc, char **argv)
{
int     fd,res=0;
int     optflag=0;
char    c,filename[32],serial_port[64];

    sprintf(filename,"Uninitialized");
    sprintf(serial_port,MODEMDEVICE);
    while ((c = getopt (argc, argv, "?exp:urw:")) != -1)
    {
        switch (c)
        {
            case 'e'    :   optflag = 1;
                            break;
            case 'w'    :   optflag = 2;
                            sprintf(filename,"%s",optarg);
                            break;
            case 'x'    :   optflag = 3;
                            break;
            case 'p'    :   sprintf(serial_port,"%s",optarg);
                            break;
            case 'u'    :   optflag = 4;
                            break;
            case 'r'    :   optflag = 5;
                            break;
            case '?'    :   usage(); return -1;
            default     :   usage(); return -1;
        }
    }

    if ( optflag == 2)
    {
        if ( get_file(filename) != 0 )
        {
            printf("Error opening %s\n",filename);
            return -1;
        }
    }
    fd = open(serial_port, O_RDWR | O_NOCTTY );
    if (fd <0)
    {
        perror(serial_port);
        return -1;
    }
    if ( serial_port_init(fd) )
    {
        printf("Error opening %s\n",serial_port);
        return -1;
    }

    res = autobaud(fd);

    if ( res == 0)
    {
        tcflush(fd, TCIFLUSH);
        get_cmd(fd);
        get_pidvid(fd);
        switch (optflag)
        {
            case    1   :   erase(fd);
                            break;
            case    2   :   printf("File size is %d\n",array_len);
                            erase(fd);
                            if ( flash(fd) == 0 )
                            {
                                printf("File %s written, starting device\n",filename);
                                execute(fd);
                            }
                            else
                                printf("Error writing file %s\n",filename);
                            break;
            case    3   :   execute(fd);
                            break;
            case    4   :   write_unprotect(fd);
                            break;
            case    5   :   device_read(fd);
                            break;
            default     :   printf("Invalid option\n");
                            break;
        }
    }
    else
        printf("No device found\n");
    close_serial_port(fd);
    close(fd);
}
