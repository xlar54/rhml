/* RHML Browser
	Written By Scott Hutter
*/

#include "browser.h"
#include "font.h"

// Terminal cursor x/y
uint16_t cx = PAGEX1;
uint16_t cy = PAGEY1;
bool pageClearedFlag = false;
struct mouse_info mouseInfo;
uint16_t speed = 2400;

char currPage[MAXFILENAMESZ];
char input[MAXINPUTBUFFER];			// keyboard input buffer
char inputWidth[MAXINPUTBUFFER];	// width of each char in buffer
int inputIdx = 0;					// keyboard input buffer current index

struct Coordinates {
	int x1;
	int y1;
	int x2;
	int y2;
};

struct TextBar {
	int x;
	int y;
	int position;
	void (*clear)(struct TextBar*);
	void (*write)(struct TextBar*, char *text);
};

struct MouseSprite {
	bool enabled;
	int curX;
	int curY;
	int lastX;
	int lastY;
	uint8_t mousePointerCache[11][9];
	void (*restoreCache)(struct MouseSprite*);
	void (*saveCache)(struct MouseSprite*);
	void (*draw)(struct MouseSprite*, bool force);
};

struct TextBar statusBar;
struct TextBar addressBar;
struct MouseSprite mouseSprite;

struct Icon {
	uint8_t width;
	uint8_t height;
	char **data;
};

// link buttons rendered on a page
char links[MAXLINKS][MAXFILENAMESZ];
struct Coordinates linkCoordinates[MAXLINKS];
int linkcount = 0;

// location of browser buttons (reload, back, next, close, etc)
struct Coordinates browserCoordinates[7];
int browserButtonCmdId[7];
int browserButtonCount=6;

//char history[1][80];
//int historycount = 0;

char inBuffer[MAXLINESPERPAGE][MAXCOLSPERPAGE];
int inBufferIndex = 0;

uint8_t pointer[] = {0xF0,0xE0,0xE0,0xB0,0x18,0x08,0x00,0x00};
uint8_t pointerBuf[8];
	

uint8_t serialGet()
{
	uint8_t c = 0;
	c = us_getc();
	return c;
}

void serialPut(uint8_t c)
{
	us_putc(c);
}

