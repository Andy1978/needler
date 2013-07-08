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
#include "../keymapping.h"

ISR(TIMER0_COMP_vect)
{
  /*
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
  * */
  
  uint8_t temp=key_get();
  process_menu(temp);

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
