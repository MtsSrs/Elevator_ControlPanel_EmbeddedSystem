#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <wiringPiI2C.h>
#include <wiringPi.h>

#define I2C_ADDR   0x27
#define LCD_CHR    1
#define LCD_CMD    0
#define LINE1      0x80
#define LINE2      0xC0
#define LCD_BACKLIGHT   0x08
#define ENABLE     0b00000100

void lcd_init(void);
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void typeInt(int i);
void typeFloat(float myFloat);
void lcdLoc(int line);
void ClrLcd(void);
void typeln(const char *s);
void typeChar(char val);

#endif