void clearScreen(void)
{
	tgi_setcolor(BACKCOLOR);
	tgi_bar(0,0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void clearTextBar(struct TextBar* textBar)
{
	tgi_setcolor(BACKCOLOR);
	tgi_bar(textBar->x,textBar->y, SCREEN_WIDTH, textBar->y + FONT_HEIGHT);
	textBar->position = 5;
}

void writeTextBar(struct TextBar* textBar, char *text)
{
	textBar->position = tgi_outtxt(text,strlen(text), textBar->position,textBar->y,SYS_FONT_SCALE);
}

void clearPage(void)
{
	tgi_setcolor(BACKCOLOR);
	tgi_bar(PAGEX1,PAGEY1,PAGEX2,PAGEY2);
	tgi_setcolor(FORECOLOR);
	pageClearedFlag = true;
}

void mousepointer_restore(struct MouseSprite* mousePointer)
{
	int xp=0;
	int yp=0;
	
	for(yp=0;yp<9;yp++)
		for(xp=0;xp<11;xp++)
		{
			tgi_setcolor(mousePointer->mousePointerCache[xp][yp]);
			tgi_setpixel(mousePointer->lastX+xp,mousePointer->lastY+yp);
		}
}

void mousepointer_stash(struct MouseSprite* mousePointer)
{
	int xp=0;
	int yp=0;
	
	for(yp=0;yp<9;yp++)
		for(xp=0;xp<11;xp++)
			mousePointer->mousePointerCache[xp][yp] = tgi_getpixel(mousePointer->curX+xp,mousePointer->curY+yp);
}

void mousepointer_draw(struct MouseSprite* mousePointer, bool force)
{
	static bool notfirstRun = false;
	int xp=0;
	int yp=0;
	
	if((mousePointer->lastX != mousePointer->curX || mousePointer->lastY != mousePointer->curY)
		|| force == true)
	{	
		mousepointer_restore(mousePointer);
		mousepointer_stash(mousePointer);
		
		// draw pointer
		tgi_setcolor(FORECOLOR);
		tgi_line(mousePointer->curX,mousePointer->curY,mousePointer->curX+10,mousePointer->curY);
		tgi_line(mousePointer->curX,mousePointer->curY+1,mousePointer->curX+9,mousePointer->curY+1);
		tgi_line(mousePointer->curX,mousePointer->curY+2,mousePointer->curX+8,mousePointer->curY+1);
		tgi_line(mousePointer->curX,mousePointer->curY+3,mousePointer->curX+7,mousePointer->curY+1);
		tgi_line(mousePointer->curX,mousePointer->curY,mousePointer->curX,mousePointer->curY+5);
		tgi_line(mousePointer->curX,mousePointer->curY+5,mousePointer->curX+10,mousePointer->curY);
		tgi_line(mousePointer->curX,mousePointer->curY,mousePointer->curX+10,mousePointer->curY+5);
		tgi_line(mousePointer->curX,mousePointer->curY,mousePointer->curX+10,mousePointer->curY+6);
		
		mousePointer->lastX = mousePointer->curX;
		mousePointer->lastY = mousePointer->curY;
	}

}

void init(void)
{
	int err = 0;
	uint8_t c = 0;
	cprintf ("initializing...\r\n");
	
	POKE(53280,0);
	POKE(53281,0);
		
#ifdef __C128__
	fast();
	us_init2400();
#endif

#ifdef __C64__
	us_init1200();
#endif

#ifdef __C128__    
	//tgi_load_driver("c128-vdc.tgi");
	tgi_install(tgi_static_stddrv);
#endif

#ifdef __C64__
	//tgi_load_driver("c64-hi.tgi");
	install_nmi_trampoline();
	tgi_install(tgi_static_stddrv);
#endif

	//tgi_init();
	
    err = tgi_geterror ();
    if (err  != TGI_ERR_OK) {
		cprintf ("Error #%d initializing graphics.\r\n%s\r\n",err, tgi_geterrormsg (err));
		exit (EXIT_FAILURE);
    };

	mouse_load_driver (&mouse_def_callbacks, mouse_static_stddrv);
	mouse_install (&mouse_def_callbacks, mouse_static_stddrv);
	
	mouseSprite.enabled = false;
	mouseSprite.curX = 0;
	mouseSprite.curY = 0;
	mouseSprite.lastX = 0;
	mouseSprite.lastY = 0;
	mouseSprite.draw = &mousepointer_draw;
	mouseSprite.restoreCache = &mousepointer_restore;
	mouseSprite.saveCache = &mousepointer_stash;
	(*mouseSprite.saveCache)(&mouseSprite);
	
	cprintf ("ok.\n\r");
	
	statusBar.x = STATUSX;
	statusBar.y = STATUSY;
	statusBar.position = 0;
	statusBar.clear = &clearTextBar;
	statusBar.write = &writeTextBar;
	
	addressBar.x = CMDLINEX;
	addressBar.y = CMDLINEY;
	addressBar.position = 0;
	addressBar.clear = &clearTextBar;
	addressBar.write = &writeTextBar;

	
	
}

void draw_icon(int x, int y, struct Icon* icon)
{
	int r =0;
	int c = 0;
	int z = 0;
	int tmp = 0;
	int tmpx = 0;
	
	for(r=0;r<icon->height;r++)
	{
		tmpx=x;
		for(c=0;c<icon->width;c++)
		{
			tgi_setcolor(icon->data[r][c] == ' ');
			
			for(z=0;z<SCREEN_SCALE;z++)
			{
				tgi_setpixel(tmpx+c,y+r);
				tmpx++;
			}
			tmpx--;
			
		}
	}
}

void drawButton_Reload(int x,int y, int id, int scale)
{
	tgi_box(x,y,x+(40*SCREEN_SCALE),y+11,FORECOLOR);	// reload
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+(40*SCREEN_SCALE);
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;
	
	tgi_outtxt("reload",6,13,15,scale);
}

void drawButton_Back(int x,int y, int id)
{
	struct Icon icon;
	icon.width = 8;
	icon.height = 9;
	icon.data = (char **)malloc(sizeof(char) * icon.width * icon.height);
	
	icon.data[0] = "        ";
	icon.data[1] = "   *    ";
	icon.data[2] = "  **    ";
	icon.data[3] = " *******";
	icon.data[4] = "********";
	icon.data[5] = " *******";
	icon.data[6] = "  **    ";
	icon.data[7] = "   *    ";
	icon.data[8] = "        ";
	
	tgi_box(x,y,x+(15*SCREEN_SCALE),y+11,FORECOLOR);	// back
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+(15*SCREEN_SCALE);
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;

	draw_icon(x+4,y+2,&icon);
	
	free(icon.data);
}

void drawButton_Next(int x,int y, int id)
{
	struct Icon icon;
	icon.width = 8;
	icon.height = 9;
	icon.data = (char **)malloc(sizeof(char) * icon.width * icon.height);
	
	icon.data[0] = "        ";
	icon.data[1] = "   *    ";
	icon.data[2] = "   **   ";
	icon.data[3] = "******* ";
	icon.data[4] = "********";
	icon.data[5] = "******* ";
	icon.data[6] = "   **   ";
	icon.data[7] = "   *    ";
	icon.data[8] = "        ";
	
	tgi_box(x,y,x+(15*SCREEN_SCALE),y+11,FORECOLOR);	// back
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+(15*SCREEN_SCALE);
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;

	draw_icon(x+4,y+2,&icon);
	
	free(icon.data);
}

void drawButton_Home(int x,int y, int id)
{
	struct Icon icon;
	icon.width = 10;
	icon.height = 9;
	icon.data = (char **)malloc(sizeof(char) * icon.width * icon.height);
	
	icon.data[0] = "          ";
	icon.data[1] = "    **    ";
	icon.data[2] = "   ****   ";
	icon.data[3] = " ******** ";
	icon.data[4] = "**********";
	icon.data[5] = " ******** ";
	icon.data[6] = " ***  *** ";
	icon.data[7] = " ***  *** ";
	icon.data[8] = " ***  *** ";
	
	tgi_box(x,y,x+(15*SCREEN_SCALE),y+11,FORECOLOR);
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+(15*SCREEN_SCALE);
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;

	draw_icon(x+3,y+1,&icon);
	
	free(icon.data);
}

void drawButton_Speed(int x,int y, int id)
{
	tgi_box(x,y,x+(35*SCREEN_SCALE),y+11,FORECOLOR);
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+(35*SCREEN_SCALE);
	browserCoordinates[id].y2 = y+11;
	browserButtonCmdId[id] = id;
}

void drawButton_Exit(int x,int y, int id)
{
	// exit box
	tgi_box(x,y,x+(12*SCREEN_SCALE+1),y+8,FORECOLOR);
	tgi_setcolor(BACKCOLOR);
	tgi_bar(x+1,y+1,x+(12*SCREEN_SCALE),y+7);
	tgi_setcolor(FORECOLOR);
	tgi_bar(x+2,y+2,x+(12*SCREEN_SCALE-1),y+6);
	browserCoordinates[id].x1 = x;
	browserCoordinates[id].y1 = y;
	browserCoordinates[id].x2 = x+(12*SCREEN_SCALE+1);
	browserCoordinates[id].y2 = y+8;
	browserButtonCmdId[id] = id;
}

void drawPrompt(int x, int y)
{
	tgi_line(x,y,x,y+FONT_HEIGHT-2);
}

void drawScreen(void)
{
	char *title = "rhml browser";
	
	tgi_init ();
    tgi_clear ();	
	clearScreen();
	
	tgi_setcolor(FORECOLOR);
	
	// rounded top corners (why not)
	tgi_setpixel(0,0);
	tgi_setpixel(SCREEN_WIDTH,0);
	
	tgi_line(0,2,TITLEX-5,2);tgi_line(TITLEX+(12*7)+5,2,SCREEN_WIDTH,2);
	tgi_line(0,4,TITLEX-5,4);tgi_line(TITLEX+(12*7)+5,4,SCREEN_WIDTH,4);
	tgi_line(0,6,TITLEX-5,6);tgi_line(TITLEX+(12*7)+5,6,SCREEN_WIDTH,6);
	tgi_line(0,8,TITLEX-5,8);tgi_line(TITLEX+(12*7)+5,8,SCREEN_WIDTH,8);
	
	tgi_line(0,10,SCREEN_WIDTH,10);
	tgi_line(0,25,SCREEN_WIDTH,25);
	tgi_line(0,PAGEY1-1,SCREEN_WIDTH,PAGEY1-1);
	tgi_line(0,PAGEY2+2,SCREEN_WIDTH,PAGEY2+2);
	
	// Draw buttons
	drawButton_Reload(8,12,0,SYS_FONT_SCALE);
	drawButton_Back(50*SCREEN_SCALE,12,1);
	drawButton_Next(70*SCREEN_SCALE,12,2);
	drawButton_Home(90*SCREEN_SCALE,12,6);
	drawButton_Speed(110*SCREEN_SCALE,12,3);
	drawButton_Exit(SCREEN_WIDTH - 39,2,4);
	
#ifdef __C128__
	tgi_outtxt("2400",4,(112*SCREEN_SCALE),15,SYS_FONT_SCALE);
#else
	tgi_outtxt("1200",4,(112*SCREEN_SCALE),15,SYS_FONT_SCALE);
#endif
	
	// Command bar (to clear when clicked)
	browserCoordinates[5].x1 = 0;
	browserCoordinates[5].y1 = 26;
	browserCoordinates[5].x2 = SCREEN_WIDTH;
	browserCoordinates[5].y2 = 37;
	browserButtonCmdId[5] = 5;

	tgi_outtxt(title, 12, TITLEX,TITLEY, SYS_FONT_SCALE);
	
	(*addressBar.clear)(&addressBar);
	(*addressBar.write)(&addressBar, "url>");
	
	(*statusBar.clear)(&statusBar);
	(*statusBar.write)(&statusBar, "terminal mode");
	
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
	
	tgi_setcolor(FORECOLOR);
}

void tgi_button(int x1, int y1, char *text, struct Coordinates* coords)
{
	int x2 = 0;
	int y2 = 0;
	int b = 0;
	int tmp = 0;
	int scale = 1;
	
	for(tmp=0;tmp<strlen(text);tmp++)
		b += getCharWidth(text[tmp])+1;
	
	x2 = x1 + (BTN_RL_PADDING + b + BTN_RL_PADDING) * SCREEN_SCALE;
	y2 = y1 + BTN_TB_PADDING + FONT_HEIGHT + BTN_TB_PADDING;

	tgi_box(x1,y1,x2,y2,FORECOLOR);
	tgi_line(x1+5,y2+1,x2+1,y2+1);
	tgi_line(x2+1,y1+3,x2+1,y2+1);
	
	tgi_outtxt(text, strlen(text), x1+BTN_RL_PADDING,y1+BTN_TB_PADDING, scale);
	
	coords->x1 = x1;
	coords->y1 = y1;
	coords->x2 = x2;
	coords->y2 = y2;
}

int tgi_outtxt(char *text, int idx, int x1, int y1, int scale)
{
	uint8_t c = 0;
	uint8_t data = 0;
	uint8_t curchar = 0;
	int bit = 0;
	int ctr = 0;
	int byt = 0;
	uint8_t width = 0;
	uint8_t height = 0;
	uint8_t htmp = 0;
	uint8_t base = 0;
	uint8_t z = 0;
	int origX = x1;
	
	for(curchar=0;curchar<idx;curchar++)
	{
		c = text[curchar]-32;
		
		if(c>96&&c<123) c=c-6;
	
		if(c>-1 && c<96)
		{
			width = vwfont[c][0];
			height = vwfont[c][1];
			base = vwfont[c][2];
			
			for(byt=0;byt<height;byt++)
			{
				x1 = origX;
				bit=128;
				ctr=0;
				data = vwfont[c][3+byt];
				
				while (ctr < width)
				{
					tgi_setcolor(!(data & bit));
					
					for(z=0;z<SCREEN_SCALE;z++)
					{
						tgi_setpixel(x1,y1+byt+(7-(height-base)));
						x1++;
					}

					bit = bit / 2;
					ctr++;
				}		
			}
		}
		
		origX+=width*SCREEN_SCALE+1;
		
	}
	
	return origX;
}
			
void tgi_putc(char c, int scale)
{
	char buf[1];
	
	if(c == 13)
	{
		cx = PAGEX1;
		cy += FONT_HEIGHT;
		
		if (cy >= PAGEY2)
		{
			clearPage();
			cy = PAGEY1;
		}
	}
	else
	{
		if(c != 10)
		{
			buf[0] = c;
			cx = tgi_outtxt(buf, 1,cx,cy,scale);
		}
	}
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
					
	char rleValBuf[5];
	char rleLineBuf[80];
	uint8_t rleLineIdx = 0;
	uint8_t rleValIdx = 0;
	uint8_t rleVal = 0;
	
	linkcount = 0;

	(*statusBar.clear)(&statusBar);
	(*statusBar.write)(&statusBar, "rendering...");

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
				clearPage();
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
				
				#ifdef __C64__
					if(scale > 1)
						scale--;
				#endif
				
				continue;
			}
			
			if(inBuffer[zz][1] == 'm')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param);
				margin=atoi(param) * SCREEN_SCALE;
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
				rleLineIdx = 0;
				
				// Decode the RLE
				for(z=0;z<strlen(param);z++)
				{
					if(param[z] != ' ' && param[z] != 'x')
					{
						rleValBuf[rleValIdx++] = param[z];
					}
					else
					{
						rleValBuf[rleValIdx] = 0;
						rleValIdx = 0;
						rleVal = atoi(rleValBuf);
						
						for(tmp=0;tmp<rleVal;tmp++)
							rleLineBuf[rleLineIdx++] = param[z];
					}
				}
					
				for(z=0;z<rleLineIdx;z++)
				{	
					tgi_setcolor(rleLineBuf[z] == ' ');
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
				tgi_ellipse (x1, y1, x2, tgi_imulround (x2, tgi_getaspectratio()));
				continue;
			}
			
			if(inBuffer[zz][1] == 'l')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param); x1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 2, param); y1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 3, param); x2 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 4, param); y2 = atoi(param);
				tgi_setcolor(FORECOLOR);
				tgi_line(x1,y1,x2,y2);
				continue;
			}
			
			if(inBuffer[zz][1] == 'b')
			{
				getSParam(',', inBuffer[zz], bufferLen, 1, param); x1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 2, param); y1 = atoi(param);
				getSParam(',', inBuffer[zz], bufferLen, 3, page);
				getSParam(',', inBuffer[zz], bufferLen, 4, param);

				tgi_button(x1,y1,param, &linkCoordinates[linkcount]);
				
				strcpy(links[linkcount++],page);
				continue;
			}
		}
	}
	
	(*statusBar.clear)(&statusBar);
	(*statusBar.write)(&statusBar, "page loaded");
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
	
	mouseSprite.enabled = false;
	(*mouseSprite.restoreCache)(&mouseSprite);
	linkcount=0;
	
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
	tgi_box(coords->x1, coords->y1, coords->x2, coords->y2, BACKCOLOR);
	tgi_box(coords->x1, coords->y1,	coords->x2, coords->y2, FORECOLOR);			
	
	strcpy(currPage,linkPage);
	
	sendRequest(currPage);
}

