/*
 This is a snake game that supports 1 or 2 players.
 Single player is the standard snake game. Each food pellet you eat you get a point, grow a segment, and speed up. Goal is to get as high a score as possible
 before you collide with a wall or yourself.
 2-Player has both player try to force the other into a collision instead of just getting points. Eating food still grows the character but the score is unused.
 A player wins when the opposing player collides in some way. If both collide at the same time then no one wins.
 */ 

#include <avr/io.h>
#include <stdlib.h>     
#include <limits.h>
#include "timer.h"
#include "lcd.h"

#define SEG_WIDTH 2 // One side of a snake segment
#define PIXEL_WIDTH 84
#define PIXEL_HEIGHT 48
#define FIELD_HEIGHT PIXEL_HEIGHT/SEG_WIDTH
#define FIELD_WIDTH PIXEL_WIDTH/SEG_WIDTH
#define START_LENGTH 4
#define MAX_LENGTH 40
#define START_SPEED 50
#define UPPER_THRESHOLD 700 //Defines the deadzone for the joystick
#define LOWER_THRESHOLD 400 

#define VERT_IN_DEADZONE(player) (vert[player] <= UPPER_THRESHOLD && vert[player] >= LOWER_THRESHOLD)
#define HORIZ_IN_DEADZONE(player) (horiz[player] <= UPPER_THRESHOLD && horiz[player] >= LOWER_THRESHOLD)

enum Direction{Up,Down,Left,Right,None};
enum States{Start, Init, Title_Screen, Move, Render, Check_Collisions, Game_Over, Eat, Player_Join} state;
enum Field_Contents{Empty, Food, Obstacle, Player1, Player2, Player};
enum bool{False, True};
struct Segment{
	//indicies of the segments position in the field
	unsigned char x;
	unsigned char y;
};

struct Snake{
	unsigned char length;
	enum Direction dir;
	enum bool collided;
	unsigned char score;
	struct Segment seg[MAX_LENGTH];
};

	
struct Cell{
	enum Field_Contents content;
	enum Direction dir;
} field[FIELD_WIDTH][FIELD_HEIGHT];


	


struct Snake players[2];
struct Segment food;
unsigned char select1 = 0;
unsigned char select2 = 0;
unsigned char reset = 0;
unsigned char speed = START_SPEED;
unsigned char num_players = 1;
unsigned char game_over_timer = 0;
unsigned short seed = 0;

struct Cell pos(unsigned char p,unsigned char s) {
	return field[players[p].seg[s].x][players[p].seg[s].y];
}

void update_seed(){
	if(seed < SHRT_MAX)
		++seed;
	else
		seed = rand() % SHRT_MAX;
}
void generate_food(){
	unsigned char food_x;
	unsigned char food_y;
	
	//keep generating coordinates until it finds an empty cell
	do{
		food_x = (rand() % (FIELD_WIDTH - 2)) + 1;
		food_y = (rand() % (FIELD_HEIGHT - 2)) + 1;
	}while(field[food_x][food_y].content != Empty);
	field[food_x][food_y].content = Food;
	food.x = food_x;
	food.y = food_y;
}
void add_segment(unsigned char player){
	unsigned char length = players[player].length;
	enum Direction last_dir = pos(player, length - 1).dir;
	
	++players[player].length;
	players[player].seg[length].x =  players[player].seg[length - 1].x;
	players[player].seg[length].y =  players[player].seg[length - 1].y;
	//places the new segment behind the previous last segment so that it follows the snake
	if(last_dir == Right)
		--players[player].seg[length].x;
	else if(last_dir == Left)
		++players[player].seg[length].x;
	else if(last_dir == Up)
		--players[player].seg[length].y;
	else if(last_dir == Down)
		++players[player].seg[length].y;
	
	field[players[player].seg[length].x][players[player].seg[length].y].dir = last_dir;
	field[players[player].seg[length].x][players[player].seg[length].y].content = Player;
	render_field();
		
}
void eat(unsigned char player){
	++players[player].score;
	if(players[player].length < MAX_LENGTH)
		add_segment(player);
	generate_food();
	if(speed > 10)
		speed -= 5;
	
	return;
}
void player_init(){
	for (unsigned char i = 0; i < num_players; ++i){
		players[i].dir =  i == 0 ? Right : Left;
		players[i].length = START_LENGTH;
		players[i].collided = False;
		players[i].score = 0;
		for (unsigned char j = 0; j < START_LENGTH && j < MAX_LENGTH; ++j){
			players[i].seg[j].x = i == 0 ? 1 + START_LENGTH - j : (FIELD_WIDTH - 1) - (START_LENGTH - j); /* first segment starts 1 + len to the right and build to the left
																											so the last segment is 1 cell to the right of the wall.
																											 Reverses that for Player2 */
			players[i].seg[j].y = i == 0 ? 2 : FIELD_HEIGHT - 3;
																
		}
	}
}
void field_init(){
	//clear the field
	for( unsigned char i = 1; i < FIELD_WIDTH; ++i){
		for( unsigned char j = 1; j < FIELD_HEIGHT; ++j){
			field[i][j].content = Empty;
			field[i][j].dir = None;
		}
	}
	
	//place the players on the field
	for (unsigned char i = 0; i < num_players; ++i){	
		for (unsigned char j = 0; players[i].length && j < MAX_LENGTH; ++j){
			field[players[i].seg[j].x][players[i].seg[j].y].dir = i == 0 ? Right : Left;
			field[players[i].seg[j].x][players[i].seg[j].y].content = Player;														
		}
	}
	generate_food();
}

