/* RHML Browser
	Written By Scott Hutter
	7/7/2018 - Initial beta release
*/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <cc65.h>
#include <c128.h>
#include <tgi.h>
#include <stdio.h>
#include <mouse.h>
#include <peekpoke.h>
#include <unistd.h>
#include "userial.h"

#define PAGEX1	0
#define PAGEY1	40
#define PAGEX2	639
#define PAGEY2	183

#define TITLEX	245
#define TITLEY	3

#define CMDLINEX 5
#define CMDLINEY 30

#define STATUSX	5
#define STATUSY 190

// prototypes

void init(void);
void drawScreen(void);
void drawButton_Reload(int x,int y, int id);
void drawButton_Back(int x,int y, int id);
void drawButton_Next(int x,int y, int id);
void drawButton_Exit(int x,int y, int id);
void drawCommandBar(void);
void drawPointer(int x, int y, int px, int py);
void clearStatusBar(void);
void clearCommandBar(void);
void clearPage(void);
void getSParam(char delimiter, char* buf,  int size, int param, char *out);

void tgi_outtxt(char *text, int idx, int x1, int y1, int scale);
void tgi_box(int x1, int y1, int x2, int y2, int color);
void tgi_putc(char c, int scale);
void tgi_print(char* text, int len, int scale);

void loadPage(char* page);
void processPage(void);

int handleMouseBug(int c, int lastkey);
int mouseClickHandler(int x, int y, int scale);
bool inBounds(int x, int y, struct Coordinates *coords);
int main (void);

// Terminal cursor x/y
int cx = PAGEX1;
int cy = PAGEY1;
unsigned char c = 0;
unsigned char d = 0;
int promptx = CMDLINEX;
int prompty = CMDLINEY;

char currPage[80];

struct Coordinates {
	int x1;
	int y1;
	int x2;
	int y2;
};

// link buttons rendered on a page
char links[10][80];
struct Coordinates linkCoordinates[10];
int linkcount = 0;

// location of browser buttons (reload, back, next, close, etc)
struct Coordinates browserCoordinates[5];
int browserButtonCmdId[5];
int browserButtonCount=5;

char history[1][80];
int historycount = 0;

char inBuffer[200][80];
int inBufferIndex = 0;

int font[59][5] = { 
					{0,0,0,0,0},
					{4,4,4,0,4},
					{17,17,0,0,0},
					{10,31,10,31,10},
					{4,14,12,6,14},
					{25,26,4,11,19},
					{4,10,4,10,13},
					{3,6,0,0,0},
					{4,8,8,8,4},
					{4,2,2,2,4},
					{21,14,31,14,21},
					{4,4,31,4,4},
					{0,0,0,4,8},
					{0,0,31,0,0},
					{0,0,0,4,0},
					{1,2,4,8,16},
					{14,25,21,19,14},
					{4,12,4,4,14},
					{12,18,4,8,30},
					{12,2,12,2,12},
					{10,10,14,2,2},
					{31,16,30,1,30},
					{14,16,30,17,14},
					{14,2,4,4,4},
					{14,17,14,17,14},
					{12,18,14,2,2},
					{4,4,0,4,4},
					{4,4,0,4,8},
					{4,8,16,8,4},
					{0,31,0,31,0},
					{4,2,1,2,4},
					{6,1,6,0,4},
					{14,23,23,16,14},
					{14,17,31,17,17}, //A
					{30,17,30,17,30},
					{14,16,16,16,14},
					{30,17,17,17,30},
					{30,16,30,16,30},
					{30,16,30,16,16},
					{14,16,23,17,14},
					{17,17,31,17,17},
					{31, 4, 4, 4,31},
					{14, 4, 4,20, 8},
					{17,18,28,18,17},
					{16,16,16,16,31},
					{17,27,21,17,17},
					{17,25,21,19,17},
					{14,17,17,17,14},
					{30,17,30,16,16},
					{14,17,17,18,13},
					{30,17,30,17,17},
					{14,16,14, 1,14},
					{31, 4, 4, 4, 4},
					{17,17,17,17,14},
					{17,17,17,14, 4},
					{17,21,31,14,10},
					{17,10, 4,10,17},
					{17,10, 4, 4, 4},
					{31, 2, 4, 8, 31} };

