#include <wiringPi.h>
#include <softPwm.h>
#include <math.h>
#include <pthread.h>
#include "gpio.h"
#include "gpio_defs.h"
#include "uart.h"
#include "pid.h"

// Alturas
int groundHeight = 0;
int floorOneHeight = 0;
int floorTwoHeight = 0;
int floorThrHeight = 0;

// Variáveis do PID 
double saidaMedida = 0.0;
double sinalDeControle = 0.0;

int pwmValue = 0;
int newValueAvailable = 0;
int queue[NUM_BUTTONS]; 
int queueSize = 0;
int elevatorState = 0; 
int calibrated = 0;
int currentElevatorState = 0;

pthread_t pwmThread, buttonThread, elevatorControlThread;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;

typedef enum {
    UP_GROUND,
    DOWN_FIRST_FLOOR,
    UP_FIRST_FLOOR,
    DOWN_SECOND_FLOOR,
    UP_SECOND_FLOOR,
    DOWN_THIRD_FLOOR,
    EMERGENCY,
    PAINEL_GROUND,
    PAINEL_FIRST_FLOOR,
    PAINEL_SECOND_FLOOR,
    PAINEL_THIRD_FLOOR
} ElevatorCommand;

// Prototipo
void initializeGPIO();
void enqueue(ElevatorCommand command);
ElevatorCommand dequeue();
ElevatorCommand mapearIndiceParaComando(int indice);
void* button_read_thread(void* arg);
void* elevator_control_thread(void* arg);   
int calibrate();
void setEngineState(int direction);
void setEnginePower(int speed);
double elevatorPWMControl(int currentEncoderValue);
void elevator(int reference);
void pwmFlutuation(int currentEncoderValue, int direction);
void *send_pwm_thread(void *arg);

void initializeGPIO()
{
    pinMode(ENGINE_DIR1, OUTPUT);
    pinMode(ENGINE_DIR2, OUTPUT);
    pinMode(ENGINE_POWER, OUTPUT);
    pinMode(SENSOR_GROUND, INPUT);
    pinMode(SENSOR_FLOOR_ONE, INPUT);
    pinMode(SENSOR_FLOOR_TWO, INPUT);
    pinMode(SENSOR_FLOOR_THR, INPUT);
    pullUpDnControl(SENSOR_GROUND, PUD_DOWN);
    pullUpDnControl(SENSOR_FLOOR_ONE, PUD_DOWN);
    pullUpDnControl(SENSOR_FLOOR_TWO, PUD_DOWN);
    pullUpDnControl(SENSOR_FLOOR_THR, PUD_DOWN);

    pid_configura_constantes(0.5, 0.05, 40);
    softPwmCreate(ENGINE_POWER, 0, 100); // Inicializado com ciclo de trabalho zero (motor desligado)

    pthread_create(&pwmThread, NULL, send_pwm_thread, NULL);
    pthread_create(&buttonThread, NULL, button_read_thread, NULL);
    pthread_create(&elevatorControlThread, NULL, elevator_control_thread, NULL);
}

void enqueue(ElevatorCommand command)
{
    pthread_mutex_lock(&queueLock);
    if (queueSize < NUM_BUTTONS) {
        queue[queueSize++] = command;
        pthread_cond_signal(&queueCond);
    }
      for (int i = 0; i < queueSize; i++) {
        printf("Queue[%d]: Floor %d\n", i, queue[i]);
    }
    pthread_mutex_unlock(&queueLock);
}

ElevatorCommand dequeue()   
{
    ElevatorCommand command = -1;
    pthread_mutex_lock(&queueLock);
    while (queueSize == 0) {
        pthread_cond_wait(&queueCond, &queueLock);
    }
    command = queue[0];
    for (int i = 0; i < queueSize - 1; i++) {
        queue[i] = queue[i + 1];
    }
    queueSize--;
    pthread_mutex_unlock(&queueLock);
    return command;
}

