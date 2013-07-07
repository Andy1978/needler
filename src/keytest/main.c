#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <caca.h>

#include "../keymapping.h"

/*
 * 6.7.2013 Andreas Weber
 * Testprogramm für die Menüsteuerung
 *
 * ALT rastet und man kann den "viewport" mit den Cursortasten verschieben
 * CAPS rastet für Großschreibung
 * STRG + Pfeil hoch = größere Schrift
 * STRG + Pfeil runter = kleinere Schrift
 * POS1 springt an den Anfang der Zeile
 * ENDE ans Ende
 * ESC verlässt den Editor oder im Menü eine Ebene höher springen
 * ENTER im Editormodus ans Ende der nächsten Zeile
 *
 * Mit SHIFT+ENTF kann zwischen Einfügemodus und Überschreibmodus umgeschaltet werden
 */

//Puffer ohne \0
#define BUFFER_WIDTH 25
#define BUFFER_HEIGHT 8

// Der eigentliche Editorbuffer
char text_buffer[BUFFER_HEIGHT][BUFFER_WIDTH+1];

void clr_text_buffer();
void insert_ch(const uint8_t x, const uint8_t y, const char c);
void delete_ch(uint8_t x, const uint8_t y);

#define LCD_WIDTH 16
#define LCD_HEIGHT 2


char lcd_lines[LCD_HEIGHT][LCD_WIDTH+1];
caca_canvas_t *cv;


//LCD großer Ausschnitt von text_buffer zeigen
//Cursor cx, cy wird rot dargestellt

void draw_lcd(uint8_t x, uint8_t y, uint8_t cx, uint8_t cy)
{
  #define LCD_LEFT 1
  #define LCD_TOP 6
  caca_fill_box(cv, LCD_LEFT, LCD_TOP,  LCD_WIDTH+2, LCD_HEIGHT+2, ' ');
  caca_draw_cp437_box(cv, LCD_LEFT, LCD_TOP, LCD_WIDTH+2, LCD_HEIGHT+2);

  //Hier wäre es schön, wenn es einen printf Befehl gäbe
  //der nur n Zeichen ausgibt.
  //Hier kopiere ich einfach nur das Stückchen
  //aber auf dem AVR könnte man das programmieren

  strncpy(lcd_lines[0],text_buffer[y]+x,LCD_WIDTH);
  strncpy(lcd_lines[1],text_buffer[y+1]+x,LCD_WIDTH);

  caca_put_str(cv, LCD_LEFT+1, LCD_TOP+1, lcd_lines[0]);
  caca_put_str(cv, LCD_LEFT+1, LCD_TOP+2, lcd_lines[1]);

  //aktuelle Cursorposition rot hervorheben
  //später auf dem LCD mit normalem Cursor
  //lcd_gotoxy...
  int8_t tx = cx-x;
  int8_t ty = cy-y;
  if(tx>=0 && tx<LCD_WIDTH && ty>=0 && ty<LCD_HEIGHT)
  {
    //caca_set_color_ansi(cv, CACA_RED, CACA_BLACK);
    caca_set_color_ansi(cv, CACA_BLACK, CACA_WHITE);
    caca_put_char(cv, LCD_LEFT+1+tx,LCD_TOP+1+ty,lcd_lines[ty][tx]);
    caca_set_color_ansi(cv, CACA_WHITE, CACA_BLACK);
  }
}

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

void cursor_viewport_calc(uint8_t scancode, uint8_t *cx, uint8_t *cy, uint8_t *vx, uint8_t *vy, char ALT)
{
  //TODO: könnte man zusammenfassen bei gleicher funktion
  //Wenn ALT und Pfeile, Viewport verschieben
  if(ALT)
  {
    if(scancode==_UP && *vy>0)
      (*vy)--;
    if(scancode==_LEFT && *vx>0)
      (*vx)--;
    if(scancode==_DOWN && *vy<(BUFFER_HEIGHT-LCD_HEIGHT-1))
      (*vy)++;
    if(scancode==_RIGHT && *vx<(BUFFER_WIDTH-LCD_WIDTH-1))
      (*vx)++;
  }
  else //Wenn Pfeile ohne ALT, Cursor bis an Rand verschieben, dann Viewport
  {
    if(scancode==_UP)
    {
      if (*cy>0)
      {
        (*cy)--;
        if ((*cy-*vy)<0 && *vy>0)
          (*vy)--;
      }
    }
    else if(scancode==_LEFT)
    {
      if (*cx>0)
      {
        (*cx)--;
        if ((*cx-*vx)<0 && *vx>0)
          (*vx)--;
      }
    }
    else if(scancode==_DOWN)
    {
      if (*cy<(BUFFER_HEIGHT-1))
      {
        (*cy)++;
        if ((*cy-*vy+1)>LCD_HEIGHT && *vy<(BUFFER_HEIGHT-1))
          (*vy)++;
      }
    }
    else if(scancode==_RIGHT)
    {
      if (*cx<(BUFFER_WIDTH-1))
      {
        (*cx)++;
        if ((*cx-*vx+1)>LCD_WIDTH && *vx<(BUFFER_WIDTH-1))
          (*vx)++;
      }
    }
  }
  //DEBUG
  //printf("cx=%i, cy=%i, vx=%i, vy=%i\n",*cx,*cy,*vx,*vy);
}


