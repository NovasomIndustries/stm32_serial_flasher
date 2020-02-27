#include "stm32_serial_flasher.h"
#include "serial.h"

struct termios oldtio,newtio,tmptio;
unsigned char device_flash[1024*1024];
unsigned char tx_buffer[STM32_BUFSIZE*2];
unsigned char rx_buffer[STM32_BUFSIZE*2];
int  bytes_read, ext_erase=0 , flash_size;
int pid;

unsigned char   write_array[BUFSIZE];
int             array_len;

int autobaud(int fd)
{
    tx_buffer[0] = 0x7f;
    write(fd,tx_buffer,1);
    bytes_read = read(fd,&rx_buffer,1);
    if ( rx_buffer[0] == 0x79)
    {
        return 0;
    }
    printf("Autobaud failed\n");
    return 1;
}

int send_1byte_wait_ack(int fd, unsigned char byte1)
{
unsigned char cks = byte1 ^ 0xff;
    tx_buffer[0] = byte1;
    tx_buffer[1] = cks;
    write(fd,tx_buffer,2);
    rx_buffer[0] = 0;
    bytes_read = read(fd,&rx_buffer,32);
    if ( rx_buffer[0] == ACK )
        return 0;
    return 1;
}

int send_2bytes_wait_ack(int fd, unsigned char byte1, unsigned char byte2)
{
unsigned char cks = byte1 ^ byte2;
    tx_buffer[0] = byte1;
    tx_buffer[1] = byte2;
    tx_buffer[2] = cks;
    write(fd,tx_buffer,3);
    rx_buffer[0] = 0;
    bytes_read = read(fd,&rx_buffer,32);
    if ( rx_buffer[0] == ACK )
        return 0;
    return 1;
}

int send_3bytes_wait_ack(int fd, unsigned char byte1, unsigned char byte2, unsigned char byte3)
{
unsigned char cks = byte1 ^ byte2 ^ byte3;
    tx_buffer[0] = byte1;
    tx_buffer[1] = byte2;
    tx_buffer[2] = byte3;
    tx_buffer[3] = cks;
    write(fd,tx_buffer,4);
    rx_buffer[0] = 0;
    bytes_read = read(fd,&rx_buffer,32);
    if ( rx_buffer[0] == ACK )
        return 0;
    return 1;
}

int send_4bytes_wait_ack(int fd, unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4)
{
unsigned char cks = byte1 ^ byte2 ^ byte3 ^ byte4;
    tx_buffer[0] = byte1;
    tx_buffer[1] = byte2;
    tx_buffer[2] = byte3;
    tx_buffer[3] = byte4;
    tx_buffer[4] = cks;
    write(fd,tx_buffer,5);
    rx_buffer[0] = 0;
    bytes_read = read(fd,&rx_buffer,1);
    if ( rx_buffer[0] == ACK )
        return 0;
    return 1;
}

int get_cmd(int fd)
{
    if ( send_1byte_wait_ack(fd,GET_CMD)==0)
    {
        printf("Bootloader version : 0x%02x\n",rx_buffer[2]);
        if ( rx_buffer[9] == EXT_ERASE_CMD)
            ext_erase = 1;
        return 0;
    }
    return 1;
}

char *get_device_string(int pid)
{
    if ( pid == 0x438 )
    {
        flash_size = 0xffff;
        return "STM32F303x4(6/8)/F334xx/F328xx";
    }
    return "Unknown";
}

int get_pidvid(int fd)
{
    if ( send_1byte_wait_ack(fd,GET_ID)==0)
    {
        pid = (rx_buffer[2] << 8 ) | rx_buffer[3];
        printf("PID : 0x%04x (%s)\n",pid,get_device_string(pid));

        return 0;
    }
    return 1;
}

void ack_error(char *caller)
{
    printf("Ack error in %s\n",caller);
    printf("Received 0x%02x\n",rx_buffer[0]);
}

int erase(int fd)
{
    if ( ext_erase == 1 )
    {
        if ( send_1byte_wait_ack(fd,EXT_ERASE_CMD)==0)
        {
            if ( send_2bytes_wait_ack(fd,0xff,0xff)==0)
                printf("Erase complete\n");
            else
                printf("Erase failed\n");
        }
        else
            ack_error("erase");
    }
    return 1;
}

int execute(int fd)
{
unsigned int addr=FLASH_ADDR;
    if ( send_1byte_wait_ack(fd,EXECUTE_CMD)==0)
    {
        if( send_4bytes_wait_ack(fd, addr >> 24,addr >> 16, addr >> 8, addr & 0xff) == 0 )
        {
            printf("Started\n");
            return 0;
        }
        else
            printf("Start failed\n");
    }
    else
        ack_error("erase");
    return 1;
}

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

int flash(int fd)
{
unsigned int addr=FLASH_ADDR, numblocks,block,j,pct=0, progress=0;
unsigned char cks=0;

    numblocks = (array_len/STM32_BUFSIZE) + 1;
    pct = 100/numblocks;
    progress = pct;

    for(block=0;block<numblocks;block++)
    {
        if ( send_1byte_wait_ack(fd,WRITE_CMD)==1)
        {
            ack_error("Sending write command");
            return 1;
        }
        printf("Address 0x%08x (%.2f%%)\r",addr, ((100.0f / array_len) * STM32_BUFSIZE)*(block));
        fflush(stdout);
        progress += pct;
        if( send_4bytes_wait_ack(fd, addr >> 24,addr >> 16, addr >> 8, addr & 0xff) == 1 )
        {
            ack_error("Sending address");
            return 1;
        }
        else
        {
            tx_buffer[0]=cks=0xff;
            for(j=0;j<STM32_BUFSIZE;j++)
            {
                tx_buffer[j+1] = write_array[j+(block*STM32_BUFSIZE)];
                cks ^=  tx_buffer[j+1];
            }
            tx_buffer[STM32_BUFSIZE+1] = cks;

            tcflush(fd, TCIFLUSH);
            if ( write(fd,tx_buffer,(STM32_BUFSIZE+2)) != (STM32_BUFSIZE+2))
                printf("Buffer truncated\n");
            bytes_read = read(fd,&rx_buffer,1);
            if ( rx_buffer[0] != ACK )
            {
                ack_error("Write");
                return 1;
            }
            addr += STM32_BUFSIZE;
        }
    }
    printf("Address 0x%08x (100%%)\n",addr);
    return 0;
}

void usage(void)
{
    printf("stm32_serial_flash [-ex] [-rw <filename>]\n");
    printf("    -e : erase\n");
    printf("    -w <file name> : writes device with file <filename>, binary only\n");
    printf("    -x : execute from 0x08000000\n");
}

int main(int argc, char **argv)
{
int     fd,res=0;
int     optflag=0;
char    c,filename[32];

    sprintf(filename,"Uninitialized");
    while ((c = getopt (argc, argv, ":er:w:x")) != -1)
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
        }
    }
    if ( argc == 1 )
    {
        usage();
        return 0;
    }

    fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
    if (fd <0)
    {
        perror(MODEMDEVICE);
        return -1;
    }
    serial_port_init(fd);

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
            case    2   :   if ( get_file(filename) == 0 )
                            {
                                printf("File size is %d\n",array_len);
                                erase(fd);
                                if ( flash(fd) == 0 )
                                {
                                    printf("File %s written, starting device\n",filename);
                                    execute(fd);
                                }
                                else
                                    printf("Error writing file %s\n",filename);
                            }
                            else
                                printf("Error opening %s\n",filename);
                            break;
            case    3   :   execute(fd);
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
