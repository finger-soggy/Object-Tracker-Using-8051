#include <reg51.h>
#include <stdio.h>
#include <math.h>

sfr lcd_data_port = 0xA0;  // P2 = LCD data

sbit rs = P3^2;
sbit rw = P3^3;
sbit en = P3^4;

sbit servo = P3^6;           // Servo motor control

// Ultrasonic Sensor 1 (Scanner)
sbit trig1 = P1^1;
sbit echo1 = P1^2;

// Ultrasonic Sensor 2 (Tracker)
sbit trig2 = P1^3;
sbit echo2 = P1^4;

#define PWM_PERIOD 18433
#define ONE_MS 922
#define SOUND_VELOCITY 34300
#define CLOCK_PERIOD_US 1.085e-6

unsigned int ON_Period, OFF_Period;
float distance1, distance2;
char buf[17];
unsigned char angle = 0;
bit sweep_dir = 1;  // 1 = forward, 0 = backward

// ================== Delay Functions ==================
void delay(unsigned int count) {
    int i, j;
    for (i = 0; i < count; i++)
        for (j = 0; j < 250; j++);
}

void delay_servo(unsigned int count) {
    unsigned int timer = 65536 - count;
    TH0 = timer >> 8;
    TL0 = timer;
    TR0 = 1;
    while (!TF0);
    TR0 = 0;
    TF0 = 0;
}

// ================== Servo ==================
void rotate(unsigned char angle) {
    ON_Period = ONE_MS + angle * 6;
    OFF_Period = PWM_PERIOD - ON_Period;
    servo = 1;
    delay_servo(ON_Period);
    servo = 0;
    delay_servo(OFF_Period);
}

// ================== LCD ==================
void LCD_Command(unsigned char cmd) {
    lcd_data_port = cmd;
    rs = 0; rw = 0; en = 1;
    delay(1);
    en = 0;
    delay(5);
}

void LCD_Char(unsigned char data_char) {
    lcd_data_port = data_char;
    rs = 1; rw = 0; en = 1;
    delay(1);
    en = 0;
    delay(5);
}

void LCD_String(unsigned char *str) {
    int i;
    for (i = 0; str[i]; i++)
        LCD_Char(str[i]);
}

void LCD_String_xy(unsigned char row, unsigned char pos, unsigned char *str) {
    if (row == 0) LCD_Command(0x80 + pos);
    else if (row == 1) LCD_Command(0xC0 + pos);
    LCD_String(str);
}

void LCD_Init() {
    delay(20);
    LCD_Command(0x38); LCD_Command(0x0C); LCD_Command(0x06);
    LCD_Command(0x01); LCD_Command(0x80);
}

// ================== Timer + Ultrasonic ==================
void init_timer() {
    TMOD = 0x01;
    TF0 = 0;
    TR0 = 0;
}

void delay_us() {
    TL0 = 0xF5;
    TH0 = 0xFF;
    TR0 = 1;
    while (TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

float measure_distance1() {
    unsigned int time;
    float distance;

    trig1 = 1;
    delay_us();
    trig1 = 0;

    while (!echo1);
    TR0 = 1;
    while (echo1 && !TF0);
    TR0 = 0;

    time = (TH0 << 8) | TL0;
    distance = (time * CLOCK_PERIOD_US * SOUND_VELOCITY) / 2.0;

    TF0 = 0;
    return distance;
}

float measure_distance2() {
    unsigned int time;
    float distance;

    trig2 = 1;
    delay_us();
    trig2 = 0;

    while (!echo2);
    TR0 = 1;
    while (echo2 && !TF0);
    TR0 = 0;

    time = (TH0 << 8) | TL0;
    distance = (time * CLOCK_PERIOD_US * SOUND_VELOCITY) / 2.0;

    TF0 = 0;
    return distance;
}

// ================== Main Program ==================
void main() {
    LCD_Init();
    init_timer();
    servo = 0;
    LCD_String_xy(0, 0, "Obj Tracking");

    while (1) {
        distance1 = measure_distance1();

        if (distance1 > 2 && distance1 < 20) {
            // Sensor 1 detected object ? move to 0° and hold
            angle = 0;
            rotate(angle);
            sprintf(buf, "Hold 0 D:%.0fcm", distance1);
            LCD_String_xy(1, 0, buf);

            // Now check sensor 2
            distance2 = measure_distance2();
            if (distance2 > 2 && distance2 < 20) {
                // Sensor 2 tracks object ? rotate toward 0° (corrected direction)
                for (angle = 180; angle >= 0; angle -= 5) {
                    rotate(angle);
                    distance2 = measure_distance2();
                    sprintf(buf, "Track %3d D%.0f", angle, distance2);
                    LCD_String_xy(1, 0, buf);

                    if (distance2 < 5 || distance2 > 100)
                        break;
                }
            }
        } else {
            // No object ? resume back-and-forth sweeping
            if (sweep_dir)
                angle += 5;
            else
                angle -= 5;

            if (angle >= 180) sweep_dir = 0;
            else if (angle <= 0) sweep_dir = 1;

            rotate(angle);

            distance1 = measure_distance1();  // update for LCD
            sprintf(buf, "Sweep %3d D:%.0f", angle, distance1);
            LCD_String_xy(1, 0, buf);
        }

        delay(30);
    }
}