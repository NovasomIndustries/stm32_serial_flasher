#define BAUDRATE        B57600
#define MODEMDEVICE     "/dev/ttyUSB1"

extern  int serial_port_init(int fd);
extern  void close_serial_port(int fd);
