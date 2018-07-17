/* RHML Browser
	Written By Scott Hutter
*/

#include "browser.h"

// Terminal cursor x/y
uint16_t cx = PAGEX1;
uint16_t cy = PAGEY1;
unsigned char c = 0;
unsigned char d = 0;
uint16_t promptx = CMDLINEX;
uint16_t prompty = CMDLINEY;
bool pageClearedFlag = false;
struct mouse_info mouseInfo;
uint16_t speed = 2400;

char currPage[MAXFILENAMESZ];
char input[MAXINPUTBUFFER];		// keyboard input buffer
int inputIdx = 0;				// keyboard input buffer current index

struct Coordinates {
	int x1;
	int y1;
	int x2;
	int y2;
};

// link buttons rendered on a page
char links[MAXLINKS][MAXFILENAMESZ];
struct Coordinates linkCoordinates[MAXLINKS];
int linkcount = 0;

// location of browser buttons (reload, back, next, close, etc)
struct Coordinates browserCoordinates[7];
int browserButtonCmdId[7];
int browserButtonCount=6;

#ifdef __C64__

static const struct ser_params serialParams = {
    SER_BAUD_2400,      /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_NONE         /* Type of handshake to use */
};

#endif


//char history[1][80];
//int historycount = 0;

char inBuffer[MAXLINESPERPAGE][MAXCOLSPERPAGE];
int inBufferIndex = 0;


uint8_t serialGet()
{
	uint8_t c;
	
#ifdef __C128__
	c = us_getc();
#endif

#ifdef __C64__
	ser_get(&c);
#endif

	return c;
}

void serialPut(uint8_t c)
{
#ifdef __C128__
		us_putc(c);
#endif

#ifdef __C64__
		ser_put(c);
		//us_putc(request[ctr]);
#endif
}

void clearStatusBar(void)
{
	tgi_setcolor(1);
	tgi_bar(STATUSX,STATUSY, SCREEN_WIDTH, SCREEN_HEIGHT);
	tgi_setcolor(0);
}

void drawCommandBar(char* text, bool showPrompt)
{
	uint8_t len = strlen(text);
	
	tgi_setcolor(1);
	tgi_bar(CMDLINEX+(SYS_FONT_SCALE*FONT_WIDTH*4),26,SCREEN_WIDTH,37);
	
	promptx = CMDLINEX+(FONT_WIDTH*SYS_FONT_SCALE*4);
	
	if (text != 0)
		tgi_outtxt(text,len, promptx, CMDLINEY, SYS_FONT_SCALE);
	
	if (showPrompt == true)
		drawPrompt(promptx,CMDLINEY,FONT_WIDTH);
}

void clearPage(void)
{
	tgi_setcolor(1);
	tgi_bar(PAGEX1,PAGEY1,PAGEX2,PAGEY2);
	tgi_setcolor(0);
	pageClearedFlag = true;
}

void drawPointer(int x, int y, int px, int py)
{
	static int prev[11][9];
	static bool notfirstRun = false;
	int xp=0;
	int yp=0;

	if (pageClearedFlag && y >= PAGEY1)
	{		
		notfirstRun = false;
		pageClearedFlag = false;
	}
	
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
	
#ifdef __C128__
	fast();
	us_init2400();
#endif

#ifdef __C64__
	//us_init1200();
	err = ser_load_driver("c64-up2400.ser");
	if (err != 0)
	{
		ser_open(&serialParams);
		ser_ioctl(1, NULL);
	}
	else
	{
		cprintf("Error loading serial driver.\r\n");
		exit(EXIT_FAILURE);
	}
#endif

#ifdef __C128__    
	tgi_load_driver("c128-vdc.tgi");
#endif

#ifdef __C64__
	tgi_load_driver("c64-hi.tgi");
#endif

	tgi_init();
	
    err = tgi_geterror ();
    if (err  != TGI_ERR_OK) {
		cprintf ("Error #%d initializing graphics.\r\n%s\r\n",err, tgi_geterrormsg (err));
		exit (EXIT_FAILURE);
    };
	
    cprintf ("ok.\n\r");
	
	mouse_load_driver (&mouse_def_callbacks, mouse_static_stddrv);
	mouse_install (&mouse_def_callbacks, mouse_static_stddrv);
	


}