void clearStatusBar(void)
{
	tgi_setcolor(1);
	tgi_bar(STATUSX,STATUSY, 639, 199);
	tgi_setcolor(0);
}

void clearCommandBar(void)
{
	int scale = 2;
	tgi_setcolor(1);
	tgi_bar(0,26,639,37);
	tgi_outtxt("cmd>",4, CMDLINEX, CMDLINEY, 2);
}

void clearPage(void)
{
	tgi_setcolor(1);
	tgi_bar(PAGEX1,PAGEY1,PAGEX2,PAGEY2);
	tgi_setcolor(0);
}

void drawPointer(int x, int y, int px, int py)
{
	static int prev[11][9];
	static bool notfirstRun = false;
	int xp=0;
	int yp=0;

	if(notfirstRun)
	{	
		for(yp=0;yp<9;yp++)
			for(xp=0;xp<11;xp++)
			{
				tgi_setcolor(prev[xp][yp]);
				tgi_setpixel(px+xp,py+yp);
			}
	}
	
	for(yp=0;yp<9;yp++)
		for(xp=0;xp<11;xp++)
			prev[xp][yp] = tgi_getpixel(x+xp,y+yp);
	
	tgi_setcolor(0);
	tgi_line(x,y,x+10,y);
	tgi_line(x,y+1,x+9,y+1);
	tgi_line(x,y+2,x+8,y+1);
	tgi_line(x,y+3,x+7,y+1);
	tgi_line(x,y,x,y+5);
	tgi_line(x,y+5,x+10,y);
	tgi_line(x,y,x+10,y+5);
	tgi_line(x,y,x+10,y+6);
	
	notfirstRun=true;

}

void init(void)
{
	int err = 0;
	
	cprintf ("initializing...\r\n");
    
	tgi_load_driver("c128-vdc.tgi");
	tgi_init();
	
    err = tgi_geterror ();
    if (err  != TGI_ERR_OK) {
		cprintf ("Error #%d initializing graphics.\r\n%s\r\n",err, tgi_geterrormsg (err));
		exit (EXIT_FAILURE);
    };
	
    cprintf ("ok.\n\r");

}

void drawButton_Reload(int x,int y, int id)
{
	tgi_box(x,y,x+80,y+11,0);	// reload
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+80;
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;
	
	tgi_outtxt("reload",6,13,15,2);
}

void drawButton_Back(int x,int y, int id)
{
	tgi_box(x,y,x+30,y+11,0);	// back
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+30;
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;

	tgi_setcolor(0);
	tgi_line(x+10,y+2,x+10,y+2);
	tgi_line(x+9,y+3,x+9,y+3);
	tgi_line(x+8,y+4,x+10,y+4);
	tgi_line(x+7,y+5,x+21,y+5);
	tgi_line(x+6,y+6,x+21,y+6);
	tgi_line(x+7,y+7,x+21,y+7);
	tgi_line(x+8,y+8,x+10,y+8);
	tgi_line(x+9,y+9,x+10,y+9);
	tgi_line(x+10,y+10,x+10,y+10);
	tgi_setcolor(1);
}

void drawButton_Next(int x,int y, int id)
{
	tgi_box(x,y,x+30,y+11,0);	// back
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+30;
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;

	tgi_setcolor(0);
	tgi_line(x+20,y+2,x+20,y+2);
	tgi_line(x+20,y+3,x+21,y+3);
	tgi_line(x+20,y+4,x+22,y+4);
	tgi_line(x+7,y+5,x+23,y+5);
	tgi_line(x+7,y+6,x+24,y+6);
	tgi_line(x+7,y+7,x+23,y+7);
	tgi_line(x+20,y+8,x+22,y+8);
	tgi_line(x+20,y+9,x+21,y+9);
	tgi_line(x+20,y+10,x+20,y+10);
	tgi_setcolor(1);
}

