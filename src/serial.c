#include "stm32_serial_flasher.h"
#include "serial.h"

extern  struct termios oldtio,newtio,tmptio;

int serial_port_init(int fd)
{
    tcgetattr(fd,&oldtio); /* save current port settings */
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD | INPCK | PARENB;
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY | IGNPAR);
    newtio.c_lflag &= ~(ECHOK | ECHOCTL | ECHOKE);
    newtio.c_oflag &= ~(OPOST | ONLCR);
    newtio.c_cc[VTIME]    = 255;   /* inter-character timer .5 second */
    newtio.c_cc[VMIN]     = 0;   /* non blocking read */
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
    tcgetattr(fd, &tmptio);
    if (newtio.c_iflag != tmptio.c_iflag ||
        newtio.c_oflag != tmptio.c_oflag ||
        newtio.c_cflag != tmptio.c_cflag ||
        newtio.c_lflag != tmptio.c_lflag)
    {
        printf("Port not set\n");
        return 1;
    }
    return 0;
}

void close_serial_port(int fd)
{
    tcsetattr(fd,TCSANOW,&oldtio);
}