void drawButton_Reload(int x,int y, int id, int scale)
{
	tgi_box(x,y,x+80,y+11,0);	// reload
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+80;
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;
	
	tgi_outtxt("reload",6,13,15,scale);
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

void drawButton_Home(int x,int y, int id)
{
	tgi_box(x,y,x+30,y+11,0);
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+30;
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;

	tgi_setcolor(0);
	tgi_line(x+14,y+2,x+18,y+2);
	tgi_line(x+11,y+3,x+19,y+3);
	tgi_line(x+8,y+4,x+22,y+4);
	tgi_line(x+5,y+5,x+25,y+5);
	tgi_line(x+9,y+6,x+21,y+6);
	tgi_line(x+9,y+7,x+21,y+7);
	tgi_line(x+9,y+8,x+21,y+8);
	tgi_line(x+9,y+9,x+21,y+9);
	tgi_setcolor(1);
}

void drawButton_Speed(int x,int y, int id)
{
	tgi_box(x,y,x+70,y+11,0);
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+70;
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;
}

void drawButton_Exit(int x,int y, int id)
{
	// exit box
	tgi_box(x,y,x+25,y+8,0);
	tgi_setcolor(1);
	tgi_bar(x+1,y+1,x+24,y+7);
	tgi_setcolor(0);
	tgi_bar(x+2,y+2,x+23,y+6);
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+25;
	browserCoordinates[id].y2 = y+8;
	browserButtonCmdId[id] = id;
}

void drawPrompt(int x, int y, int size)
{
	tgi_setcolor(0);
	tgi_line(x,y,x+size,y);
	tgi_line(x,y+1,x+size,y+1);
	tgi_line(x,y+2,x+size,y+2);
	tgi_line(x,y+3,x+size,y+3);
	tgi_line(x,y+4,x+size,y+4);
	tgi_setcolor(1);
}

void drawScreen(void)
{
	char *title = "rhml browser";
	
	tgi_init ();
    tgi_clear ();
	
	tgi_bar(0,0, SCREEN_WIDTH, SCREEN_HEIGHT);
	tgi_setcolor(0);
	
	// rounded top corners (why not)
	tgi_setpixel(0,0);
	tgi_setpixel(SCREEN_WIDTH,0);
	
	tgi_line(0,2,TITLEX-5,2);tgi_line(TITLEX+(12*12)+5,2,SCREEN_WIDTH,2);
	tgi_line(0,4,TITLEX-5,4);tgi_line(TITLEX+(12*12)+5,4,SCREEN_WIDTH,4);
	tgi_line(0,6,TITLEX-5,6);tgi_line(TITLEX+(12*12)+5,6,SCREEN_WIDTH,6);
	tgi_line(0,8,TITLEX-5,8);tgi_line(TITLEX+(12*12)+5,8,SCREEN_WIDTH,8);
	
	tgi_line(0,10,SCREEN_WIDTH,10);
	tgi_line(0,25,SCREEN_WIDTH,25);
	tgi_line(0,PAGEY1-1,SCREEN_WIDTH,PAGEY1-1);
	tgi_line(0,PAGEY2+2,SCREEN_WIDTH,PAGEY2+2);
	
	// Draw buttons
	drawButton_Reload(8,12,0,SYS_FONT_SCALE);
	drawButton_Back(100,12,1);
	drawButton_Next(140,12,2);
	drawButton_Home(180,12,6);
	drawButton_Speed(220,12,3);
	drawButton_Exit(SCREEN_WIDTH - 39,2,4);
	
	tgi_setcolor(0);
	tgi_outtxt("2400",4,225,15,SYS_FONT_SCALE);
	tgi_setcolor(1);
	
	
	// Command bar (to clear when clicked)
	browserCoordinates[4].x1 = 0;
	browserCoordinates[4].y1 = 26;
	browserCoordinates[4].x2 = SCREEN_WIDTH;
	browserCoordinates[4].y2 = 37;
	browserButtonCmdId[4] = 5;

	tgi_outtxt(title, 12, TITLEX,TITLEY, SYS_FONT_SCALE);
	drawCommandBar(NULL, true);
	tgi_outtxt("url>",4, CMDLINEX, CMDLINEY, SYS_FONT_SCALE);
	tgi_outtxt("terminal mode",13, STATUSX,STATUSY,SYS_FONT_SCALE);

}

void getSParam(char delimiter, char* buf,  int size, int param, char *out) 
{
    
    int idx =0;
    int pcount=-1;
    int x=0;
    bool textcmd = false;
	
    out[idx] = 0;
    
	if(buf[0] == '*' && buf[1] == 't')
		textcmd = true;
	
    for(x=0 ; x<size; x++)
    {
        if (buf[x] == delimiter)
        {
			if(textcmd == true && pcount==-1)
				delimiter = 0xff;
			
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
	int ctr = 0;
	int byt = 0;
	
	tgi_setcolor(0);
	
	for(z=0;z<idx;z++)
	{
		p = text[z]-32;
		if(p>64&&p<91) p=p-32;
	
		if(p>-1 && p<91)
		{
			for(z2=0;z2<FONT_WIDTH;z2++)
			{
				yt=y1+z2;
				tmp = font[p][z2];
				b = 0;
				ctr = 1;
				
				for(byt=FONT_HIGHBIT; byt>0; byt/=2)
				{
					tgi_setcolor(!(tmp & byt));

					while(b<scale*ctr)
					{
						if(tmp & byt)
							tgi_setpixel(x1+b,yt);
						b++;
					}
					ctr++;
				}
			}
			x1+=(FONT_WIDTH)*scale;
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
			tgi_bar(PAGEX1,PAGEY1, PAGEX2, PAGEY2);
			cy = PAGEY1;
		}
	}
	else
	{
		if(c != 10)
		{
			buf[0] = c;
			tgi_setcolor(0);
			tgi_outtxt(buf, 1,cx,cy,scale);
			cx+=6*scale;
		}
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
  register int z = 0;
  int z2 = 0;
  int p = 0;
  int tmp = 0;
  int zz = 0;
  int scale = 2;
  int margin = 0;
  int tx1 = 0;
  int b=0;
  uint8_t bufferLen = 0;
  char page[80];
	
	linkcount = 0;
	clearStatusBar();
	tgi_outtxt("rendering...",12, STATUSX,STATUSY,SYS_FONT_SCALE);
	
	clearPage();
	
	for(zz=0;zz<inBufferIndex;zz++)
	{	
		bufferLen = strlen(inBuffer[zz]);
		
		if (inBuffer[zz][0] == '*')
		{
			if(inBuffer[zz][1] == 't')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param);
				tgi_outtxt(param, strlen(param), margin+x1,y1,scale);
				y1+=FONT_HEIGHT+1;
				continue;
			}
			
			if(inBuffer[zz][1] == 'c')
			{
				tgi_setcolor(1);
				tgi_bar(PAGEX1,PAGEY1, PAGEX2, PAGEY2);
				tgi_setcolor(0);
				margin=0;
				scale=2;
				x1 = PAGEX1;
				y1 = PAGEY1;
				continue;
			}
			
			if(inBuffer[zz][1] == 's')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param);
				scale=atoi(param);
				continue;
			}
			
			if(inBuffer[zz][1] == 'm')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param);
				margin=atoi(param);
				continue;
			}
			
			if(inBuffer[zz][1] == 'g')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param);
				x1=PAGEX1+atoi(param)+margin;
				
				getSParam(',', inBuffer[zz], bufferLen, 2, param);
				y1=PAGEY1+atoi(param);
				
				tgi_gotoxy(x1,y1);
				continue;
			}
			
			if(inBuffer[zz][1] == 'x')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param);

				for(z=0;z<bufferLen-2;z++)
				{
					tgi_setcolor(param[z] == ' ');
					for(b=0; b<scale; b++)
					{
						for(p=0;p<scale;p++)
							tgi_setpixel(x1+z+margin,y1+p); 
					}
				}
				y1+=scale;
				continue;
			}
			
			if(inBuffer[zz][1] == 'o')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param); x1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 2, param); y1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 3, param); x2 = atoi(param);
				tgi_setcolor(0);
				tgi_ellipse (x1, y1, x2, tgi_imulround (x2, tgi_getaspectratio()));
				continue;
			}
			
			if(inBuffer[zz][1] == 'l')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param); x1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 2, param); y1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 3, param); x2 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 4, param); y2 = atoi(param);
				tgi_setcolor(0);
				tgi_line(x1,y1,x2,y2);
				continue;
			}
			
			if(inBuffer[zz][1] == 'b')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param); x1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 2, param); y1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 3, page);
				getSParam(',', inBuffer[zz], bufferLen, 4, param);
				
				x2 = x1 + BTN_RL_PADDING + (FONT_WIDTH * strlen(param) * scale) + BTN_RL_PADDING;
				y2 = y1 + BTN_TB_PADDING + FONT_HEIGHT + BTN_TB_PADDING;
				
				tgi_setcolor(0);
				tgi_box(x1,y1,x2,y2,0);
				tgi_line(x1+5,y2+1,x2+1,y2+1);
				tgi_line(x2+1,y1+3,x2+1,y2+1);
				tgi_outtxt(param, strlen(param), x1+BTN_RL_PADDING,y1+BTN_TB_PADDING, scale);
				strcpy(links[linkcount],page);

				linkCoordinates[linkcount].x1 = x1;
				linkCoordinates[linkcount].y1 = y1;
				linkCoordinates[linkcount].x2 = x2;
				linkCoordinates[linkcount].y2 = y2;
				linkcount++;
				continue;
			}
		}
	}
	
	clearStatusBar();	
	tgi_outtxt("page loaded ",12, STATUSX,STATUSY,SYS_FONT_SCALE);
}

