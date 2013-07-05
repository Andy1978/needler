#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd.h"
#include "keymatrix.h"

/* Scancode 23 und 30 ist nicht belegt (nur 62 Tasten) */
// TODO: in flash schieben
const char characters[64] = {  0,   0,   0,    0,   0, ' ',   0,    0,
                             'm', 'b', 'c',  'y', '.',   0,   0,    0,
                             'n', 'v', 'x',    0, ',', '@',   0,    0,
                             'j', 'g', 'd',  'a', 'l', '#',   0,  '0',
                             'h', 'f', 's',    0, 'k', '<',   0, '\b',
                             'u', 't', 'e',  'q', 'o', '+',   0,  '?',
                             'z', 'r', 'w', '\t', 'i', 'p', '8',  '9',
                             '3', '2', '1',    0, '4', '5', '6',  '7'};

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

ISR(TIMER0_COMP_vect)
{
  lcd_gotoxy(0,0);
  uint8_t temp=key_get();
  char buf[3];
  itoa(temp,buf,16);
  lcd_puts(buf);
  lcd_putc(' ');
  if(temp!=0xff)
  {
    uint8_t c=characters[temp];
    if(c) lcd_putc(c);
  }
  lcd_puts("__");

}

int main(void)
{
  lcd_init(LCD_DISP_ON);
  lcd_clrscr();
  lcd_gotoxy(0,0);
  lcd_puts_P("matrixtest\n");

  //Delay for Splash
  for(uint8_t i=0;i<40;++i)
    _delay_ms(15);

  lcd_clrscr();

  /*** TIMER0 ***/
  OCR0=250;
  //CTC = Clear Timer on Compare match S.80
  //Normal port operation, OC0 disconnected
  //Prescaler=64 -> clk=250kHz
  TCCR0 = _BV(WGM01) | _BV(CS01) | _BV(CS00);
  //On Compare match Interrupt Enable for timer 0
  TIMSK |= _BV(OCIE0);

  /* Init keyboard driver */
  key_init();
  //enable global interrupts
  sei();

  /* Get keys and transmit forever */
  do
  {

  } while(1);
  return 0;
}
