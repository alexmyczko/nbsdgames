/*
 _
(_
 _)UDOKU 

Authored by abakh <abakh@tuta.io>
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses -lm

NOTE: This program is only made for entertainment porpuses. The puzzles are generated by randomly clearing tiles on the table and are guaranteed to have a solution , but are not guaranteed to have only one unique solution.
*/
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  //to seed random
#include <limits.h>
#include <signal.h>
#include <math.h>
#include "config.h"
typedef signed char byte;

byte _wait=0, waitcycles=0;//apparently 'wait' conflicts with a variable in a library macOS includes by default
byte py,px;
byte diff;
unsigned int filled;
chtype colors[6]={0};


#if defined Plan9 //the Plan9 compiler can not handle VLAs  
#define size 3
#define s 9
// I hope this is approximately right 
int round(float x)
{
    int y;
    if(x > 0)
        y = (int)(x + 0.5); //int will round down, so if the decimal of x is .5 or higher this will round up.
    else if(x < 0)
        y = (int)(x - 0.5); // the same but opposite
    return y;
}   
#else
byte size,s;//s=size*size
#endif
void cross(byte sy,byte sx,chtype start,chtype middle,chtype end){ //to simplify drawing tables. doesn't draw a cross (why did I choose that name?)
		mvaddch(sy,sx,start);
		byte f = 2*size;
		for(char n=1;n<size;++n){
			mvaddch(sy,sx+f,middle);
			f+=2*size;
		}
		mvaddch(sy,sx+f,end);
}

void table(byte sy,byte sx){ //empty table
		byte l;//line
		for(l=0;l<=size;++l){
			for(byte y=0;y<=s+size;++y)
				mvaddch(sy+y,sx+l*size*2,ACS_VLINE);
			for(byte x=0;x<=s*2;++x)
				mvaddch(sy+(size+1)*l,sx+x,ACS_HLINE);
		}
		cross(sy,sx,ACS_ULCORNER,ACS_TTEE,ACS_URCORNER);
		for(l=1;l<size;++l)
			cross(sy+l*size+l,sx,ACS_LTEE,ACS_PLUS,ACS_RTEE);
		cross(sy+l*size+l,sx,ACS_LLCORNER,ACS_BTEE,ACS_LRCORNER);
}

byte sgn2int(char sgn){
	if('0'<sgn && sgn <='9')
		return sgn-'0';
	if('a'<=sgn && sgn <='z')
		return sgn-'a'+10;
	if('A'<=sgn && sgn <= 'Z')
		return sgn-'A'+36;
	return 0;
}

char int2sgn(byte num){// convert integer to representing sign
	if(0< num && num <= 9)
		return num+'0';
	else if(10<=num && num <=35)
		return num-10+'a';
	else if(36<=num && num <=51)
		return num-36+'A';
	return 0;
}

bool isvalid(byte ty,byte tx,char board[s][s]){ //is it allowed to place that char there?
	char t= board[ty][tx];
	if(!t)
		return 0;
	byte y,x;
	for(y=0;y<s;++y){
		if(board[y][tx] == t && y!=ty)
			return 0;
	}
	for(x=0;x<s;++x){
		if(board[ty][x] == t && x!= tx)
			return 0;
	}
	byte sy=size*(ty/size);//square
	byte sx=size*(tx/size);
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			if(board[sy+y][sx+x]==t && sy+y != ty && sx+x != tx)
				return 0;
		}
	}				
	return 1;
}

