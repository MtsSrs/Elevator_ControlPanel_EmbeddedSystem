#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "uart.h"
#include "crc16.h"
#include "uart_defs.h"

int uart_file = -1;

void close_uart_file();
void open_uart_file();
int request_encoder();
void send_pwm(int pwm);
void send_temp(float temp);
unsigned char* read_register(unsigned char byteAdress, unsigned char byteQuantity);
void write_register(unsigned char byteAdress, unsigned char byteQuantity, unsigned char data);

void close_uart_file()
{
    if (uart_file != -1)
    {
        close(uart_file);
    }
}

void open_uart_file()
{
    uart_file = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_file == -1)
    {
        perror("Error opening UART file");
        return;
    }

    struct termios attrs;
    tcgetattr(uart_file, &attrs);

    attrs.c_iflag = IGNPAR;
    attrs.c_oflag = 0;
    attrs.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    attrs.c_lflag = 0;

    tcflush(uart_file, TCIFLUSH);
    tcsetattr(uart_file, TCSANOW, &attrs);
}

//Abrir e fechar a uart file
int request_encoder()
{
    unsigned char tx_buffer[255];
    unsigned short crc;
    int index = 0;
    tx_buffer[index++] = ADDRESS;

    tx_buffer[index++] = REQUEST;
    tx_buffer[index++] = REQ_VALUE;

    memcpy(&tx_buffer[index], registration_number, sizeof(registration_number));
    index += sizeof(registration_number);

    crc = calcula_CRC(tx_buffer, index);
    memcpy(&tx_buffer[index], &crc, sizeof(crc));
    index += sizeof(crc);

    int count = write(uart_file, &tx_buffer, index);

    usleep(50 * 1000);

    unsigned char response[255];
    int bytes_read = read(uart_file, response, sizeof(response));

    int value;
    memcpy(&value, &response[3], 4);
    return value;
}

void send_pwm(int pwm)
{
    unsigned char tx_buffer[255];
    unsigned short crc;
    int index = 0;
    tx_buffer[index++] = ADDRESS;

    tx_buffer[index++] = SEND;
    tx_buffer[index++] = SEN_CONTROL;

    memcpy(&tx_buffer[index], &pwm, sizeof(int));
    index += sizeof(int);

    memcpy(&tx_buffer[index], registration_number, sizeof(registration_number));
    index += sizeof(registration_number);

    crc = calcula_CRC(tx_buffer, index);
    memcpy(&tx_buffer[index], &crc, sizeof(crc));
    index += sizeof(crc);

    int count = write(uart_file, &tx_buffer, index);
    usleep(50 * 1000);
}

void send_temp(float temp)
{
    unsigned char tx_buffer[255];
    unsigned short crc;
    int index = 0;
    tx_buffer[index++] = ADDRESS;

    tx_buffer[index++] = SEND;
    tx_buffer[index++] = SEN_TEMPERATURE;

    memcpy(&tx_buffer[index], &temp, sizeof(float));
    index += sizeof(float);

    memcpy(&tx_buffer[index], registration_number, sizeof(registration_number));
    index += sizeof(registration_number);

    crc = calcula_CRC(tx_buffer, index);
    memcpy(&tx_buffer[index], &crc, sizeof(crc));
    index += sizeof(crc);

    int count = write(uart_file, &tx_buffer, index);

    usleep(50 * 1000);
}

unsigned char* read_register(unsigned char byteAdress, unsigned char byteQuantity)
{
    unsigned char tx_buffer[255];
    unsigned short crc;
    int index = 0;
    tx_buffer[index++] = ADDRESS;


    tx_buffer[index++] = READ;

    memcpy(&tx_buffer[index], &byteAdress, sizeof(unsigned char));
    index += sizeof(unsigned char);

    memcpy(&tx_buffer[index], &byteQuantity, sizeof(unsigned char));
    index += sizeof(unsigned char);
    

    memcpy(&tx_buffer[index], registration_number, sizeof(registration_number));
    index += sizeof(registration_number);

    crc = calcula_CRC(tx_buffer, index);
    memcpy(&tx_buffer[index], &crc, sizeof(crc));
    index += sizeof(crc);

    int count = write(uart_file, &tx_buffer, index);

    usleep(200 * 1000);

    unsigned char response[255];
    int bytes_read = read(uart_file, response, sizeof(response));

    unsigned char *value = malloc((bytes_read - 4) * sizeof(unsigned char));
    if (value == NULL) {
        return NULL;
    }

    for (int i = 0; i < bytes_read - 4; i++) {
        memcpy(&value[i], &response[i + 2], sizeof(unsigned char));
    }

    return value;
}

void write_register(unsigned char byteAdress, unsigned char byteQuantity, unsigned char data){
    unsigned char tx_buffer[255];
    unsigned short crc;
    int index = 0;
    tx_buffer[index++] = ADDRESS;


    tx_buffer[index++] = WRITE;

    memcpy(&tx_buffer[index], &byteAdress, sizeof(unsigned char));
    index += sizeof(unsigned char);

    memcpy(&tx_buffer[index], &byteQuantity, sizeof(unsigned char));
    index += sizeof(unsigned char);
    
    memcpy(&tx_buffer[index], &data, sizeof(unsigned char));
    index += sizeof(unsigned char);

    memcpy(&tx_buffer[index], registration_number, sizeof(registration_number));
    index += sizeof(registration_number);

    crc = calcula_CRC(tx_buffer, index);
    memcpy(&tx_buffer[index], &crc, sizeof(crc));
    index += sizeof(crc);

    int count = write(uart_file, &tx_buffer, index);

    usleep(50 * 1000);
}


