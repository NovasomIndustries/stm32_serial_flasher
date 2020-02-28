extern  int autobaud(int fd);
extern  int send_1byte_wait_ack(int fd, unsigned char byte1);
extern  int send_2bytes_wait_ack(int fd, unsigned char byte1, unsigned char byte2);
extern  int send_3bytes_wait_ack(int fd, unsigned char byte1, unsigned char byte2, unsigned char byte3);
extern  int send_4bytes_wait_ack(int fd, unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4);
extern  int get_cmd(int fd);
extern  char *get_device_string(int pid);
extern  int get_pidvid(int fd);
extern  void ack_error(char *caller);
extern  int erase(int fd);
extern  int execute(int fd);
extern  int write_unprotect(int fd);
extern  int flash(int fd);

