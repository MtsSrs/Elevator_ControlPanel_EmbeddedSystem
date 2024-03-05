#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "uart.h"
#include "bme280.h"
#include "temp.h"
#include "gpio.h"
#include "control_lcd_16x2.h"

pthread_t tempThread;
struct bme280_dev bme_connection;
int control = 0;

void startProcess();
void endProcess(int signum);
void menu();
void* temperature_thread(void* arg);
void display(const char *buffer1, const char *buffer2, float temp, int state);

int main()
{
    signal(SIGINT, endProcess);
    startProcess();
    menu();
    return 0;
}

void startProcess()
{
    wiringPiSetup();
    initializeGPIO();
    lcd_init();
    ClrLcd();
    bme_connection = connectBme();
    open_uart_file();
    pthread_create(&tempThread, NULL, temperature_thread, NULL);
}

void endProcess(int signum) {
    printf("Ctrl+C pressionado. Terminando processo\n");
    close_uart_file();
    exit(0);
}

void menu()
{
    float temp;
    while (1)
    {
        printf("What would you like to do?(number only):\n");
        printf("1 - Request Encoder Value\n");
        printf("2 - Send PWM control signal\n");
        printf("3 - Send room temperature\n");
        printf("4 - Read the registers\n");
        printf("5 - Write in the registers\n");
        printf("6 - Exit\n");
        printf("7 - Elevator control\n");
        printf("8 - Calibrate\n");
        printf("9 - Floor command\n");
        int command;
        scanf("%d", &command);

        if (command == 6)
        {
            printf("Bye o/\n");
            break;
        }
        else if (command == 1)
        {
            request_encoder();
        }
        else if (command == 2)
        {
            send_pwm(50);
        }
        else if (command == 3)
        {
            break;
        }
        else if (command == 4)
        {
            unsigned char byteAdress;
            unsigned char byteQuantity;
            unsigned char *values;
            printf("Write the byte Adress (0x00 ~ 0x0B) and the quantity (1 - 11)\n");
            scanf("%hhx %hhx", &byteAdress, &byteQuantity);
            values = read_register(byteAdress, byteQuantity);
            for (int i = 0; i < byteQuantity; i++) {
                printf("0x%x ", values[i]);
            }
            free(values);
        }
        else if (command == 5)
        {
            unsigned char byteAdress;
            unsigned char byteQuantity;
            unsigned char data;
            printf("Write the byte Adress (0x00 ~ 0x0A) and the quantity (1 - 11) and data \n");
            scanf("%hhx %hhx %hhx", &byteAdress, &byteQuantity, &data);
            write_register(byteAdress, byteQuantity, data);
        }
        else if (command == 7)
        {
            int state;
            int speed;
            printf("Digite o estado do motor\n");
            scanf("%d", &state);
            printf("Digite a velocidade do motor\n");
            scanf("%d", &speed);
            setEngineState(state);
            setEnginePower(speed);
        }
        else if (command == 8)
        {
            control = calibrate();
        }
        else if (command == 9)
        {
            int floor;
            scanf("%d", &floor);
            elevator(floor);
        }
        printf("Please wait...\n");
        sleep(1); 
    }
}


void* temperature_thread(void* arg) {
    char buffer1[20];
    char buffer2[20];
    while (1) {
        if (!control) {
            usleep(500 * 1000); // Espera por 500 ms e tenta novamente
            continue;
        }
        float temp = getCurrentTemperature(&bme_connection); 
        send_temp(temp);
        int state = elevator_status();
        //display
        display(buffer1, buffer2, temp, state);
        usleep(1000 * 1000); 
    }
    return NULL;
}

void display(const char *buffer1, const char *buffer2, float temp, int state){
    sprintf(buffer1, "Temp: %.2f C", temp);
    lcdLoc(LINE1);
    typeln(buffer1);

    switch (state) {
            case 0:
                sprintf(buffer2, "Elevador Parado");
                break;
            case 1:
                sprintf(buffer2, "Elevador Subindo");
                break;
            case 2:
                sprintf(buffer2, "Elevador Descendo");
                break;
            case 3:
                sprintf(buffer2, "Elevador Livre");
                break;
    }  
        lcdLoc(LINE2); 
        typeln(buffer2);
}