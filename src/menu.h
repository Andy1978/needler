#ifndef _MENU_H_
#define _MENU_H_

#include <string.h>
#include <ctype.h>

#ifndef AVR
  #include <caca.h>
#else
  #include "lcd.h"
#endif

//Puffer ohne \0
#define BUFFER_WIDTH 25
#define BUFFER_HEIGHT 8

#define LCD_WIDTH 16
#define LCD_HEIGHT 2

// Der eigentliche Editorbuffer
extern char text_buffer[BUFFER_HEIGHT][BUFFER_WIDTH+1];
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