ElevatorCommand mapearIndiceParaComando(int indice)
{
    switch (indice) {
        case 0:
            return UP_GROUND;
        case 1:
            return DOWN_FIRST_FLOOR;
        case 2:
            return UP_FIRST_FLOOR;
        case 3:
            return DOWN_SECOND_FLOOR;
        case 4:
            return UP_SECOND_FLOOR;
        case 5:
            return DOWN_THIRD_FLOOR;
        case 6:
            return EMERGENCY;
        case 7:
            return PAINEL_GROUND;
        case 8:
            return PAINEL_FIRST_FLOOR;
        case 9:
            return PAINEL_SECOND_FLOOR;
        case 10:
            return PAINEL_THIRD_FLOOR;
    }
}

void* button_read_thread(void* arg) {
    while (1) {
        if (!calibrated) {
            usleep(500 * 1000); // Espera por 500 ms e tenta novamente
            continue;
        }
        pthread_mutex_lock(&queueLock); 
        if (elevatorState == 0) {  
            pthread_mutex_unlock(&queueLock); 
            
            unsigned char* buttonStates = read_register(0x00, 0x0B);
            unsigned char true = 0x01;
            if (buttonStates != NULL) {
                for (int i = 0; i < NUM_BUTTONS; i++) {
                    if (buttonStates[i] == true) {
                        ElevatorCommand comando = mapearIndiceParaComando(i);
                        enqueue(comando);
                    }
                }
                free(buttonStates);
            }

        } else {
            pthread_mutex_unlock(&queueLock);
        }
        usleep(200 * 1000); 
    }
    return NULL;
}

void* elevator_control_thread(void* arg) {
    while (1) {
        ElevatorCommand command = dequeue();

        pthread_mutex_lock(&queueLock);
        elevatorState = 1; 
        pthread_mutex_unlock(&queueLock);

        switch (command) {
            case UP_GROUND:
                elevator(groundHeight);
                write_register(0x00, 0x01, 0x00);
                break;
            case DOWN_FIRST_FLOOR:
                elevator(floorOneHeight);
                write_register(0x01, 0x01, 0x00);
                break;
            case UP_FIRST_FLOOR:
                elevator(floorOneHeight);
                write_register(0x02, 0x01, 0x00);
                break;
            case DOWN_SECOND_FLOOR:
                elevator(floorTwoHeight);
                write_register(0x03, 0x01, 0x00);
                break;
            case UP_SECOND_FLOOR:
                elevator(floorTwoHeight);
                write_register(0x04, 0x01, 0x00);
                break;
            case DOWN_THIRD_FLOOR:
                elevator(floorThrHeight);
                write_register(0x05, 0x01, 0x00);
                break;
            case EMERGENCY:
                elevator(groundHeight);
                write_register(0x06, 0x01, 0x00);
                break;
            case PAINEL_GROUND:
                elevator(groundHeight);
                write_register(0x07, 0x01, 0x00);
                break;
            case PAINEL_FIRST_FLOOR:
                elevator(floorOneHeight);
                write_register(0x08, 0x01, 0x00);
                break;
            case PAINEL_SECOND_FLOOR:
                elevator(floorTwoHeight);
                write_register(0x09, 0x01, 0x00);
                break;
            case PAINEL_THIRD_FLOOR:
                elevator(floorThrHeight);
                write_register(0x0A, 0x01, 0x00);
                break;
        }
        pthread_mutex_lock(&queueLock);
        elevatorState = 0; 
        pthread_mutex_unlock(&queueLock);
    }
    return NULL;
}

void *send_pwm_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&lock);
        while (!newValueAvailable)
        {
            pthread_cond_wait(&cond, &lock);
        }
        send_pwm(pwmValue); // Envia o valor PWM
        newValueAvailable = 0;
        pthread_mutex_unlock(&lock);

        usleep(200 * 1000);  // Espera por 200 ms (200000 microssegundos)
    }
    return NULL;
}

