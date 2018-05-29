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
#define START_LENGTH 1
#define MAX_LENGTH 10

#define UPPER_THRESHOLD 600 //Defines the deadzone for the joystick
#define LOWER_THRESHOLD 400 

#define VERT_IN_DEADZONE(player) (vert[player] <= UPPER_THRESHOLD && vert[player] >= LOWER_THRESHOLD)
#define HORIZ_IN_DEADZONE(player) (horiz[player] <= UPPER_THRESHOLD && horiz[player] >= LOWER_THRESHOLD)

enum Direction{Up,Down,Left,Right,None};
enum States{Start, Init, };
enum Field_Contents{Empty, Food, Obstacle, Player1, Player2};
	
struct Segment{
	//indicies of the segments position in the field
	int x;
	int y;
};

struct Snake{
	unsigned char length;
	enum Direction dir;
	struct Segment seg[MAX_LENGTH];
};

	
struct Cell{
	enum Field_Contents content;
	enum Direction dir;
} field[FIELD_WIDTH][FIELD_HEIGHT];


	


struct Snake players[2];

unsigned char select1 = 0;
unsigned char select2 = 0;

unsigned char num_players = 1;

inline struct Cell pos(unsigned char p,unsigned char s) {
	return field[players[p].seg[s].x][players[p].seg[s].y];
}

void player_init(){
	for (unsigned char i = 0; i < num_players; ++i){
		players[i].dir = Right;
		players[i].length = START_LENGTH;
		for (unsigned char j = 0; j < START_LENGTH && j < MAX_LENGTH; ++j){
			players[i].seg[j].x = 1 + START_LENGTH - j; /* first segment starts 1 + len to the right and build to the left
												  so the last segment is 1 cell to the right of the wall*/
			players[i].seg[j].y = i == 0 ? 2 : FIELD_HEIGHT - 2;
																
		}
	}
}
void field_init(){
	for( unsigned char i = 0; i < FIELD_WIDTH; ++i){
		for( unsigned char j = 0; j < FIELD_HEIGHT; ++j){
			field[i][j].content = Empty;
			field[i][j].dir = None;
		}
	}
	
	for (unsigned char i = 0; i < num_players; ++i){	
		for (unsigned char j = 0; players[i].length && j < MAX_LENGTH; ++j){
			field[players[i].seg[j].x][players[i].seg[j].y].dir = Right;
			field[players[i].seg[j].x][players[i].seg[j].y].content = i == 0 ? Player1 : Player2;														
		}
	}
}

void draw_snake_segment(unsigned char x, unsigned char y){
	for( unsigned char i = 0; i < SEG_WIDTH; ++i){
		for( unsigned char j = 0; j < SEG_WIDTH; ++j){
			nokia_lcd_set_pixel(SEG_WIDTH * x + i, SEG_WIDTH * y + j, 1);
		}	
	}
}
void clear_segment(unsigned char x, unsigned char y){
	for( unsigned char i = 0; i < SEG_WIDTH; ++i){
		for( unsigned char j = 0; j < SEG_WIDTH; ++j){
			nokia_lcd_set_pixel(SEG_WIDTH * x + i, SEG_WIDTH * y + j, 0);
		}
	}
}
void render_field(){
	for(unsigned char i = 1 ; i < FIELD_WIDTH - 2; ++i){
		for (unsigned char j = 1; j < FIELD_HEIGHT - 2; ++j)
			if(field[i][j].content == Player1 || field[i][j].content == Player2)
				draw_snake_segment(i,j);
			else
				clear_segment(i,j);
	}
}
void draw_border(){
	for( unsigned char i = 0; i < FIELD_WIDTH; ++i){
		draw_snake_segment(i,0);
		field[i][0].content = Obstacle;
		draw_snake_segment(i, FIELD_HEIGHT-1);
		field[i][FIELD_HEIGHT - 1].content = Obstacle;
	}
	for( unsigned char i = 0; i < FIELD_HEIGHT; ++i){
		draw_snake_segment(0,i);
		field[0][i].content = Obstacle;
		draw_snake_segment(FIELD_WIDTH-1, i);
		field[FIELD_WIDTH-1, i][i].content = Obstacle;
	}
	
}

