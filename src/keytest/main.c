#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <caca.h>

#include "../keymapping.h"

/*
 * 6.7.2013 Andreas Weber
 * Testprogramm für die Menüsteuerung
 */
 
char lcd_lines[2][17];
caca_canvas_t *cv;

void draw_lcd()
{

  
  #define LCD_LEFT 1
  #define LCD_TOP 6
  #define LCD_WIDTH 16
  #define LCD_HEIGHT 2
  caca_draw_cp437_box(cv, LCD_LEFT, LCD_TOP, LCD_WIDTH+2, LCD_HEIGHT+2);
  caca_printf(cv, LCD_LEFT+1, LCD_TOP+1, lcd_lines[0]);
  caca_printf(cv, LCD_LEFT+1, LCD_TOP+2, lcd_lines[1]);
  
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

// Scancode ist der Code, wie er von der 8x8 Folientastatur kommt
// 255 = keine Taste gedrückt
// 0..63 die jeweilige Taste (Siehe CSV Tabelle)

int process_menu(uint8_t scancode)
{
  //Cursor für Texteingabe
  #define CURSOR_MAX_X 15
  #define CURSOR_MAX_y 2
  
  static uint8_t cursor_x=0;  //0..15
  static uint8_t cursor_y=0;  //0..1
  static union _modifier_state modifier_state; 
  static uint8_t last_scancode;
  
  //Nur Flanke auswerten
  if(last_scancode==0xff && scancode!=0xff)
  {
    //Taste gedrückt?
    if(scancode!=0xff)
    {
      caca_printf(cv,0,12,"8x8 last scancode = %i  ",scancode);
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
          lcd_lines[cursor_y][cursor_x]=c;
          //Shift zurücksetzen
          modifier_state.SHIFT=0;
          if (cursor_x<CURSOR_MAX_X) cursor_x++;
        }
      }
      else /*muss ein Modifier sein*/
      {
        printf("must be a modifier. scancode=%i\n",scancode);
        switch(scancode)
        {
          case _SHIFT: modifier_state.SHIFT=1; break;
          case _CAPS : modifier_state.CAPS=!modifier_state.CAPS; break;
          case _ALT  : /* TODO */ break;
          case _STRG : /* TODO */ break;
          default:
            break;
        }
        
      }
      
    }
  }
  last_scancode=scancode;
  return 0;
}

int main()
{
  strncpy(lcd_lines[0],"Matrix Test",17);
  strncpy(lcd_lines[1],"1234567890123456",17);
  
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
    int ret = caca_get_event(dp, CACA_EVENT_KEY_PRESS, &ev, 1000);
    c = (ret)? caca_get_event_key_ch(&ev):0xff;
    //printf("%i ",c);

    uint8_t key = ascii_to_key(c);
  
    process_menu(key);
    //einfach simulieren, die Taste würde länger als ein zyklus gedrückt
    process_menu(key);
    

    draw_lcd(cv);
  }while(c!=CACA_KEY_CTRL_C);
  
  caca_free_display(dp);

  return 0;
}