void drawButton_Exit(int x,int y, int id)
{
	// exit box
	tgi_box(x,y,x+25,y+8,0);
	tgi_setcolor(1);
	tgi_bar(x+1,y+1,x+24,y+7);
	tgi_setcolor(0);
	tgi_bar(x+2,y+2,x+23,y+6);
	browserCoordinates[id].x1 = 600;
	browserCoordinates[id].y1 = 2;
	browserCoordinates[id].x2 = 625;
	browserCoordinates[id].y2 = 10;
	browserButtonCmdId[id] = id;
}


void drawScreen(void)
{
	char *title = "rhml browser";
	
	tgi_init ();
    tgi_clear ();
	
	tgi_bar(0,0, 640, 200);
	tgi_setcolor(0);
	
	// rounded top corners (why not)
	tgi_setpixel(0,0);
	tgi_setpixel(639,0);
	
	tgi_line(0,2,TITLEX-5,2);tgi_line(TITLEX+(12*12)+5,2,639,2);
	tgi_line(0,4,TITLEX-5,4);tgi_line(TITLEX+(12*12)+5,4,639,4);
	tgi_line(0,6,TITLEX-5,6);tgi_line(TITLEX+(12*12)+5,6,639,6);
	tgi_line(0,8,TITLEX-5,8);tgi_line(TITLEX+(12*12)+5,8,639,8);
	tgi_line(0,10,639,10);
	tgi_line(0,25,639,25);
	tgi_line(0,PAGEY1-2,639,PAGEY1-2);
	tgi_line(0,PAGEY2+2,639,PAGEY2+2);
	
	// Draw buttons
	drawButton_Reload(8,12,0);
	drawButton_Back(100,12,1);
	drawButton_Next(140,12,2);
	drawButton_Exit(600,2,3);
	
	// Command bar (to clear when clicked)
	browserCoordinates[4].x1 = 0;
	browserCoordinates[4].y1 = 26;
	browserCoordinates[4].x2 = 639;
	browserCoordinates[4].y2 = 37;
	browserButtonCmdId[4] = 4;

	tgi_outtxt(title, 12, TITLEX,TITLEY, 2);
	tgi_outtxt("cmd>",4, CMDLINEX, CMDLINEY, 2);
	tgi_outtxt("command mode",12, STATUSX,STATUSY,2);
	
	
}

void getSParam(char delimiter, char* buf,  int size, int param, char *out) {
    
    int idx =0;
    int pcount=-1;
    int x=0;
    
    out[idx] = 0;
    
    for(x=0 ; x<size; x++)
    {
        if (buf[x] == delimiter)
        {
            pcount++;
            if(pcount == param)
            {
                out[idx] = '\0';
                return;
            }
            else
                idx = 0;
        }
        else
        {
            out[idx++] = buf[x];
        }
    }
    
    if (pcount < param-1)
    {
        out[0] = '\0'; 
        return;
    }
    else
    {
        out[idx] = '\0';
        return;
    }
}

void tgi_box(int x1, int y1, int x2, int y2, int color)
{
	tgi_setcolor(color);
	tgi_line(x1,y1,x2,y1);
	tgi_line(x2,y1,x2,y2);
	tgi_line(x2,y2,x1,y2);
	tgi_line(x1,y2,x1,y1);
}
					
