/*
 * Snake.cpp
 *
 * Created: 5/16/2018 10:43:43 PM
 * Author : Connor
 */ 

#include <avr/io.h>
#include <string.h>

#include "timer.h"
#include "lcd.h"

#define SEG_WIDTH 2 // One side of a snake segment
#define FIELD_HEIGHT 48/SEG_WIDTH
#define FIELD_WIDTH 84/SEG_WIDTH

unsigned char select1 = 0;
unsigned char select2 = 0;


void draw_snake_segment(unsigned char x, unsigned char y){
	for( unsigned char i = 0; i < SEG_WIDTH; ++i){
		for( unsigned char j = 0; j < SEG_WIDTH; ++j){
			nokia_lcd_set_pixel(SEG_WIDTH * x + i, SEG_WIDTH * y + j, 1);
		}	
	}
}

void draw_border(){
	for( unsigned char i = 0; i < FIELD_WIDTH; ++i){
		draw_snake_segment(i,0);
		draw_snake_segment(i, FIELD_HEIGHT-1);
	}
	for( unsigned char i = 0; i < FIELD_HEIGHT; ++i){
		draw_snake_segment(0,i);
		draw_snake_segment(FIELD_WIDTH-1, i);
	}
	
}

int main(void)
{
    /* Replace with your application code */
	DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	unsigned short elapsedTime = 0;
	unsigned char period = 10;
	ADC_init();
	TimerOn();
	TimerSet(period);
	
	nokia_lcd_init();
	 //nokia_lcd_write_string("Hello", 1);
	 ////draw_border();
	 //
	 //nokia_lcd_set_cursor(0,10);
	 //nokia_lcd_write_string("Vert: ",1);
	////// nokia_lcd_write_string(vert1,1);
//
	 ////
	 //nokia_lcd_set_cursor(0,20);
	 //nokia_lcd_write_string("Horiz: ",1);
	////// nokia_lcd_write_string(horiz1,1);

		  
	 //nokia_lcd_render();
	 nokia_lcd_clear();
	 char str1[6];
	 char str2[6];
	 char str3[6];
	 char str4[6];
    while (1) 
    {

		if(elapsedTime = 100){
			nokia_lcd_clear();
			nokia_lcd_set_cursor(0,0);
			nokia_lcd_write_string("Vert1: ",1);
			
			sprintf(str1, "%d", vert1);
			nokia_lcd_write_string(str1,1);

			sprintf(str2, "%d", horiz1);
			nokia_lcd_set_cursor(0,10);
			nokia_lcd_write_string("Horiz1: ",1);
			nokia_lcd_write_string(str2,1);
			
						sprintf(str3, "%d", vert2);
						nokia_lcd_set_cursor(0,20);
						nokia_lcd_write_string("Vert2: ",1);
						nokia_lcd_write_string(str3,1);
						
									sprintf(str4, "%d", horiz2);
									nokia_lcd_set_cursor(0,30);
									nokia_lcd_write_string("Horiz2: ",1);
									nokia_lcd_write_string(str4,1);

			
			nokia_lcd_render();
			elapsedTime = 0;
		}
		while(!TimerFlag)
		
		TimerFlag = 0;
		elapsedTime += period;
		
    }
}

