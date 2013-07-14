#ifndef _MENU_H_
#define _MENU_H_

#include <string.h>
#include <ctype.h>
#ifndef AVR
  #include <caca.h>
#else
  #include "lcd.h"
#endif

/* Nicht-ASCII darstellbare Tastencodes */
#define _F3    0
#define _F2    1
#define _F1    2
#define _STRG  3
#define _F4    4
#define _DOWN  6
#define _LEFT  7
#define _UP    14
#define _RIGHT 15
#define _SHIFT 19
#define _ALT   22
#define _CAPS  35
#define _ENTF  38
#define _ENDE  46
#define _TAB   51
#define _ESC   59

union _modifier_state
{
  unsigned char complete;
  struct
  {
    unsigned char SHIFT : 1;
    unsigned char ALT   : 1;
    unsigned char STRG  : 1;
    unsigned char CAPS  : 1;
    unsigned char OVERWRITE  : 1;
    unsigned char un2  : 1;
    unsigned char un3  : 1;
    unsigned char un4  : 1;
  };
};

//Puffer ohne \0
#define BUFFER_WIDTH 25
#define BUFFER_HEIGHT 6

#define LCD_WIDTH 16
#define LCD_HEIGHT 2

void get_cursor(uint8_t *cx, uint8_t *cy);
void get_viewport(uint8_t *vx, uint8_t *vy);
const char* get_text_buffer(uint8_t line);

void clr_text_buffer(void);
void insert_ch(const uint8_t x, const uint8_t y, const char c);
void delete_ch(uint8_t x, const uint8_t y);
void cursor_viewport_calc(uint8_t scancode, uint8_t *cx, uint8_t *cy, uint8_t *vx, uint8_t *vy, char ALT);
uint8_t get_line_end(uint8_t y);
int process_menu(uint8_t scancode);

//von extern
#ifndef AVR
  void draw_lcd(char *line0, char *line1, uint8_t x, uint8_t y, uint8_t cx, uint8_t cy);
#else
  void draw_lcd(char *line, uint8_t cx, uint8_t cy);
#endif

#endif