void tgi_outtxt(char *text, int idx, int x1, int y1, int scale)
{
	int yl = 0;
	int z = 0;
	int tmp = 0;
	int z2 = 0;
	int p = 0;
	int yt = 0;
	int b = 0;
	
	for(z=0;z<idx;z++)
	{
		p = text[z]-32;
		if(p>64&&p<91) p=p-32;
	
		if(p>-1 && p<91)
		{
			for(z2=0;z2<5;z2++)
			{
				yt=y1+z2;
				tmp = font[p][z2];                                       
				if((tmp & 16)==16) tgi_setcolor(0); else tgi_setcolor(1);
				for(b=0; b<scale*1; b++) tgi_setpixel(x1+b,yt); 

				if((tmp & 8) == 8) tgi_setcolor(0); else tgi_setcolor(1);
				for(   ; b<scale*2; b++) tgi_setpixel(x1+b,yt);
				
				if((tmp & 4) == 4) tgi_setcolor(0); else tgi_setcolor(1);
				for(   ; b<scale*3; b++) tgi_setpixel(x1+b,yt);
				
				if((tmp & 2) == 2) tgi_setcolor(0); else tgi_setcolor(1);
				for(   ; b<scale*4; b++) tgi_setpixel(x1+b,yt);
				
				if((tmp & 1) == 1) tgi_setcolor(0); else tgi_setcolor(1);
				for(   ; b<scale*5; b++) tgi_setpixel(x1+b,yt);

				//if((tmp & 16) == 16)  tgi_setpixel(x1+0,yt); //0,1
				//if((tmp & 8) == 8)    tgi_setpixel(x1+1,yt); //2,3
				//if((tmp & 4) == 4)    tgi_setpixel(x1+2,yt); //4,5
				//if((tmp & 2) == 2)    tgi_setpixel(x1+3,yt); //6,7
				//if((tmp & 1) == 1)    tgi_setpixel(x1+4,yt); //8,9
			}
			x1+=6*scale;
		}
	}
}
		
void tgi_putc(char c, int scale)
{
	char buf[1];
	
	if(c == 13)
	{
		cx = PAGEX1;
		cy += 6*scale;
		
		if (cy >= PAGEY2)
		{
			tgi_setcolor(1);
			tgi_bar(PAGEX1+1,PAGEY1+1, PAGEX2-1, PAGEY2-1);
			cy = PAGEY1;
		}
	}
	else
	{
		buf[0] = c;
		tgi_outtxt(buf, 1,cx,cy,scale);
		cx+=6*scale;
	}
}

void tgi_print(char* text, int len, int scale)
{
	tgi_outtxt(text, len, cx, cy,scale);
	cx += 6*len*scale;
}

void processPage(void)
{
  char param[80];
  int idx = 0;
  int x1 = PAGEX1;
  int y1 = PAGEY1;
  int x2 = 0;
  int y2 = 0;
  int yt = 0;
  int z = 0;
  int z2 = 0;
  int p = 0;
  int tmp = 0;
  int zz = 0;
  int scale = 2;
  int margin = 0;
  int tx1 = 0;
  int b=0;
  char page[80];
	
	clearStatusBar();
	tgi_outtxt("rendering...",12, STATUSX,STATUSY,scale);
	
	clearPage();
	
	for(zz=0;zz<inBufferIndex;zz++)
	{	
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 't')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param);
			tgi_outtxt(param, strlen(param), margin+x1,y1,scale);
			y1+=6;
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 'c')
		{
			tgi_setcolor(1);
			tgi_bar(PAGEX1,PAGEY1, PAGEX2, PAGEY2);
			tgi_setcolor(0);
			margin=0;
			scale=2;
			x1 = PAGEX1;
			y1 = PAGEY1;
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 's')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param);
			scale=atoi(param);
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 'm')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param);
			margin=atoi(param);
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 'g')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param);
			x1=PAGEX1+atoi(param)+margin;
			
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 2, param);
			y1=PAGEY1+atoi(param);
			
			tgi_gotoxy(x1,y1);
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 'x')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param);
			tx1=x1+margin;
			for(z=0;z<strlen(inBuffer[zz])-2;z++)
			{
				if(param[z] != ' ') tgi_setcolor(0); else tgi_setcolor(1);
				for(b=1; b<scale+1; b++)
				{
					for(p=0;p<scale;p++)
						tgi_setpixel(tx1,y1+p); 
					tx1 +=b;
				}
				if(scale>1) tx1--;
			}
			y1+=scale;
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 'l')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param); x1 = atoi(param);
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 2, param); y1 = atoi(param);
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 3, param); x2 = atoi(param);
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 4, param); y2 = atoi(param);
			tgi_setcolor(0);
			tgi_line(x1,y1,x2,y2);
		}
		
		if(inBuffer[zz][0] == '*' && inBuffer[zz][1] == 'b')
		{
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 1, param); x1 = atoi(param);
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 2, param); y1 = atoi(param);
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 3, page);
			getSParam(',', inBuffer[zz], strlen(inBuffer[zz]), 4, param);
			
			p = strlen(param);
			z2 = 6*p*scale;
			
			tgi_setcolor(0);
			tgi_box(x1,y1,x1+z2+6,y1+10,0);
			tgi_outtxt(param, p, x1+5,y1+3, scale);
			strcpy(links[linkcount],page);

			linkCoordinates[linkcount].x1 = x1;
			linkCoordinates[linkcount].y1 = y1;
			linkCoordinates[linkcount].x2 = x1+z2+5;
			linkCoordinates[linkcount].y2 = y1+10;
			linkcount++;

		}
	}
	
	scale=2;
	clearStatusBar();	
	tgi_outtxt("command mode",12, STATUSX,STATUSY,scale);
}

