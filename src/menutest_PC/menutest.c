#include <stdlib.h>
#include <stdio.h>

#include "../menu.h"
#include "../keymapping.h"

/*
 * 6.7.2013 Andreas Weber
 * Testprogramm für die Menüsteuerung
 * 8.7. grundlegend erweitert
 *
 * ALT rastet und man kann den "viewport" mit den Cursortasten verschieben
 * CAPS rastet für Großschreibung
 * STRG + Pfeil hoch = größere Schrift
 * STRG + Pfeil runter = kleinere Schrift
 * POS1 springt an den Anfang der Zeile
 * ENDE ans Ende
 * ESC verlässt den Editor oder im Menü eine Ebene höher springen
 * ENTER im Editormodus ans Ende der nächsten Zeile
 * Mit SHIFT+ENTF kann zwischen Einfügemodus und Überschreibmodus umgeschaltet werden
 *
 * TODO bzw. zu überlegen
 * Soll "ENTER" eine neue Zeile einfügen und die alte, unterste rauswerfen?
 * äöüß ?
 */

caca_canvas_t *cv;
extern const char characters[64];

char text_buffer[BUFFER_HEIGHT][BUFFER_WIDTH+1];
uint8_t cursor_x;  //0..BUFFER_WIDTH-1
uint8_t cursor_y;  //0..BUFFER_HEIGHT-1
uint8_t viewport_x;
uint8_t viewport_y;

void draw_text_buffer()
{
  #define BUFFER_LEFT 22
  #define BUFFER_TOP 6
  caca_fill_box(cv, BUFFER_LEFT, BUFFER_TOP,  BUFFER_WIDTH+2, BUFFER_HEIGHT+2, ' ');
  caca_draw_cp437_box(cv, BUFFER_LEFT, BUFFER_TOP, BUFFER_WIDTH+2, BUFFER_HEIGHT+2);

  int i;
  for (i=0; i<BUFFER_HEIGHT; i++)
    caca_put_str(cv, BUFFER_LEFT+1, BUFFER_TOP+1+i, text_buffer[i]);
}

//Zeichen von libcaca auf 8x8 Matrix Keycode
uint8_t ascii_to_key(int c)
{
  //caca_printf(cv,0,10,"event_key_ch = %i   ",c);

  //erst ASCII Zeichen suchen
  int i;
  for (i=0;i<64; ++i)
  {
    if (c==characters[i])
    break;
  }
  if(i==64)  //nicht in characters gefunden
  {
    //Spezial keys prüfen
    switch(c)
    {
      case CACA_KEY_F4: i= _F4; break;
      case CACA_KEY_F3: i= _F3; break;
      case CACA_KEY_F2: i= _F2; break;
      case CACA_KEY_F1: i= _F1; break;
      case CACA_KEY_UP: i= _UP; break;
      case CACA_KEY_DOWN: i= _DOWN; break;
      case CACA_KEY_LEFT: i= _LEFT; break;
      case CACA_KEY_RIGHT: i= _RIGHT; break;
      case CACA_KEY_END: i= _ENDE; break;
      case CACA_KEY_DELETE: i= _ENTF; break;
      case CACA_KEY_ESCAPE : i= _ESC; break;

      //Die Mappings weil Modifier SHIFT, ALT, STRG nicht abfragbar
      case CACA_KEY_F5: i= _SHIFT; break;
      case CACA_KEY_F6: i= _ALT; break;
      case CACA_KEY_F7: i= _STRG; break;
      case CACA_KEY_F8: i= _CAPS; break;
      default:
        i=0xff;
        break;
    }
  }
  return i;
}

//LCD großer Ausschnitt von text_buffer zeigen
//Cursor cx, cy wird dargestellt
void draw_lcd(char *line0, char *line1, uint8_t x, uint8_t y, uint8_t cx, uint8_t cy)
{
  #define LCD_LEFT 1
  #define LCD_TOP 6
  caca_fill_box(cv, LCD_LEFT, LCD_TOP,  LCD_WIDTH+2, LCD_HEIGHT+2, ' ');
  caca_draw_cp437_box(cv, LCD_LEFT, LCD_TOP, LCD_WIDTH+2, LCD_HEIGHT+2);
  caca_put_str(cv, LCD_LEFT+1, LCD_TOP+1, line0);
  caca_put_str(cv, LCD_LEFT+1, LCD_TOP+2, line1);

  //aktuelle Cursorposition rot hervorheben
  int8_t tx = cx-x;
  int8_t ty = cy-y;

  if(tx>=0 && tx<LCD_WIDTH && ty>=0 && ty<LCD_HEIGHT)
  {
    caca_set_color_ansi(cv, CACA_BLACK, CACA_WHITE);
    if(ty==0)
      caca_put_char(cv, LCD_LEFT+1+tx,LCD_TOP+1+ty,line0[tx]);
    else if(ty==1)
      caca_put_char(cv, LCD_LEFT+1+tx,LCD_TOP+1+ty,line1[tx]);
    caca_set_color_ansi(cv, CACA_WHITE, CACA_BLACK);
  }
}

int main()
{
  clr_text_buffer();
  caca_display_t *dp;
  caca_event_t ev;
  dp = caca_create_display(NULL);
  if(!dp) return 1;
  cv = caca_get_canvas(dp);
  caca_set_display_title(dp, "Keyboard mapping test and menu state-machine for needler!");
  caca_set_color_ansi(cv, CACA_WHITE, CACA_BLACK);

  // SHIFT und ALT kann nicht abgefragt werden, daher mapping
  caca_put_str(cv, 1, 1, "Info: F5 = SHIFT");
  caca_put_str(cv, 1, 2, "      F6 = ALT");
  caca_put_str(cv, 20, 1, "F7 = STRG");
  caca_put_str(cv, 20, 2, "F8 = CAPS");

  int c=0;
  do
  {
    caca_refresh_display(dp);
    int ret = caca_get_event(dp, CACA_EVENT_KEY_PRESS, &ev, 100000);
    c = (ret)? caca_get_event_key_ch(&ev):0xff;

    uint8_t key = ascii_to_key(c);
    //printf("%i\n",key);

    process_menu(key);
    draw_text_buffer();

  }while(c!=CACA_KEY_CTRL_C);

  caca_free_display(dp);

  return 0;
}
