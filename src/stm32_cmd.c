#include "stm32_serial_flasher.h"
#include "serial.h"
#include "stm32_cmd.h"

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
    printf("NACK in %s\n",caller);
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

int write_unprotect(int fd)
{
    if ( send_1byte_wait_ack(fd,WRITE_UNPROTECT_CMD)==0)
    {
        rx_buffer[0] = 0;
        bytes_read = read(fd,&rx_buffer,1);
        if ( rx_buffer[0] == ACK )
        {
            usleep(200000);
            return autobaud(fd);
        }
    }
    return 1;
}

int read_unprotect(int fd)
{
    if ( send_1byte_wait_ack(fd,READ_UNPROTECT_CMD)==0)
    {
        rx_buffer[0] = 0;
        bytes_read = read(fd,&rx_buffer,1);
        if ( rx_buffer[0] == ACK )
        {
            usleep(200000);
            return autobaud(fd);
        }
    }
    return 1;
}

int flash(int fd)
{
unsigned int addr=FLASH_ADDR, numblocks,block,j;
unsigned char cks=0;

    numblocks = (array_len/STM32_BUFSIZE) + 1;

    for(block=0;block<numblocks;block++)
    {
        if ( send_1byte_wait_ack(fd,WRITE_CMD)==1)
        {
            ack_error("Sending write command");
            return 1;
        }
        printf("Address 0x%08x (%.2f%%)\r",addr, ((100.0f / array_len) * STM32_BUFSIZE)*(block));
        fflush(stdout);
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

int device_read(int fd)
{
unsigned int addr=FLASH_ADDR, numblocks,block;

    if ( read_unprotect(fd) != 0 )
    {
        ack_error("Read unprotect error");
        return 1;
    }
    numblocks = (65536/STM32_BUFSIZE);
    sleep(5);
    for(block=0;block<numblocks;block++)
    {
        if ( send_1byte_wait_ack(fd,READ_CMD)==1)
        {
            ack_error("Sending read command");
            return 1;
        }
        printf("Address 0x%08x (%.2f%%)\r",addr, ((100.0f / 65536) * STM32_BUFSIZE)*(block));
        fflush(stdout);
        if( send_4bytes_wait_ack(fd, addr >> 24,addr >> 16, addr >> 8, addr & 0xff) == 1 )
        {
            ack_error("Sending read address");
            return 1;
        }
        else
        {
            if ( send_1byte_wait_ack(fd,127)==1)
            {
                ack_error("Sending number of bytes to read command");
                return 1;
            }
            bytes_read = read(fd,&rx_buffer,1);
            addr += STM32_BUFSIZE;
        }
    }
    printf("Address 0x%08x (100%%)\n",addr);
    return 0;
}