double elevatorPWMControl(int currentEncoderValue)
{
    int aux;
    saidaMedida = (double)currentEncoderValue;
    sinalDeControle = pid_controle(saidaMedida);
    aux = abs((int)sinalDeControle);
    setEnginePower(aux);

    pthread_mutex_lock(&lock);
        pwmValue = aux; // Define o valor de PWM
        newValueAvailable = 1;
        pthread_cond_signal(&cond); // Sinaliza a thread send_pwm
    pthread_mutex_unlock(&lock);
    
    return sinalDeControle;
}

int calibrate()
{
    int currentEncoderValue;
    setEngineState(1);
    setEnginePower(10);
    
    printf("Calibrating, please wait...\n");
    printf("Checking sensors...\n");
    while (1)
    {
        if (digitalRead(SENSOR_GROUND) == HIGH)
        {
            groundHeight = request_encoder();
            printf("Ground floor detected. Height: %d\n", groundHeight);
        }
        if (digitalRead(SENSOR_FLOOR_ONE) == HIGH)
        {
            floorOneHeight = request_encoder();
            printf("First floor detected. Height: %d\n", floorOneHeight);
        }
        if (digitalRead(SENSOR_FLOOR_TWO) == HIGH)
        {
            floorTwoHeight = request_encoder();
            printf("Second floor detected. Height: %d\n", floorTwoHeight);
        }
        if (digitalRead(SENSOR_FLOOR_THR) == HIGH)
        {
            floorThrHeight = request_encoder();
            printf("Third floor detected. Height: %d. Breaking loop.\n", floorThrHeight);
            calibrated = 1;
            break;
        }
        usleep(200 * 1000);
    }

    printf("Calibration complete.\n");
    printf("Ground floor height: %d\n", groundHeight);
    printf("First floor height: %d\n", floorOneHeight);
    printf("Second floor height: %d\n", floorTwoHeight);
    printf("Third floor height: %d\n", floorThrHeight);

    setEngineState(0);
    setEnginePower(0);
    return 1;
}

void elevator(int reference)
{
    pid_atualiza_referencia((float)reference);
    int currentEncoderValue;
    int direction;

    currentEncoderValue = request_encoder();

    // Realiza a aproximação para a referência
    while (currentEncoderValue != reference && abs(currentEncoderValue - reference) > 10)
    {
        currentEncoderValue = request_encoder();
        pwmFlutuation(currentEncoderValue, direction);
    }

    setEngineState(0);
}

void setEngineState(int direction)
{
    if (direction == 1)
    {
        // Motor Sobe
        digitalWrite(ENGINE_DIR1, HIGH);
        digitalWrite(ENGINE_DIR2, LOW);
        currentElevatorState = 1;
    }
    else if (direction == 2)
    {
        // Motor Desce
        digitalWrite(ENGINE_DIR1, LOW);
        digitalWrite(ENGINE_DIR2, HIGH);
        currentElevatorState = 2;
    }
    else if (direction == 0)
    {
        // Motor Para
        digitalWrite(ENGINE_DIR1, HIGH);
        digitalWrite(ENGINE_DIR2, HIGH);
        currentElevatorState = 0;
    }
    else if (direction == 3)
    {
        // Motor Livre
        digitalWrite(ENGINE_DIR1, LOW);
        digitalWrite(ENGINE_DIR2, LOW);
        currentElevatorState = 3;
    }
}

void setEnginePower(int speed)
{
    // Ajustar a velocidade do motor usando PWM
    softPwmWrite(ENGINE_POWER, speed);
}

void pwmFlutuation(int currentEncoderValue, int direction)
{
    direction = elevatorPWMControl(currentEncoderValue);
    if (direction > 0)
    {
        setEngineState(1);
    } else if (direction < 0)
    {
        setEngineState(2);
    } else if (direction == 0){
        setEngineState(0);
    }
}

int elevator_status(){
    int state = currentElevatorState;
    return state;
}