void genocide(char board[s][s],char victim){
	for(byte y=0;y<s;++y)
		for(byte x=0;x<s;++x)
			if(board[y][x]==victim)
				board[y][x]=0;
}
bool fill_with(char board[s][s],char fillwith){//returns 1 on failure
	byte firstx,x,tries=0;
	Again:
	++tries;
	if (tries>s)
		return 1;
	for(byte y=0;y<s;++y){//there should be only one occurence of a number in a row, and this function makes use of this fact to improve generation speed
		firstx=x=rand()%s;
		while(1){
			if(!board[y][x]){
				board[y][x]=fillwith;
				if(isvalid(y,x,board)){
					break;
				}
				else{
					board[y][x]=0;
					goto Next;
				}
			}
			else{
				Next:
				++x;
				if(x==s)
					x=0;
				if(x==firstx){
					genocide(board,fillwith);
					goto Again;
				}
			}
		}
	}
	refresh();
	return 0;
}
void fill(char board[s][s]){
	for(byte num=1;num<=s;++num){//it fills random places in the board with 1s, then random places in the remaining space with 2s and so on...
		if(num==4){//something that randomly happens
			_wait=(_wait+1)%60;
			if(!_wait && waitcycles<3)
				++waitcycles;
		}
		if ( fill_with(board,int2sgn(num) ) ){
			memset(board,0,s*s);
			num=0;
			mvaddstr(0,0,"My algorithm sucks, so you need to wait a bit                    ");//with animated dots to entertain the waiter
			if(waitcycles==3)
				mvaddstr(2,0,"(You can set SUDOKU_FASTGEN if you just want to see if it works)");
			move(0,45);
			for(byte b=0;b<_wait;b+=10)
				addch('.');
		}
	}
}
void swap(char board[s][s],char A,char B){
	byte y,x;
	for(y=0;y<s;++y){
		for(x=0;x<s;++x){
			if(board[y][x]==A)
				board[y][x]=B;
			else if(board[y][x]==B)
				board[y][x]=A;
		}
	}
}
void justfill(char board[s][s]){//sometimes fill() gets too much , and you just want a 49x49 sudoku puzzle of any quality
	byte y,x,k;//k is here to minimize calls to isvalid()
	for(y=0;y<s;++y){//fill with 1,2,3,4...
		k=1;
		for(x=0;x<s;++x){
			board[y][x]=int2sgn(k);
			while(!isvalid(y,x,board)){
				board[y][x]=int2sgn(k=k+1);
				if(k>s)
					board[y][x]=int2sgn(k=1);
			}
			++k;
			if(k>s)
				k=1;
		}
	}
	for(byte n=0;n<s*2;++n)//randomize
		swap(board,int2sgn(1+(rand()%s)),int2sgn(1+(rand()%s)) );
}
void mkpuzzle(char board[s][s],char empty[s][s],char game[s][s]){//makes a puzzle to solve
	byte y,x;
	for(y=0;y<s;++y){
		for(x=0;x<s;++x){
			if( !(rand()%diff) ){
				empty[y][x]=board[y][x];
				game[y][x]=board[y][x];
			}
		}
	}
}

void header(byte sy,byte sx){
	mvaddch(sy, sx+1, '_');
	mvprintw(sy+1,sx,"(_       Solved:%d/%d",filled,s*s);
	mvprintw(sy+2,sx," _)UDOKU Left  :%d/%d",s*s-filled,s*s);
}
	
void draw(byte sy,byte sx,char empty[s][s],char game[s][s]){
	chtype attr;
	table(sy,sx);
	filled=0;
	for(byte y=0;y<s;++y){
		for(byte x=0;x<s;++x){
			attr=A_NORMAL;
			if(x==px && y==py)
				attr |= A_STANDOUT;
			if(empty[y][x])
				attr |= A_BOLD;
			if(game[y][x]){
				if(!isvalid(y,x,game))
					attr |= colors[5];
				else{
					attr |= colors[game[y][x]%5];
					++filled;
				}
				mvaddch(sy+y+y/size+1,sx+x*2+1,attr|game[y][x]);
			}
			else
				mvaddch(sy+y+y/size+1,sx+x*2+1,attr|' ');
		}
	}
}

