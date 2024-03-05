#ifndef GPIO_H
#define GPIO_H

void initializeGPIO();
void* button_read_thread(void* arg);
void* elevator_control_thread(void* arg);
int calibrate();
void setEngineState(int direction);
void setEnginePower(int speed);
double elevatorPWMControl(int currentEncoderValue);
void elevator(int reference);
void pwmFlutuation(int currentEncoderValue, int direction);
void *send_pwm_thread(void *arg);
int elevator_status();

#endif