int handleMouseBug(int c, int lastkey)
{
	int pk = 0;
	// for some reason (a bug?) when the mouse driver is
	// loaded, the 1 and 2 keys do not work.  this fixes
	// it by checking the pressed key (212) and injecting
	// into the keyboard buffer.  But it does this every
	// cycle, so we have to compare to last key pressed.
	
#ifdef __C128__
	pk = PEEK(212);
#endif

#ifdef __C64__
	pk = PEEK(212);
#endif
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

void sendRequest(char* request)
{
	int ctr = 0;
	int t=0;
	
	for(ctr=0;ctr<strlen(request);ctr++)
	{
		for(t=0;t<1200;t++) {}; // create a brief delay for the modem to keep up
		serialPut(request[ctr]);
	}
	for(t=0;t<1200;t++){};
	serialPut(13);
}

bool inBounds(int x, int y, struct Coordinates *coords)
{
	if(x > coords->x1 && x < coords->x2 && y > coords->y1 && y < coords->y2) 
		return true;
	else
		return false;
}

void linkbuttonClick(struct Coordinates *coords, char *linkPage)
{
	tgi_box(coords->x1, coords->y1, coords->x2, coords->y2, 1);
	tgi_box(coords->x1, coords->y1,	coords->x2, coords->y2, 0);			
	
	strcpy(currPage,linkPage);
	
	sendRequest(currPage);
}

void browserbuttonClick(struct Coordinates *coords, int command)
{
	if(command != 5) 
	{
		// Flash the box that was clicked for visual feedback
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
			sendRequest(currPage);
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
#ifdef __C128__
			if (speed == 1200)
			{
				speed = 2400;
				tgi_setcolor(0);
				tgi_outtxt("2400",4,225,15,SYS_FONT_SCALE);
				tgi_setcolor(1);
				us_close();
				us_init2400();
			}
			else
			{
				speed = 1200;
				tgi_setcolor(0);
				tgi_outtxt("1200",4,225,15,SYS_FONT_SCALE);
				tgi_setcolor(1);
				us_close();
				us_init1200();
			}
#endif
			return;
		}
		case 4:  
		{
			// exit
			asm(MACHINE_RESET_VECTOR);
			break;
		}
		case 5:
		{
			// user clicks command bar.  clear it.
			drawCommandBar(NULL, true);
			return;
		}
		case 6:
		{
			strcpy(currPage, "index.rhml");
			sendRequest("index.rhml");
			break;
		}
	}
}

