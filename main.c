/*
 * Snake.cpp
 *
 * Created: 5/16/2018 10:43:43 PM
 * Author : Connor
 */ 

#include <avr/io.h>
#include <string.h>

#include "timer.h"
#include "nokia5110.h"

#define SEG_WIDTH 2 // One side of a snake segment
#define FIELD_HEIGHT 48/SEG_WIDTH
#define FIELD_WIDTH 84/SEG_WIDTH
#define START_LENGTH 4
#define MAX_LENGTH 10
// #define Player1 0
// #define PLAYER2 1
#define UPPER_THRESHOLD 600 //Defines the deadzone for the joystick
#define LOWER_THRESHOLD 400 

#define VERT_IN_DEADZONE(player) (vert[player] <= UPPER_THRESHOLD && vert[player] >= LOWER_THRESHOLD)
#define HORIZ_IN_DEADZONE(player) (horiz[player] <= UPPER_THRESHOLD && horiz[player] >= LOWER_THRESHOLD)

enum Direction{Up,Down,Left,Right,None};
enum States{Start, Init, Title_Screen, Move, Render, Check_Collisions, Game_Over} state;
enum Field_Contents{Empty, Food, Obstacle, Player1, Player2};
enum bool{False, True};
struct Segment{
	//indicies of the segments position in the field
	int x;
	int y;
};

struct Snake{
	unsigned char length;
	enum Direction dir;
	enum bool collided;
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
unsigned char game_over_timer = 0;
struct Cell pos(unsigned char p,unsigned char s) {
	return field[players[p].seg[s].x][players[p].seg[s].y];
}

void player_init(){
	for (unsigned char i = 0; i < num_players; ++i){
		players[i].dir = Right;
		players[i].length = START_LENGTH;
		players[i].collided = False;
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
	for(unsigned char i = 1 ; i < FIELD_WIDTH - 1; ++i){
		for (unsigned char j = 1; j < FIELD_HEIGHT - 1; ++j)
			if(field[i][j].content == Player1 || field[i][j].content == Player2)
				clear_segment(i,j);
			else
				clear_segment(i,j);
	}
	for (unsigned char i = 0; i < num_players; ++i){
		for (unsigned char j = 0; players[i].length && j < MAX_LENGTH; ++j){
			draw_snake_segment(players[i].seg[j].x,players[i].seg[j].y);	
		}
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
	enum Direction oldDir = dir;
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
		if((dir == Up && oldDir == Down) || (oldDir == Up && dir == Down))
			dir = oldDir;
		else if ((dir == Left && oldDir == Right) || (oldDir == Left && dir == Right))
			dir = oldDir;
	return dir;	
}
void move_segment(unsigned char player,unsigned char seg, enum Direction dir){
	switch(dir){
	case Up: if(players[player].seg[seg].y > 0)--players[player].seg[seg].y;
		break;
	case Down:if(players[player].seg[seg].y < FIELD_HEIGHT) ++players[player].seg[seg].y;
		break;
	case Right:if(players[player].seg[seg].x < FIELD_WIDTH) ++players[player].seg[seg].x;
		break;
	case Left:if (players[player].seg[seg].x > 0)--players[player].seg[seg].x;
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
		field[players[i].seg[players[i].length - 1].x]
			 [players[i].seg[players[i].length - 1].y].content = Empty;
		for(unsigned char j = 0; j < players[i].length; ++j){
			move_segment(i, j, pos(i,j).dir);
		}
		field[players[i].seg[0].x][players[i].seg[0].x].content = i == 0 ? Player1 : Player2;
	}
	
}

void render_title_screen(){
	nokia_lcd_clear();
	nokia_lcd_write_string("Press sel to begin", 1);
	nokia_lcd_render();
}
enum Field_Contents determine_collisions(){
	for (unsigned char i = 0; i < num_players; ++i){		
		//if(pos(i,0).content == Obstacle)
		if((players[i].seg[0].x == 0 || players[i].seg[0].x == FIELD_WIDTH - 1)
		 || (players[i].seg[0].y == 0 || players[i].seg[0].y == FIELD_HEIGHT - 1)){
			players[i].collided = True;
				return Obstacle;
			}
			
			
	}	
	if(players[0].collided == True || players[1].collided == True)
		return Obstacle;
	
	return Empty;
} 

void game_over(){
	nokia_lcd_clear();
	nokia_lcd_write_string("Game Over Man", 1);
	nokia_lcd_render();
}
void Tick(){
	select1 = !(PINA & (1 << PINA7));
	select2 = !(PINA & (1 << PINA6));
	enum Field_Contents collision = Empty;
	switch (state){
		case Start: state = Title_Screen;
			break;
		case Init: state = Render;
			break;
		case Title_Screen: state = select1 ? Init : Title_Screen;
			break;
		case Move: state = Check_Collisions;
			break;
		case Render: state = Move;
			break;
		case Check_Collisions: collision = determine_collisions();
								if(collision == Obstacle)
									state = Game_Over;
								else
									state = Render;
			break;
		case Game_Over: state = select1 ? Start : Game_Over;
			break;
	}
	
	switch (state){
		case Start:
			break;
		case Init: nokia_lcd_clear();
				   draw_border();
				   player_init();
				   field_init();
			break;
		case Title_Screen: render_title_screen();
			break;
		case Move: move_players();
			break;
		case Render: render_field();
					 nokia_lcd_render();
			break;
		case Check_Collisions:
			break;
		case Game_Over: if (game_over_timer >= 2)
							game_over();
						++game_over_timer;
			break;
	}
}
int main(void)
{
    /* Replace with your application code */
	DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	unsigned short elapsedTime = 0;
	unsigned short period = 10;
	ADC_init();
	TimerSet(period);
	TimerOn();
	
	nokia_lcd_init();
	
 char str1[10];
 char ch;
    while (1) 
    {
//nokia_lcd_clear();
// 	enum Direction dir = determine_direction(0);
// 	switch(dir){
// 		case Up: ch = '1';
// 			break;
// 		case Down: ch = '2';
// 			break;
// 		case Left: ch = '3';
// 			break;
// 		case Right: ch = '4';
// 			break;	
// 	}
// 	nokia_lcd_set_cursor(0,0);
// 	nokia_lcd_write_char(ch,1);
// 	nokia_lcd_render();
		if(elapsedTime == 100){
			Tick();
			elapsedTime = 0;
 		}
		while(!TimerFlag);
		TimerFlag = 0;
		elapsedTime += period;


		
    }
}

