#include <reg51.h>
#include <stdio.h>
#include <intrins.h>
#include <math.h>
#define data_line P2
#define SpeedofSound 34300 //cm/s
#define s_to_us pow(10, -6)
#define periodcycle 1.085*(s_to_us)
#define speed_in_us SpeedofSound*s_to_us //cm per microseconds
#define PWM_Period 18433
#define onems 922

sbit trig1 = P1^1;
sbit echo1 = P1^2;
sbit trig2 = P1^3;
sbit echo2 = P1^4;
sbit servo = P3^6;
sbit RS = P3^2;
sbit RW = P3^3;
sbit EN = P3^4;

float distance1, duration1, distance2, duration2;		                        //Ultrasonic sensors parameters
unsigned int cycle1, cycle2;										    		//Ultrasonic sensors parameters
unsigned int ON_Period, OFF_Period;  							               	//Servo Motor Parameters
char angle_write, i, angle_inc;						                //Servo Motor Parameters
code unsigned char dist1[20], dist2[20];
void trigdelay();																//Ultrasonic trigger pulse delay

void sendtriggerpulse();														//Send 10us trigger pulse
void sendtriggerpulse2();
void rotate(unsigned char);
void delay_servo(unsigned int);

void lcd_delay();
void cmd(unsigned char);
void lcd_init();
void dat(unsigned char);
void LCD_String(unsigned char*);
void delay(unsigned int);


void main() {
  EA = 1;
  ET0 = 1;
  ET1 = 1;
	trig1 = trig2 = echo1 = echo2 = RS = RW = EN = servo = 0;
	angle_inc = 1;
	TMOD = 0x01;
	lcd_init();
	while (1) {

		if (angle_inc) {
			angle_write++;
		}
		else {
			angle_write--;
		}

		for (i=0; i<3; i++) {
      rotate(angle_write);
		}

		sendtriggerpulse();
		while (!echo1);
		TR0 = 1;
		while (echo1 && !TF0);
		TR0 = 0;
		TF0 = 0;
		cycle1 = TL0 | (TH0 << 8);
		sendtriggerpulse2();
		while (!echo2);
		TR1 = 1;
		while (echo2 && !TF0);
        TR1 = 0;
        TF1 = 0;


    cycle2 = TL0 | (TH0 << 8);
    duration1 = cycle1*periodcycle;
    duration2 = cycle2*periodcycle;
    distance1 = (duration1*speed_in_us)/2;
    distance2 = (duration2*speed_in_us)/2;

    if (distance1 <= 20.0) {
			angle_inc = 1;
    }
		else if (distance2 <= 20.0) {
			angle_inc = 0;
    }


		sprintf(dist1, "Dist_1: %.2f ", distance1);
    sprintf(dist2, "Dist_2: %.2f ", distance2);
		cmd(0x01);
		delay(500);
		cmd(0x80);
		LCD_String(dist1);
		cmd(0xc0);
		LCD_String(dist2);

      if (angle_write == 180)
				angle_inc = 0;
			else if (angle_write == 0)
				angle_inc = 1;

	}
}
void LCD_String(unsigned char *str) {
    int i;
    for (i = 0; str[i]; i++)
        dat(str[i]);
}


void lcd_delay() {
	TH0 = 0xdb;
	TL0 = 0xFF;
	TR0 = 1;
	while (!TF0);
	TF0 = 0;
	TR0 = 0;
}

void cmd(unsigned char x) {
	data_line = x;
	RS = 0;
	RW = 0;
	EN = 1;
	lcd_delay();
	EN = 0;
}

void lcd_init() {
	cmd(0x38);
	cmd(0x0e);
	cmd(0x01);
	cmd(0x06);
	cmd(0x0c);
	cmd(0x80);
}

void dat(unsigned char y) {
	data_line = y;
	RS = 1;
	RW = 0;
	EN = 1;
	lcd_delay();
	EN = 0;
}


void trigdelay() {

    TH0 = 0xff;
    TL0 = 0xf6;
    TR0 = 1;
    while(TF0 == 0);
    TR0 = 0;
    TF0 = 0;

}



void sendtriggerpulse() {
	trig1 = 1;
	trigdelay();
	trig1 = 0;
}

void sendtriggerpulse2() {
	trig2 = 1;
	trigdelay();
	trig2 = 0;
}
void rotate(unsigned char angle) {
  ON_Period = onems + angle*6;
	OFF_Period = PWM_Period - ON_Period;
	servo = 1;
	delay_servo(ON_Period);
	servo = 0;
	delay_servo(OFF_Period);
}

void delay_servo(unsigned int count) {
	unsigned int timer;
	timer = 65536 - count;
	TH0 = timer >> 8;
	TL0 = timer;
	TR0 = 1;
	while(!TF0);
	TR0 = 0;
	TF0 = 0;
}

void delay(unsigned int a) {
	unsigned int i;
	for (i=0; i<a; i++) {
		TH0 = onems >> 8;
		TL0 = onems;
	}
}