bool mouseClickHandler(int x, int y)
{
	int tmp=0;
	bool clicked = false;

	for (tmp=0;tmp<=browserButtonCount;tmp++)
	{
		if(inBounds(x, y, &browserCoordinates[tmp])==true)
		{
			clicked = false;
			browserbuttonClick(&browserCoordinates[tmp], browserButtonCmdId[tmp]);
			break;
		}
	}

	for(tmp=0;tmp<linkcount;tmp++)
	{
		if(inBounds(x, y, &linkCoordinates[tmp]) == true)
		{
			clicked = true;
			linkbuttonClick(&linkCoordinates[tmp], links[tmp]);
			break;
		}
	}

	return clicked;
}

int main ()
{
  
  int idx = 0;
  int tmp = 0;
  int scale = 2;
  bool gettingPage = false;
  char cbuf[1];
  int sayonce = 0;
  int px = 0;
  int py = 0;
  int lastkey = -1;
  bool clicked = false;
  int periodx=0;

  init();
  drawScreen();

  promptx = CMDLINEX+(FONT_WIDTH*SYS_FONT_SCALE*4);
  drawPrompt(promptx,CMDLINEY,FONT_WIDTH);
  
  currPage[0] = 0;
  input[0] = 0;

	while (1)
	{
		mouse_info (&mouseInfo);
		
		
#ifdef __C128__
// mouse driver is for 320 screen
// so we double the x for 640 VDC
		mouseInfo.pos.x *=2;
#endif
		
		if((mouseInfo.buttons & MOUSE_BTN_LEFT) != 0 && clicked == false)
			clicked = mouseClickHandler(mouseInfo.pos.x, mouseInfo.pos.y);
		
		if(px != mouseInfo.pos.x || py != mouseInfo.pos.y)
		{
			// software sprite 
			drawPointer(mouseInfo.pos.x, mouseInfo.pos.y, px,py);
			px=mouseInfo.pos.x;
			py=mouseInfo.pos.y;
		}

		c = kbhit();
		d = serialGet();

		c = handleMouseBug(c, lastkey);
	  
  
		if(c != 0)
		{
			c = cgetc();
			lastkey=c;

			if(gettingPage == false)
			{	
				if(c == 13)
				{
					input[inputIdx] = 0;
					strcpy(currPage,input);
					inputIdx = 0;

					sendRequest(currPage);
					drawCommandBar("", true);
				}
				else if (c == 20)
				{
					if (inputIdx > 0)
					{
						cbuf[0] = 32;
						tgi_outtxt(cbuf,1, promptx, CMDLINEY, SYS_FONT_SCALE);
						
						promptx-=FONT_WIDTH*SYS_FONT_SCALE;
						drawPrompt(promptx,CMDLINEY,10);
						inputIdx--;
						input[inputIdx] = 0;
					}
					
					c = 0;
				}
				else
				{
					if(inputIdx < MAXINPUTBUFFER)
					{
						// add input char to command line
						cbuf[0] = c;
						tgi_outtxt(cbuf,1, promptx, CMDLINEY, SYS_FONT_SCALE);
						promptx+=FONT_WIDTH*SYS_FONT_SCALE;
						drawPrompt(promptx,CMDLINEY,FONT_WIDTH);
						input[inputIdx] = c;
						inputIdx++;
					}
				}
			}
		}


		if (d !=0)
		{
			if (idx == 0)
				gettingPage = (d == '*' ? true : false);

			if (gettingPage == true)
			{
				if (sayonce == 0)
				{
					if(currPage[0] == 0)
						strcpy(currPage, "index.rhml");
					
					drawCommandBar(currPage, false);
					
					clearStatusBar();
					tgi_outtxt("page loading",12, STATUSX,STATUSY,SYS_FONT_SCALE);
					sayonce=1;
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
						gettingPage = false;
						sayonce=0;
						clicked=false;	
						periodx=0;
						drawPointer(mouseInfo.pos.x, mouseInfo.pos.y, px,py);
					}
					else
					{			
						inBufferIndex++;
						idx=0;
						if(inBufferIndex % 3 == 0)
						{
							cbuf[0] = '.';
							tgi_outtxt(cbuf,1, STATUSX+150+periodx, STATUSY, SYS_FONT_SCALE);
							periodx+=6*scale;
						}
					}
				}
			}
			else
				tgi_putc(d,scale);
		}
	}

	return 0;
}