int handleMouseBug(int c, int lastkey)
{
	int pk = 0;
	// for some reason (a bug?) when the mouse driver is
	// loaded, the 1 and 2 keys do not work.  this fixes
	// it by checking the pressed key (212) and injecting
	// into the keyboard buffer.  But it does this every
	// cycle, so we have to compare to last key pressed.
	pk = PEEK(212);
	if(pk == 56)
	{
	  c=0;
	  if(PEEK(211) == 1 && lastkey != 33)
		c = 33;
	  if(PEEK(211) == 0 && lastkey != 49)
		c = 49;

	  if(c>0)
	  {
		POKE(842,c);
		POKE(208,1);
	  }
	  return c;
	}
	
	if(pk == 59)
	{
		c=0;
	  if(PEEK(211) == 1 && lastkey != 34)
		c = 34;
	  if(PEEK(211) == 0 && lastkey != 50)
		c = 50;

	  if(c>0)
	  {
		POKE(842,c);
		POKE(208,1);
	  }
	  return c;
	}
	return c;
}

void loadPage(char* page)
{
	int ctr = 0;
	int t=0;
	
	us_putc('g');us_putc('e');us_putc('t');

	for(ctr=0;ctr<strlen(page);ctr++)
	{
		for(t=0;t<1200;t++)
		{
			// create a brief delay for the modem to keep up
		}
		us_putc(page[ctr]);
	}
	us_putc(13);
}

bool inBounds(int x, int y, struct Coordinates *coords)
{
	if(x > coords->x1 && x < coords->x2 && y > coords->y1 && y < coords->y2) 
		return true;
	else
		return false;
}

void linkbuttonClick(struct Coordinates *coords, char *linkPage, int scale)
{
	tgi_box(coords->x1, coords->y1, coords->x2, coords->y2, 1);
	tgi_box(coords->x1, coords->y1,	coords->x2, coords->y2, 0);			
	
	tgi_outtxt("click.......",12, STATUSX,STATUSY,scale);
		
	tgi_setcolor(1);
	tgi_bar(CMDLINEX,CMDLINEY, CMDLINEX+500, CMDLINEY+5);
	tgi_setcolor(0);
	
	tgi_outtxt("cmd>GET ",8, CMDLINEX, CMDLINEY, 2);
	tgi_outtxt(linkPage,strlen(linkPage), CMDLINEX+(6*8), CMDLINEY, 2);
	
	strcpy(currPage,linkPage);
	
	loadPage(currPage);
}

void browserbuttonClick(struct Coordinates *coords, int command)
{
	if(command != 4) 
	{
		tgi_setcolor(1);
		tgi_box(coords->x1, coords->y1, coords->x2, coords->y2,1);
		tgi_setcolor(0);
		tgi_box(coords->x1, coords->y1,	coords->x2, coords->y2,0);			
	}

	switch(command)
	{
		case 0:
		{
			// reload current page
			loadPage(currPage);
			break;
		}
		case 1:
		{
			// todo: prev page
			return;
		}
		case 2:
		{
			// todo: next page
			return;
		}
		case 3:
		{
			// exit
			asm("jsr $FF3D");
			break;
		}
		case 4:
		{
			clearCommandBar();
			return;
		}
	}
}