void draw_snake_segment(unsigned char x, unsigned char y){
	//draws a 2x2 pixel square at (x,y)
	for( unsigned char i = 0; i < SEG_WIDTH; ++i){
		for( unsigned char j = 0; j < SEG_WIDTH; ++j){
			setPixel(SEG_WIDTH * x + i, SEG_WIDTH * y + j, 1);
		}	
	}
}
void clear_segment(unsigned char x, unsigned char y){
	for( unsigned char i = 0; i < SEG_WIDTH; ++i){
		for( unsigned char j = 0; j < SEG_WIDTH; ++j){
			setPixel(SEG_WIDTH * x + i, SEG_WIDTH * y + j, 0);
		}
	}
}
void render_field(){
	for(unsigned char i = 1 ; i < FIELD_WIDTH - 1; ++i){
		for (unsigned char j = 1; j < FIELD_HEIGHT - 1; ++j)
			if(field[i][j].content == Empty)
				clear_segment(i,j);
			else if(field[i][j].content == Food)
				draw_snake_segment(i,j);
			else
				clear_segment(i,j);
	}
	for (unsigned char i = 0; i < num_players; ++i){
		for (unsigned char j = 0; j < players[i].length; ++j){
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
				//For each quadrant the direction is determined by which value is further from the center
			else if(vert[player] > UPPER_THRESHOLD && horiz[player] > UPPER_THRESHOLD)
				dir = vert[player] >= horiz[player] ? Up : Left;
			else if(vert[player] > UPPER_THRESHOLD && horiz[player] < LOWER_THRESHOLD)
				dir = vert[player] >= (MAX_ADC_VALUE - horiz[player]) ? Up : Right;
			else if(vert[player] < LOWER_THRESHOLD && horiz[player] > UPPER_THRESHOLD)
				dir = horiz[player] >= (MAX_ADC_VALUE - vert[player]) ? Left : Down;
			else if(vert[player] < LOWER_THRESHOLD && horiz[player] < LOWER_THRESHOLD)
				dir = horiz[player] <= vert[player] ? Right : Down;
		}
		// if new direction directly opposes the current direction it is ignored.
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
		//clear the cell that the snake left
		field[players[i].seg[players[i].length - 1].x]
			 [players[i].seg[players[i].length - 1].y].content = Empty;
		for(unsigned char j = 0; j < players[i].length; ++j){
			move_segment(i, j, pos(i,j).dir);
		}
		field[players[i].seg[0].x][players[i].seg[0].x].content = Player;
	}
	
}

void render_title_screen(){
	clearDisplay();
	
	//Top-Bot Border
	unsigned char start_x = (PIXEL_WIDTH/4) - 9;
	for(unsigned char i = 0 ; i < 62 ; ++i){
		setPixel(start_x + i,1,1);
		setPixel(start_x + i,2,1);
		
		setPixel(start_x + i,19,1);
		setPixel(start_x + i,20,1);
				
	}
	//Right-Left Border
	for(unsigned char i = 3 ; i < 19 ; ++i){
		setPixel(start_x,i,1);
		setPixel(start_x + 1,i,1);
			
		setPixel(start_x + 60,i,1);
		setPixel(start_x + 61,i,1);
			
	}
	setStr("Snake",(PIXEL_WIDTH/4) + 7,7,1);
	setStr("Player1",0,PIXEL_HEIGHT - 24,1);
	setStr("Push to",0,PIXEL_HEIGHT - 16,1);
	setStr("Start",0,PIXEL_HEIGHT - 8,1);
	
	setStr("Player2",PIXEL_WIDTH - 41,PIXEL_HEIGHT - 24,1);
	setStr("Push to",PIXEL_WIDTH - 41,PIXEL_HEIGHT - 16,1);
	if (num_players == 2){
		setStr("Leave",PIXEL_WIDTH - 41,PIXEL_HEIGHT - 8,1);
	}
	else{
		setStr("Join",PIXEL_WIDTH - 41,PIXEL_HEIGHT - 8,1);
	}
	updateDisplay();
}
enum Field_Contents determine_collisions(){
	unsigned char opposing_player;
	
