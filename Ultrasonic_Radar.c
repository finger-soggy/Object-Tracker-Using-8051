#include <reg51.h>
#include <stdio.h>
#include <math.h>
#define PWM_PERIOD 18433
#define ONE_MS 922
#define SOUND_VELOCITY 34300
#define CLOCK_PERIOD_US 1.085e-6

sfr lcd_data_port = 0xA0;  // P2 = LCD_data

sbit rs = P3^2;
sbit rw = P3^3;
sbit en = P3^4;

sbit servo = P3^6;           // Servo motor control
sbit led = P1^0;

// Ultrasonic Sensor 1 (Scanner)
sbit trig1 = P1^1;
sbit echo1 = P1^2;

// Ultrasonic Sensor 2 (Tracker)
sbit trig2 = P1^3;
sbit echo2 = P1^4;


unsigned int ON_Period, OFF_Period;
float distance1, distance2;
char buf[17];
unsigned int angle = 0;
bit angle_inc;
bit detector1;
bit detector2;

// ================== Serial ==================
void serial_init() {
    SCON = 0x50;      // Mode 1: 8-bit UART, REN enabled
    TMOD |= 0x20;     // Timer1 in mode 2 (8-bit auto-reload)
    TH1 = 0xFD;       // 9600 baud @ 11.0592 MHz
    TR1 = 1;
}

void serial_send_char(char c) {
    SBUF = c;
    while (!TI);
    TI = 0;
}

void serial_send_string(char *str) {
    while (*str) {
        serial_send_char(*str++);
    }
}


// ================== Delay Functions ==================
void delay(unsigned int count) {
    int i, j;
    for (i = 0; i < count; i++)
        for (j = 0; j < 319; j++);
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
		TH0 = 0x00;
		TL0 = 0x00;
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
		TH0 = 0x00;
		TL0 = 0x00;
    return distance;
}




// ================== Main Program ==================
void main() {
	LCD_Init();
  init_timer();
  servo = 0;
	angle = 0;
	led = 0;
	angle_inc = 1;
	
	serial_init();
	
	
	LCD_String_xy(0, 0, "Dist1: ");
	LCD_String_xy(1, 0, "Dist2: ");
	
  while (1) {
		if (angle_inc) {
			angle = angle + 5;
		}
		else {
			angle = angle - 5;
		}
		// Now check sensor 1
    distance1 = measure_distance1();
		// Now check sensor 2
		
			if(distance1>=100){
			distance1=99.99;
		} 
			
    if (distance1 > 2 && distance1 < 20) {
        // Sensor 1 detected object ? rotate to 180 degree position
			angle_inc = 0;
			detector1=1;
			
            
		}
		else{
			detector1=0;
		}
		
		distance2 = measure_distance2();
		
		if(distance2>=100){
			distance2=99.99;
		}
		
   if (distance2 > 2 && distance2 < 20) {
        // Sensor 2 detected object ? rotate to 0 degree position
      angle_inc = 1;
			detector2=1;
          
    }
		
		else{
			detector2=0;
		}
		
		if(detector1==1|| detector2==1)
		{led=1;
		}
		
		else{
			led=0;
		}
		
		delay(20);
            
    rotate(angle);	
		
		sprintf(buf, "%d,%d.", angle, (int)distance1);
    serial_send_string(buf);
	
		
		delay(25);
		sprintf(buf, "%.2f cm   ", distance1);
		LCD_String_xy(0, 6, buf);
		sprintf(buf, "%.2f cm   ", distance2);
    LCD_String_xy(1, 6, buf);
				
		if (angle == 180) {
			angle_inc = 0;
		}
		else if (angle == 0) {
			angle_inc = 1;
		}
		delay(150);
  }
}