void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(int sy, int sx){
#ifndef NO_MOUSE
	MEVENT m;
	#ifdef PDCURSES
	nc_getmouse(&m);
	#else
	getmouse(&m);
	#endif
	if( m.y < (3+1+size+s)-sy && m.x<(2*s+1)-sx ){//it's a shame to include math.h only for round() but it was the only moral way to make gcc shut up
		py= round( (float)(size*(m.y-4-sy))/(size+1) );//these are derived from the formulas in draw() by simple algebra
		px=(m.x-1-sx)/2;
	}
	else
		return;
	if(m.bstate & BUTTON1_CLICKED)
		ungetch('\n');
	if(m.bstate & (BUTTON2_CLICKED|BUTTON3_CLICKED) )
		ungetch(' ');
#endif //NO_MOUSE
}
void help(void){
	erase();
	header(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(13,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"1 - %c : Enter number/character",int2sgn(s));
	mvprintw(5,0,"SPACE : Clear tile");
	mvprintw(6,0,"ARROW KEYS : Move cursor");
	mvprintw(7,0,"q : Quit");
	mvprintw(8,0,"n : New board");
	mvprintw(9,0,"r : Restart");
	if(size>4)
		printw(" (some of these alphabet controls maybe overridden in certain sizes)");
	mvprintw(10,0,"? : Hint (not like in other games)");
	mvprintw(11,0,"F1 & F2: Help on controls & gameplay");
	mvprintw(12,0,"PgDn,PgUp,<,> : Scroll");
	mvprintw(15,0,"Press a key to continue");
	refresh();
	getch();
	erase();
}
void gameplay(void){
	erase();
	header(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("Fill the table with digits");
	if(size>3)
		printw(" (and characters)  \n");
	else
		addch('\n');

	printw("so that all the rows, columns and smaller subregions \n");
	printw("contain all of the digits from 1 to ");
	if(size<=3){
		addch(int2sgn(s));
		addch('.');
	}
	if(size>3){
		addch('9');
		printw(" and all\nthe alphabet letters from 'a' to '%c'.",int2sgn(s));
	}
	printw("\n\nPress a key to continue.");
	refresh();
	getch();
	erase();
}
int main(int argc,char** argv){
	signal(SIGINT,sigint_handler);
#ifndef Plan9
	if(argc>4 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s [size [ difficulty]] \n",argv[0]);
		return EXIT_FAILURE;
	}
	if(argc>1 ){
		if(strlen(argv[1])>1 || argv[1][0]-'0'>7 || argv[1][0]-'0'< 2){ 
			printf("2 <= size <= 7\n");
			return EXIT_FAILURE;
		}
		else
			size = *argv[1]-'0';
	}	
	else
		size=3;
	if(argc>2){ 
		if (strlen(argv[2])>1 || argv[2][0]-'0'>4 || argv[2][0]-'0'<= 0 ){
			printf("1 <= diff <=4\n");
			return EXIT_FAILURE;
		}
		else
			diff = *argv[2]-'0'+1;
	}
	else
		diff=2;
#else //Plan9 doesn't take size
	if(argc>2 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s  [difficulty]\n",argv[0]);
		return EXIT_FAILURE;
	}
	if(argc>1){ 
		if (strlen(argv[2])>1 || argv[2][0]-'0'>4 || argv[2][0]-'0'<= 0 ){
			printf("1 <= diff <=4\n");
			return EXIT_FAILURE;
		}
		else
			diff = *argv[2]-'0'+1;
	}
	else
		diff=2;
#endif
	bool fastgen= !(!getenv("SUDOKU_FASTGEN"));
	initscr();
#ifndef NO_MOUSE
	mousemask(ALL_MOUSE_EVENTS,NULL);
#endif //NO_MOUSE
	noecho();
	cbreak();
	keypad(stdscr,1);
	srand(time(NULL)%UINT_MAX);
	if( has_colors() ){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_YELLOW,-1);
		init_pair(2,COLOR_GREEN,-1);
		init_pair(3,COLOR_BLUE,-1);
		init_pair(4,COLOR_CYAN,-1);
		init_pair(5,COLOR_MAGENTA,-1);
		init_pair(6,COLOR_RED,-1);
		for(byte b=0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}
	}
#ifndef Plan9
	s= size*size;
#endif
	char board[s][s];
	char empty[s][s];
	char game[s][s];
	int input=0;
	int sy,sx;
	Start:
	sy=sx=0;
	erase();
	curs_set(0);
	filled =0;
	memset(board,0,s*s);
	memset(empty,0,s*s);
	memset(game,0,s*s);
	if(fastgen)
		justfill(board);
	else
		fill(board);
	mkpuzzle(board,empty,game);
	py=px=0;

	while(1){
		erase();
		draw(sy+3,sx+0,empty,game);
		header(sy+0,sx+0);
		refresh();
		if(filled == s*s)
			break;
		input = getch();
		if( input==KEY_PPAGE && LINES< s+size+3){//the board starts in 3
			sy+=10;
			if(sy>0)
				sy=0;
		}
		if( input==KEY_NPAGE && LINES< s+size+3){
			sy-=10;
			if(sy< -(s+size+3) )
				sy=-(s+size+3);
		}
		if( input=='<' && COLS< s*2){
			sx+=10;
			if(sx>0)
				sx=0;
		}
		if( input=='>' && COLS< s*2){
			sx-=10;
			if(sx< -(s*2))
				sx=-(s*2);
		}
		if(input == KEY_F(1))
			help();
		if(input == KEY_F(2))
			gameplay();
		if(input == KEY_MOUSE)
			mouseinput(sy,sx);
		if(input == KEY_UP && py)
			--py;
		if(input == KEY_DOWN && py<s-1)
			++py;
		if(input == KEY_LEFT && px)
			--px;
		if(input == KEY_RIGHT && px<s-1)
			++px;
		if(!empty[py][px]){
			if(input == ' ' )
				game[py][px]=0;
			else if(input<=CHAR_MAX && sgn2int(input) && sgn2int(input)<=s )
				game[py][px]=input;
		}
		if( (input=='q' && size<= 5) || input=='Q')
			sigint_handler(EXIT_SUCCESS);
		if(input=='n'&& size <= 4)
			goto Start;
		if(input=='r'&& size <= 5){
			byte y,x;
			for(y=0;y<s;++y)
				for(x=0;x<s;++x)
					game[y][x]=empty[y][x];
		}
				
		if(input=='?')
			game[py][px]=board[py][px];
		if(input == 'X' && getch()=='Y' && getch()=='Z' && getch()=='Z' && getch()=='Y'){
			byte y,x;
			for(y=0;y<s;++y)
				for(x=0;x<s;++x)
					game[y][x]=board[y][x];
		}
	}
	mvprintw(sy+s+size+4,sx+0,"Yay!! Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if(input != 'N' && input != 'n' && input != 'q')
			goto Start;
	endwin();
	return EXIT_SUCCESS;
}
