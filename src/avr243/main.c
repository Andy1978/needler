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

//*** Character conversion table ***

/*
const char __flash characters[4][32] = {
                       { '1', '2', '3', 0, 0, 0, 0, 0,  // No alternation
                             '4', '5', '6', 0, 0, 0, 0, 0,
                             '7', '8', '9', 0, 0, 0, 0, 0,
                             '*', '0', '#', 0, 0, 0, 0, 0 },

                           { 'a', 'd', 'g', 0, 0, 0, 0, 0,  // First character
                             'j', 'm', 'p', 0, 0, 0, 0, 0,
                             's', 'v', 'y', 0, 0, 0, 0, 0,
                             '*', '.', '#', 0, 0, 0, 0, 0 },

                           { 'b', 'e', 'h', 0, 0, 0, 0, 0,  // Second character
                             'k', 'n', 'q', 0, 0, 0, 0, 0,
                             't', 'w', 'z', 0, 0, 0, 0, 0,
                             '*', '?', '#', 0, 0, 0, 0, 0 },

                           { 'c', 'f', 'i', 0, 0, 0, 0, 0,  // Third character
                             'l', 'o', 'r', 0, 0, 0, 0, 0,
                             'u', 'x', '-', 0, 0, 0, 0, 0,
                             '*', '!', '#', 0, 0, 0, 0, 0 }
                         };

*/

/*
char convertKey(void)
{
  char tempChar;

  if( key_code.altKey0 )          // First char ?
  {
    tempChar = characters[1][key_code.scan];

  } else if( key_code.altKey1 )     // Second char ?
  {
    tempChar = characters[2][key_code.scan];

  } else if( key_code.altKey2 )     // Third char ?
  {
    tempChar = characters[3][key_code.scan];

  } else                  // No alternation ?
  {
    tempChar = characters[0][key_code.scan];
  }


  if( key_code.lckKey0 )          // Uppercase if caps lock
  {
    tempChar = toupper( tempChar );
  }


  return tempChar;
}
*/

ISR(TIMER0_COMP_vect)
{
  lcd_gotoxy(0,0);
  uint8_t temp=key_get();
  char buf[3];
  itoa(temp,buf,16);
  lcd_puts(buf);

}

/*** Main test application loop ***/
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
/*
    char tempChar;

    key_get();                // Wait for keypress
    //PORTA = ~key_altState;          // Show flags on STK500 LEDs

    tempChar = convertKey();        // Convert to character

//    while( key_code.scan != 0xFF )      // Uncomment to implement simple key repetition
    {
      sendChar( tempChar );       // Transmit character until key released
    }
 */
  } while(1);
  return 0;
}
