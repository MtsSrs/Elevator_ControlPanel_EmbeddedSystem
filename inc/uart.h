#ifndef UART_H
#define UART_H

void close_uart_file();
void open_uart_file();
int request_encoder();
void send_pwm(int pwm);
void send_temp(float temp);
unsigned char* read_register(unsigned char byteAdress, unsigned char byteQuantity);
void write_register(unsigned char byteAdress, unsigned char byteQuantity, unsigned char data);

#endif