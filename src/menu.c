#include "menu.h"

#ifndef AVR
  extern caca_canvas_t *cv;
#endif

/* ALT rastet und man kann den "viewport" mit den Cursortasten verschieben
 * CAPS rastet für Großschreibung
 * STRG + Pfeil hoch = größere Schrift
 * STRG + Pfeil runter = kleinere Schrift
 * POS1 springt an den Anfang der Zeile
 * ENDE ans Ende
 * ESC verlässt den Editor oder im Menü eine Ebene höher springen
 * ENTER im Editormodus ans Ende der nächsten Zeile
 * Mit SHIFT+ENTF kann zwischen Einfügemodus und Überschreibmodus umgeschaltet werden
 * F1 Schrift kleiner
 * F2 Schrift größer
 * F4 Schrift von rowmans zu scripts und umgekehrt wechseln
 *
 * TODO bzw. zu überlegen
 * Soll "ENTER" eine neue Zeile einfügen und die alte, unterste rauswerfen? Vorerst nicht so implementiert
 * äöüß ?
 */

char text_buffer[BUFFER_HEIGHT][BUFFER_WIDTH+1];
uint8_t cursor_x;  //0..BUFFER_WIDTH-1
uint8_t cursor_y;  //0..BUFFER_HEIGHT-1
uint8_t viewport_x;
uint8_t viewport_y;

extern uint8_t font_size;
extern char font_name[10];
extern uint8_t updated_settings;

/* Scancode 23 und 30 ist nicht belegt (nur 62 Tasten) */
// TODO: bei AVR in Flash schieben?
const char characters[64] = {  0,   0,   0,    0,   0, ' ',   0,    0,
                             'm', 'b', 'c',  'y', '.','\r',   0,    0,
                             'n', 'v', 'x',    0, ',', '@',   0,    0,
                             'j', 'g', 'd',  'a', 'l', '#',   0,  '0',
                             'h', 'f', 's',    0, 'k', '<',   0, '\b',
                             'u', 't', 'e',  'q', 'o', '+',   0,  '?',
                             'z', 'r', 'w', '\t', 'i', 'p', '8',  '9',
                             '3', '2', '1',    0, '4', '5', '6',  '7'};


/****** GETTER ************/
void get_cursor(uint8_t *cx, uint8_t *cy)
{
  *cx=cursor_x;
  *cy=cursor_y;
}

void get_viewport(uint8_t *vx, uint8_t *vy)
{
  *vx=viewport_x;
  *vy=viewport_y;
}

const char* get_text_buffer(uint8_t line)
{
  if(line<(BUFFER_HEIGHT))
    return text_buffer[line];
  else
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
  for(; x<BUFFER_WIDTH-1; ++x)
    text_buffer[y][x]=text_buffer[y][x+1];
  text_buffer[y][BUFFER_WIDTH-1]=' ';
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
    if(scancode==_UP && (*cy)>0)   (*cy)--;
    if(scancode==_LEFT && (*cx)>0) (*cx)--;
    if(scancode==_DOWN && (*cy<(BUFFER_HEIGHT-1))) (*cy)++;
    if(scancode==_RIGHT && (*cx<(BUFFER_WIDTH-1))) (*cx)++;
    //DEBUG
    //printf("cx=%i cy=%i vx=%i vy=%i\n",*cx,*cy,*vx,*vy);
    if ((*cy-*vy)<0) (*vy)=(*cy);
    if ((*cx-*vx)<0) (*vx)=(*cx);
    if ((*cy-*vy)>(LCD_HEIGHT-1)) (*vy)=(*cy)-LCD_HEIGHT+1;
    if ((*cx-*vx)>(LCD_WIDTH-1)) (*vx)=(*cx)-LCD_WIDTH+1;
  }
}

// Die Position des Textendes
// (nicht \0 sondern das letzte druckbare Zeichen)
// zurückgeben
uint8_t get_line_end(uint8_t y)
{
  uint8_t i;
  for(i=BUFFER_WIDTH-1;i>0 && (text_buffer[y][i]==' ');i--);
  return (i==(BUFFER_WIDTH-1) || !i)? i:i+1;
}

// Scancode ist der Code, wie er von der 8x8 Folientastatur kommt
// 255 = keine Taste gedrückt
// 0..63 die jeweilige Taste (Siehe CSV Tabelle)

