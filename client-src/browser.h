#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <cc65.h>
#include <serial.h>
#include <tgi.h>
#include <stdio.h>
#include <mouse.h>
#include <peekpoke.h>
#include <unistd.h>
#include "userial.h"
#include "font.h"

#ifdef __C128__
#include <c128.h>
#endif

#ifdef __C64__
#include <c64.h>
#endif

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;

// vdc is scale 2, vic scale 1
#ifdef __C128__
#define SCREEN_SCALE	2
#endif

#ifdef __C64__
#define SCREEN_SCALE	1
extern install_nmi_trampoline();
#endif


#define SCREEN_WIDTH	320 * SCREEN_SCALE
#define SCREEN_HEIGHT	199

#define BTN_TB_PADDING	3
#define BTN_RL_PADDING	5

#define PAGEX1	0
#define PAGEY1	40
#define PAGEX2	SCREEN_WIDTH
#define PAGEY2	182

#ifdef __C128__
#define TITLEX	275
#endif

#ifdef __C64__
#define TITLEX	120
#endif

#define TITLEY	2

#define CMDLINEX 5
#define CMDLINEY 30

#define STATUSX	5
#define STATUSY 190

#define MAXRCVBUFFERSZ	1000
#define MAXINPUTBUFFER	40
#define MAXLINKS		10
#define MAXCOLSPERPAGE	80
#define MAXLINESPERPAGE	100
#define MAXFILENAMESZ	40

#ifdef __C128__
#define MACHINE_RESET_VECTOR	"jsr $FF3D"
#endif

#ifdef __C64__
#define MACHINE_RESET_VECTOR	"jsr $FCE2"
#endif

// prototypes

void init(void);
void drawScreen(void);
void drawButton_Reload(int x,int y, int id, int scale);
void drawButton_Back(int x,int y, int id);
void drawButton_Next(int x,int y, int id);
void drawButton_Home(int x,int y, int id);
void drawButton_Speed(int x,int y, int id);
void drawButton_Exit(int x,int y, int id);
void drawCommandBar(char* text, bool showPrompt);
void mousepointer_restore(struct MouseSprite* mousePointer);
void mousepointer_stash(struct MouseSprite* mousePointer);
void mousepointer_draw(struct MouseSprite* mousePointer, bool force);
void draw_icon(int x, int y, struct Icon* icon);
void clearStatusBar(void);
void clearPage(void);
void getSParam(char delimiter, char* buf,  int size, int param, char *out);

int tgi_outtxt(char *text, int idx, int x1, int y1, int scale);
void tgi_box(int x1, int y1, int x2, int y2, int color);
void tgi_button(int x1, int y1, char *text, struct Coordinates*);
void tgi_putc(char c, int scale);

void sendRequest(char* request);
void processPage(void);

int handleMouseBug(int c, int lastkey);
bool mouseClickHandler(int x, int y);
bool inBounds(int x, int y, struct Coordinates *coords);