// Scancode ist der Code, wie er von der 8x8 Folientastatur kommt
// 255 = keine Taste gedrückt
// 0..63 die jeweilige Taste (Siehe CSV Tabelle)

/*
 * TODO und BUGS
 * Umlaute äöü und ß
 */
int process_menu(uint8_t scancode)
{
  static uint8_t cursor_x=0;  //0..BUFFER_WIDTH-1
  static uint8_t cursor_y=0;  //0..BUFFER_HEIGHT-1
  static uint8_t viewport_x=0;
  static uint8_t viewport_y=0;
  static union _modifier_state modifier_state;
  static uint8_t last_scancode;

  //Nur Flanke auswerten
  if(last_scancode==0xff && scancode!=0xff)
  {
    //Taste gedrückt?
    if(scancode!=0xff)
    {
      caca_printf(cv,1,4,"8x8 scancode(0..63) = %i  ",scancode);
      //Wandlung von 8x8 Keycode auf ASCII oder Modifier
      if(scancode>63)
      {
        fprintf(stderr, "panic, keycode>63\n");
        exit(-1);
      }
      uint8_t c=characters[scancode];
      //Prüfen ob ASCII Zeichen
      if(c)
      {
        if(modifier_state.SHIFT || modifier_state.CAPS)
         c = toupper(c);

        //Ist es auf dem LCD anzeigbar?
        if(isprint(c))
        {
          //Shift zurücksetzen
          modifier_state.SHIFT=0;

          //Einfügen oder Überschreiben?
          if(modifier_state.OVERWRITE)
            text_buffer[cursor_y][cursor_x]=c;
          else
            insert_ch(cursor_x, cursor_y, c);

          //Ein Zeichen weiter rechts
          cursor_viewport_calc(_RIGHT, &cursor_x,&cursor_y,&viewport_x,&viewport_y, 0);

        }
        else
        {
          //Andere Funktion mit ASCII Zeichen wie \b, \r, \t
          //TODO: was mit \t machen?
          if(c=='\b')
          {
            if(cursor_x>0)
            {
              cursor_viewport_calc(_LEFT, &cursor_x,&cursor_y,&viewport_x,&viewport_y, 0);
              delete_ch(cursor_x, cursor_y);
            }
          }

        }
      }
      else /*muss ein Modifier, Cursor, Funktionstaste oder ähnlich sein*/
      {
        //printf("Not an ASCII char. scancode=%i\n",scancode);
        switch(scancode)
        {
          case _SHIFT: modifier_state.SHIFT=1; break;
          case _CAPS : modifier_state.CAPS=!modifier_state.CAPS; break;
          case _ALT  : modifier_state.ALT=!modifier_state.ALT; break;
          case _STRG : modifier_state.STRG=1; break;
          case _ENTF :if(modifier_state.SHIFT)
                        modifier_state.OVERWRITE=!modifier_state.OVERWRITE;
                      else
                        delete_ch(cursor_x, cursor_y);
                      break;
          default:
            break;
        }

        cursor_viewport_calc(scancode, &cursor_x,&cursor_y,&viewport_x,&viewport_y, modifier_state.ALT);


      }
    }
  }
  last_scancode=scancode;

  draw_lcd(viewport_x, viewport_y, cursor_x, cursor_y);
  draw_text_buffer();
  return 0;
}

//Text Buffer löschen
void clr_text_buffer()
{
  memset(text_buffer, ' ', (BUFFER_WIDTH+1)*BUFFER_HEIGHT);
  uint8_t i;
  for(i=0;i<BUFFER_HEIGHT;++i)
    text_buffer[i][BUFFER_WIDTH]=0;
}

//Zeichen einfügen (Rest nach rechts verschieben)
//Achtung: keine Überprüfung der Grenzen. Muss oberhalb passieren
void insert_ch(const uint8_t x, const uint8_t y, const char c)
{
  uint8_t i;
  for(i=(BUFFER_WIDTH-1); i>x; i--)
    text_buffer[y][i]=text_buffer[y][i-1];
  text_buffer[y][x]=c;
}

//Zeichen einfügen (Rest nach links verschieben)
//Achtung: keine Überprüfung der Grenzen. Muss oberhalb passieren
void delete_ch(uint8_t x, const uint8_t y)
{
  //man könnte auch memmove nehmen. In libc für AVR drin?
  for(; x<BUFFER_WIDTH-3; ++x)
    text_buffer[y][x]=text_buffer[y][x+1];
  text_buffer[y][BUFFER_WIDTH-2]=' ';
}

int main()
{
  clr_text_buffer();

  strncpy(text_buffer[0],"Matrix Test",17);
  strncpy(text_buffer[1],"1234567890123456",17);


  text_buffer[0][BUFFER_WIDTH-1]='%';
  text_buffer[3][BUFFER_WIDTH-1]='%';

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
    //printf("%i ",c);

    uint8_t key = ascii_to_key(c);

    process_menu(key);
    //einfach simulieren, die Taste würde länger als ein zyklus gedrückt
    //process_menu(key);

  }while(c!=CACA_KEY_CTRL_C);

  caca_free_display(dp);

  return 0;
}