enum Direction determine_direction(unsigned char player){
	enum Direction dir = players[player].dir; // directiopn in fiels defaults to the current direction of the player
	
	//If the joystick is out of the deadzone then a direction is read otherwise the player continues ion the same direction
	if (!VERT_IN_DEADZONE(player) || !HORIZ_IN_DEADZONE(player)){
			if(!VERT_IN_DEADZONE(player) && HORIZ_IN_DEADZONE(player))
				dir = vert[player] > UPPER_THRESHOLD ? Up : Down;
			else if(VERT_IN_DEADZONE(player) && !HORIZ_IN_DEADZONE(player))
				dir = horiz[player] > UPPER_THRESHOLD ? Left : Right;
			else if(vert[player] > UPPER_THRESHOLD && horiz[player] > UPPER_THRESHOLD)
				dir = vert[player] >= horiz[player] ? Up : Left;
			else if(vert[player] > UPPER_THRESHOLD && horiz[player] < LOWER_THRESHOLD)
				dir = vert[player] >= (MAX_ADC_VALUE - horiz[player]) ? Up : Right;
			else if(vert[player] < LOWER_THRESHOLD && horiz[player] > UPPER_THRESHOLD)
				dir = horiz[player] >= (MAX_ADC_VALUE - vert[player]) ? Left : Down;
			else if(vert[player] < LOWER_THRESHOLD && horiz[player] < LOWER_THRESHOLD)
				dir = horiz[player] <= vert[player] ? Right : Down;
		}
	return dir;	
}
void move_segment(unsigned char player,unsigned char seg, enum Direction dir){
	switch(dir){
	case Up: players[player].seg[seg].y = players[player].seg[seg].y - 1;
		break;
	case Down: players[player].seg[seg].y = players[player].seg[seg].y + 1;
		break;
	case Right: players[player].seg[seg].x = players[player].seg[seg].x + 1;
		break;
	case Left: players[player].seg[seg].x = players[player].seg[seg].x - 1;
		break;
	case None:
		break;
	}
}
void move_players(){
	enum Direction dir;
	for (unsigned char i = 0; i < num_players; ++i){
		dir = determine_direction(i);
		field[players[i].seg[0].x][players[i].seg[0].y].dir = dir;
		players[i].dir = dir;
		for(unsigned char j = 0; j < players[i].length; ++j){
			move_segment(i, j, pos(i,j).dir);
		}
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
	
	 //char str1[6];
	 //char str2[6];
	 //char str3[6];
	 //char str4[6];
	 //nokia_lcd_clear();
	 //nokia_lcd_set_cursor(0,0);
	 //nokia_lcd_write_string("Vert1: ",1);
	 //
	 //sprintf(str1, "%d", vert1);
	 //nokia_lcd_write_string(str1,1);
//
	 //sprintf(str2, "%d", horiz1);
	 //nokia_lcd_set_cursor(0,10);
	 //nokia_lcd_write_string("Horiz1: ",1);
	 //nokia_lcd_write_string(str2,1);
	 //
	 //sprintf(str3, "%d", vert2);
	 //nokia_lcd_set_cursor(0,20);
	 //nokia_lcd_write_string("Vert2: ",1);
	 //nokia_lcd_write_string(str3,1);
	 //
	 //sprintf(str4, "%d", horiz2);
	 //nokia_lcd_set_cursor(0,30);
	 //nokia_lcd_write_string("Horiz2: ",1);
	 //nokia_lcd_write_string(str4,1);
//
	 //
	 //nokia_lcd_render();
    while (1) 
    {

		if(elapsedTime = 100){
			
			elapsedTime = 0;
		}
		while(!TimerFlag)
		
		TimerFlag = 0;
		elapsedTime += period;
		
    }
}