	//checks the static collisions for each player
	for (unsigned char i = 0; i < num_players; ++i){	
		//checks if each player has collided with a wall	
		if((players[i].seg[0].x == 0 || players[i].seg[0].x == FIELD_WIDTH - 1)
		 || (players[i].seg[0].y == 0 || players[i].seg[0].y == FIELD_HEIGHT - 1)){
			players[i].collided = True;
			}
		//checks if each player has collided with a food pellet
		if(players[i].seg[0].x == food.x && players[i].seg[0].y == food.y){
			eat(i);
		}
		//checks inter-player collisions when in 2 player
		if(num_players == 2){
			opposing_player = i == 0 ? 1 : 0;
			for(unsigned char j = 0; j < players[i].length; ++j){
				if((players[i].seg[0].x == players[opposing_player].seg[j].x) && (players[i].seg[0].y == players[opposing_player].seg[j].y)){
					players[i].collided = True;
				}
			}
		}
		//checks players for collisions with themselves
		for(unsigned char j = 1; j < players[i].length; ++j){
			if((players[i].seg[0].x == players[i].seg[j].x) && (players[i].seg[0].y == players[i].seg[j].y)){
				players[i].collided = True;
			}
		}
		
	}
	
	if(players[0].collided == True || players[1].collided == True)
		return Obstacle;
	
	return Empty;
} 

void game_over(){
	clearDisplay();
	char buf[4];

	unsigned char start_x = (PIXEL_WIDTH/4) - 9;
	setStr("Game Over", start_x, 0,1);
	if (num_players == 1){

		setStr("Score: ", start_x, 30, 1);
		sprintf(buf, "%d", players[0].score);
		setStr(buf,start_x + 40, 30, 1);
	}
	else{
		if(players[0].collided == False && players[1].collided == True)
			setStr("Player 1 Wins!", 0, 30, 1);
		else if (players[0].collided == True && players[1].collided == False)
			setStr("Player 2 Wins!", 0, 30, 1);
		else{
			setStr("No One Wins!!",0, 30, 1);
		}	
	}
	updateDisplay();
}


void Tick(){
	update_seed();
	select1 = !(PINA & (1 << PINA7));
	select2 = !(PINA & (1 << PINA6));
	reset = !(PINA & (1 << PINA5));
	enum Field_Contents collision = Empty;
	switch (state){
		case Start: state = Title_Screen;
			break;
		case Init: state = Render;
			break;
		case Title_Screen: 	if(!select2)
								state = select1 ? Init : Title_Screen;
							else{
								num_players = num_players == 1 ? 2 : 1;
								state = Player_Join;
							}
								
			break;
		case Move: state = reset ? Start : Check_Collisions;
			break;
		case Render: state = reset ? Start :Move;
			break;
		case Check_Collisions: 
								if(reset)
									state = Start;
								else{
									collision = determine_collisions();
									if(collision == Obstacle)
										state = Game_Over;
									else
										state = Render;
								}

			break;
		case Game_Over: state = select1 ? Start : Game_Over;
			break;
		case Player_Join: state = select2 ? Player_Join : Title_Screen; 
			break;
	}
	
	switch (state){
		case Start: num_players = 1;
					game_over_timer = 0;
					speed = START_SPEED;
			break;
		case Init: clearDisplay();
				   speed = START_SPEED;
				   srand(seed);
				   draw_border();
				   player_init();
				   field_init();
			break;
		case Title_Screen: render_title_screen();
							if(select2 && num_players == 1)
								num_players = 2;
							else if(select2 && num_players == 2)
								num_players = 1;
			break;
		case Move: move_players();
			break;
		case Render: render_field();
					 updateDisplay();
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
	unsigned short period = 5;
	ADC_init();
	TimerSet(period);
	TimerOn();
	srand (seed);
	lcdBegin();

    while (1) 
    {
		players[0].dir = determine_direction(0);
		players[1].dir = determine_direction(1);
		if(elapsedTime >= speed){
			Tick();
			elapsedTime = 0;
 		}
		while(!TimerFlag);
		TimerFlag = 0;
		elapsedTime += period;
    }
}