int mouseClickHandler(int x, int y, int scale)
{
	int tmp=0;
	int clicked = 0;

	for (tmp=0;tmp<=browserButtonCount;tmp++)
	{
		if(inBounds(x, y, &browserCoordinates[tmp])==true)
		{
			clicked = 0;
			browserbuttonClick(&browserCoordinates[tmp], browserButtonCmdId[tmp]);
			break;
		}
	}

	for(tmp=0;tmp<linkcount;tmp++)
	{
		if(inBounds(x, y, &linkCoordinates[tmp]) == true)
		{
			clicked = 1;
			linkbuttonClick(&linkCoordinates[tmp], links[tmp], scale);
			break;
		}
	}

	return clicked;
}

int main (void)
{
  struct mouse_info info;
  int idx = 0;
  int tmp = 0;
  int scale = 2;
  int mode=0;
  char cbuf[1];
  int sayonce = 0;
  int px = 0;
  int py = 0;
  int lastkey = -1;
  int clicked = 0;
  int periodx=0;

  init();
  us_init();
    
  fast();
  drawScreen();
  
  mouse_load_driver (&mouse_def_callbacks, mouse_static_stddrv);
  mouse_install (&mouse_def_callbacks, mouse_static_stddrv);

  promptx = CMDLINEX+(6*scale*4);
  
  strcpy(currPage,"index.rhml");

  while (1)
	{
		mouse_info (&info);
		
		// mouse driver is for 320 screen
		// so we double the x for 640 VDC
		info.pos.x *=2;
		
		if((info.buttons & MOUSE_BTN_LEFT) != 0 && clicked == 0)
			clicked = mouseClickHandler(info.pos.x, info.pos.y, scale);
		
		if(px != info.pos.x || py != info.pos.y)
		{
			// VDC ONLY
			drawPointer(info.pos.x, info.pos.y, px,py);
			px=info.pos.x;
			py=info.pos.y;
			//drawPointer(0,px,py);
			// END VDC ONLY
		}

	  c = kbhit();
	  d = us_getc();
	  c = handleMouseBug(c, lastkey);
	  
  
	  if(c != 0)
	  {
		c = cgetc();
		lastkey=c;
		
		if(mode == 0)
		{	
			if(c == 13)
			{
				// clear command line
				clearCommandBar();
				promptx = CMDLINEX+(6*scale*4);
			}
			else
			{
				// add input char to command line
				cbuf[0] = c;
				tgi_outtxt(cbuf,1, promptx, CMDLINEY, scale);
				promptx+=6*scale;
			}
			
			// send input char to modem
			us_aprintf("%c",c);
		}
	  }
	  
	  if (d !=0)
	  {
		if (idx == 0 && d != '*')
			mode = 0;
		else
			mode = 1;
		
		if (mode == 0)  
			tgi_putc(d,scale);
		else
		{
			if (sayonce == 0)
			{
				clearStatusBar();
				tgi_outtxt("page loading",12, STATUSX,STATUSY,scale);
				sayonce=1;
				linkcount=0;
				cx = STATUSX+130;
				cy = STATUSY;
			}
			
			if (d != 13)
				inBuffer[inBufferIndex][idx++] = d;
			else
			{
				inBuffer[inBufferIndex][idx] = 0;
				
				if(inBuffer[inBufferIndex][0] == '*' && inBuffer[inBufferIndex][1] == 'e')
				{
					processPage();
					inBufferIndex=0;
					idx=0;
					mode=0;
					sayonce=0;
					clicked=0;	
					periodx=0;
					drawPointer(info.pos.x, info.pos.y, px,py);
				}
				else
				{			
					inBufferIndex++;
					idx=0;
					if(inBufferIndex % 3 == 0)
					{
						cbuf[0] = '.';
						tgi_outtxt(cbuf,1, STATUSX+150+periodx, STATUSY, scale);
						periodx+=6*scale;
					}
				}
			}
		}
	  }

  
  }

  return 0;
}