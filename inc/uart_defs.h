#ifndef UART_DEFS_H
#define UART_DEFS_H

// Endereço da ESP32
#define ADDRESS 0x01

// Endereços dos botões
const unsigned char GROUND_UP = 0x00;         // Byte do endereço de terréo sobe
const unsigned char FLOOR_ONE_DOWN = 0x01;    // Byte do endereço do andar 1 desce
const unsigned char FLOOR_ONE_UP = 0x02;      // Byte do endereço do andar 1 sobe
const unsigned char FLOOR_TWO_DOWN = 0x03;    // Byte do endereço do andar 2 desce
const unsigned char FLOOR_TWO_UP = 0x04;      // Byte do endereço do andar 2 sobe
const unsigned char FLOOR_THR_DOWN = 0x05;    // Byte do endereço do andar 3 desce
const unsigned char EMERGENCY = 0x06;         // Byte do botão de emergência
const unsigned char PANEL_GROUND = 0x07;      // Byte do botão do painel para o terréo
const unsigned char PANEL_FLOOR_ONE = 0x08;   // Byte do botão do painel para o andar 1
const unsigned char PANEL_FLOOR_TWO = 0x09;   // Byte do botão do painel para o andar 2
const unsigned char PANEL_FLOOR_THR = 0x0A;   // Byte do botão do painel para o andar 3

// Códigos
const unsigned char REQUEST = 0x23; // Byte de solicitação de dados
const unsigned char SEND = 0x16;    // Byte de enviar dados
const unsigned char READ = 0x03;    // Byte de leitura de registradores
const unsigned char WRITE = 0x06;   // Byte de escrita de registradores

// Sub-código
const unsigned char REQ_VALUE = 0xC1;       // Byte para solicitar valor do encoder
const unsigned char SEN_CONTROL = 0xC2;     // Byte de envio de sinal de controle
const unsigned char SEN_TEMPERATURE = 0xD1; // Byte de envio de temperatura ambiente

// Matricula
const unsigned char registration_number[] = {3, 4, 8, 0};

#endif