void browserbuttonClick(struct Coordinates *coords, int command)
{
	if(command != 5) 
	{
		// Flash the box that was clicked for visual feedback
		tgi_box(coords->x1, coords->y1, coords->x2, coords->y2,BACKCOLOR);
		tgi_box(coords->x1, coords->y1,	coords->x2, coords->y2,FORECOLOR);		
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
				tgi_outtxt("2400",4,(112*SCREEN_SCALE),15,SYS_FONT_SCALE);
				us_close();
				us_init2400();
			}
			else
			{
				speed = 1200;
				tgi_outtxt("1200",4,(112*SCREEN_SCALE),15,SYS_FONT_SCALE);
				us_close();
				us_init1200();
			}
#endif
			return;
		}
		case 4:  
		{
			// exit
			cprintf("bye");
			asm(MACHINE_RESET_VECTOR);
			break;
		}
		case 5:
		{
			// user clicks command bar.  clear it.
			(*addressBar.clear)(&addressBar);
			(*addressBar.write)(&addressBar, "url>");
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

void addressBarHandler(uint8_t c)
{
	char cbuf[] = { 0x00, 0x00 };
	
	if(c == 13)
	{
		input[inputIdx] = 0;
		strcpy(currPage,input);
		inputIdx = 0;

		sendRequest(currPage);
		(*addressBar.clear)(&addressBar);
		
	}
	else if (c == 20)
	{
		if (inputIdx > 0)
		{
			inputIdx--;
			input[inputIdx] = 0;
			
			(*addressBar.clear)(&addressBar);
			(*addressBar.write)(&addressBar, "url>");
			(*addressBar.write)(&addressBar, input);

			//drawPrompt(promptx,CMDLINEY);
		}
		
		c = 0;
	}
	else
	{
		if(inputIdx < MAXINPUTBUFFER)
		{
			// add input char to command line
			cbuf[0] = c;
			input[inputIdx++] = c;
			(*addressBar.write)(&addressBar, cbuf);
		}
	}
}

int main ()
{
	int idx = 0;
	int tmp = 0;
	int scale = 2;
	bool gettingPage = false;
	bool sayonce = false;
	int px = 0;
	int py = 0;
	int lastkey = -1;
	bool clicked = false;
	int periodx=0;
	uint8_t d = 0;
	uint8_t c = 0;

	init();
	drawScreen();
	
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
		
		// Handle mouse if not loading a page
		if(mouseSprite.enabled == true)
		{
			mouseSprite.curX = mouseInfo.pos.x;
			mouseSprite.curY = mouseInfo.pos.y;
			(*mouseSprite.draw)(&mouseSprite, false);

			if((mouseInfo.buttons & MOUSE_BTN_LEFT) != 0 && clicked == false)
				clicked = mouseClickHandler(mouseInfo.pos.x, mouseInfo.pos.y);
		}
		
		c = kbhit();
		c = handleMouseBug(c, lastkey);

		if(c != 0)
		{
			c = cgetc();
			lastkey=c;

			if(gettingPage == false)
				addressBarHandler(c);
		}

		d = serialGet();
		
		if (d != 0)
		{
			if (idx == 0)
				gettingPage = (d == '*' ? true : false);

			if (gettingPage == true)
			{
				if (sayonce == false)
				{
					if(currPage[0] == 0)
						strcpy(currPage, "index.rhml");
					
					(*addressBar.clear)(&addressBar);
					(*addressBar.write)(&addressBar, "url>");
					(*addressBar.write)(&addressBar, currPage);
									
					(*statusBar.clear)(&statusBar);
					(*statusBar.write)(&statusBar, "page loading");
					sayonce=true;
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
						sayonce=false;
						clicked=false;	
						mouseSprite.enabled = true;
						(*mouseSprite.saveCache)(&mouseSprite);
						(*mouseSprite.draw)(&mouseSprite,true);
					}
					else
					{			
						inBufferIndex++;
						idx=0;
						if(inBufferIndex % 3 == 0)
							(*statusBar.write)(&statusBar, ".");
					}
				}
			}
			else
				tgi_putc(d,scale);
		} 
	}

	return 0;
}