int process_menu(uint8_t scancode)
{
  static union _modifier_state modifier_state;
  static uint8_t last_scancode;
  uint8_t ret=0;

  //Nur Flanke auswerten
  if(last_scancode==0xff && scancode!=0xff)
  {
    //Taste gedrückt?
    if(scancode!=0xff)
    {
#ifndef AVR
      caca_printf(cv,1,4,"8x8 scancode(0..63) = %i  ",scancode);
#endif
      //Wandlung von 8x8 Keycode auf ASCII oder Modifier
      uint8_t c=characters[scancode];
      uint8_t printable_char=0;
      //Prüfen ob ASCII Zeichen
      if(c)
      {
        //printf("%c %i\n",c,c);
        if(modifier_state.SHIFT || modifier_state.CAPS)
        { //alternative Belegung
          switch(c)
          {
            case '1': c='!'; break;
            case '2': c='"'; break;
            case '3': c='^'; break;
            case '4': c='$'; break;
            case '5': c='%'; break;
            case '6': c='&'; break;
            case '7': c='/'; break;
            case '8': c='('; break;
            case '9': c=')'; break;
            case '0': c='='; break;
            case '\\': c='?'; break;
            case '+': c='*'; break;
            case '#': c='\''; break;
            case '<': c='>'; break;
            case ',': c=';'; break;
            case '.': c=':'; break;
            case '@': c='|'; break;
            default:
             c = toupper(c);
             break;
          }
        }
        //Ist es auf dem LCD anzeigbar?
        if(isprint(c))
        {
          //Shift zurücksetzen
          modifier_state.SHIFT=0;
          printable_char=c;
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
          else if(c=='\r')
          {
            cursor_viewport_calc(_DOWN, &cursor_x,&cursor_y,&viewport_x,&viewport_y, 0);
            cursor_x=get_line_end(cursor_y);
            cursor_viewport_calc(0, &cursor_x,&cursor_y,&viewport_x,&viewport_y, 0);
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
          case _ENDE :  cursor_x=(modifier_state.SHIFT)?0:get_line_end(cursor_y);
                        modifier_state.SHIFT=0;
                        break;
          case _F1: if(font_size>2) font_size--; updated_settings=1; break;
          case _F2: if(font_size<9) font_size++; updated_settings=1; break;
          case _F4: strncpy(font_name,strcmp(font_name,"rowmans")? "rowmans": "scripts", 10); updated_settings=1; break;

          default:
            break;
        }

        //Nur wenn nicht SHIFT, sonst könnten es
        //Cursortasten -_{} oder F3, F4 [] sein
        if(!modifier_state.SHIFT)
          cursor_viewport_calc(scancode, &cursor_x,&cursor_y,&viewport_x,&viewport_y, modifier_state.ALT);
        else
          switch(scancode)
          {
            case _DOWN:  printable_char='-'; break;
            case _LEFT:  printable_char='_'; break;
            case _UP:    printable_char='{'; break;
            case _RIGHT: printable_char='}'; break;
            case _F3:    printable_char='['; break;
            case _F4:    printable_char=']'; break;
            default:
              break;
          }
          if(printable_char) //Shift zurücksetzen
            modifier_state.SHIFT=0;
      }
      if(isprint(printable_char))
      {
        //Einfügen oder Überschreiben?
        if(modifier_state.OVERWRITE)
          text_buffer[cursor_y][cursor_x]=printable_char;
        else
          insert_ch(cursor_x, cursor_y, printable_char);

        //Ein Zeichen weiter rechts
        cursor_viewport_calc(_RIGHT, &cursor_x,&cursor_y,&viewport_x,&viewport_y, 0);
      }
      ret=1;
    }
  }
  last_scancode=scancode;

#ifndef AVR
  if(scancode!=0xff)
  {
    char line0[LCD_WIDTH+1];
    char line1[LCD_WIDTH+1];

    strncpy(line0,text_buffer[viewport_y]+viewport_x,LCD_WIDTH);
    strncpy(line1,text_buffer[viewport_y+1]+viewport_x,LCD_WIDTH);
    line0[LCD_WIDTH]=0;
    line1[LCD_WIDTH]=0;

    draw_lcd(line0, line1, viewport_x, viewport_y, cursor_x, cursor_y);
  }
#endif

  return ret;
}
