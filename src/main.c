/*
    Copyright 2013 Andreas Weber (andy.weber.aw at gmail dot com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*************************************************

  Needler AVR firmware

  Autor: Andreas Weber
  src: https://github.com/Andy1978/Needler
  changelog: 23.05.2013 created by Andy

**************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd.h"
#include "uart.h"
#include "../../hf2gcode/src/libhf2gcode.h"

#define UART_BAUD_RATE 38400

volatile uint8_t do_update_lcd;
volatile uint8_t uart_error;
volatile uint16_t g_line;

//  Integer (Basis 10) rechtsbündig auf LCD ausgeben.
void lcd_put_int(int16_t val, uint8_t len)
{
  char buffer[len+1];
  itoa(val,buffer,10);
  size_t empty=len-strlen(buffer);
  while(empty--)
    lcd_putc(' ');
  lcd_puts(buffer);
}

void lcd_put_int32(int32_t val, uint8_t len)
{
  char buffer[len+1];
  ltoa(val,buffer,10);
  size_t empty=len-strlen(buffer);
  while(empty--)
    lcd_putc(' ');
  lcd_puts(buffer);
}

/*void update_lcd(void)
{
  char buf[20];
  lcd_gotoxy(0,0);
  _delay_ms(2);   //sonst zickt gotoxy rum, TODO: nachprüfen, ggf. Zeit verkleinern
  //lcd_puts_P("hello world!");
  //dtostrf(M_PI,5,2,buf);
  itoa(g_line,buf,10);
  lcd_puts(buf);
}
*/

//ISR(TIMER0_COMP_vect, ISR_NOBLOCK) //1kHz
ISR(TIMER0_COMP_vect) //1kHz
{
  static int16_t cnt=0;
  if (cnt++ == 1000)
  {
    cnt = 0;
  }else if(cnt==500)
    do_update_lcd=1;
}

ISR(ADC_vect) //ca. 125kHz
{
  int16_t tmp=ADC-512;
}

// UART bearbeiten. Es gibt nur ein Telegramm mit allen Sollwerten
// und eine Antwort mit allen Istwerten bzw. Status
void processUART(void)
{
  //~ //Alle Daten empfangen
  //~ while(uart_GetRXCount()>=sizeof(struct s_setvalues))
  //~ {
    //~ //Empfangen
    //~ char* rec=(char*)&setvalues;
    //~ uint8_t i;
    //~ uint16_t rx_tmp;
    //~ for(i=0;i<sizeof(struct s_setvalues);i++)
    //~ {
      //~ rx_tmp=uart_getc();
      //~ rec[i]=rx_tmp & 0xFF;
      //~ if(rx_tmp & 0xFF00)
      //~ {
        //~ status.uart_error_cnt++;
        //~ uart_error=((rx_tmp & 0xFF00) >> 8);
      //~ }
    //~ }
    //~ //status.bits |= setvalues.bits;
    //~ //Senden
    //~ char* send=(char*)&status;
    //~ for(i=0;i<sizeof(struct s_status);i++)
    //~ {
      //~ uart_putc(send[i]);
    //~ }
  //~ }
}

static int uart_putchar(char c, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

static int uart_putchar(char c, FILE *stream)
{

  //if (c == '\n')
  //  uart_putchar('\r', stream);
  //loop_until_bit_is_set(UCSRA, UDRE);
  //UDR = c;
  uart_putc(c);
  return 0;
}

int main(void)
{
  stdout = &mystdout;
  //uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU));
  uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
  lcd_init(LCD_DISP_ON);
  lcd_clrscr();
  lcd_puts_P("Needler v0.1\n");
  lcd_gotoxy(0,1);
  lcd_puts_P(__DATE__" aw");

  //3s Delay for Splash
  for(uint8_t i=0;i<60;++i)
    _delay_ms(15);
  lcd_clrscr();

  //H-Brücke
  //DDRD |= _BV(PD2) | _BV(PD4) | _BV(PD5) | _BV(PD6) | _BV(PD7);
  //Relais für Heizung
  //DDRB |= _BV(PB3);
  //PORTD |= _BV(PD6) | _BV(PD7);

  //PullUp für den linken Taster
  //(der rechte Taster kann nicht verwendet werden da auch TxD)
  PORTD |= _BV(PD3);

  //2 LEDs als Ausgang
  DDRA |= _BV(PA4) | _BV(PA5);

  /*** TIMER0 ***/
  OCR0=250;
  //CTC = Clear Timer on Compare match S.80
  //Normal port operation, OC0 disconnected
  //Prescaler=64 -> clk=250kHz
  TCCR0 = _BV(WGM01) | _BV(CS01) | _BV(CS00);
  //On Compare match Interrupt Enable for timer 0
  TIMSK |= _BV(OCIE0);

  /** TIMER1 **/
  //PWM Phase correct 10bit
  //Set OC1A+OC1B on match when upcounting (page 108)
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1A0) | _BV(COM1B0) | _BV(WGM11) | _BV(WGM10);
  //Prescaler = 1 (page 110)
  TCCR1B = _BV(CS10);


  /*** ADC ***/
  //Prescaler 128 = 125kHz ADC Clock, AutoTrigger, Interrupts enable
  ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2) | _BV(ADATE) | _BV(ADSC) | _BV(ADIE);

  //AVCC with external capacitor at AREF, internal 2.56V bandgap
  //siehe S. 215
  //8=Multiplexer ADC0 positive Input, ADC0 negative, 10x gain
  //9=Multiplexer ADC1 positive Input, ADC0 negative, 10x gain
  ADMUX = _BV(REFS1) | _BV(REFS0) | 11;
  //ADC in Free Running mode
  SFIOR &= ~(_BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0));

  //enable global interrupts
  sei();
  for (;;)    /* main event loop */
    {
      //printf("abcdef\n");
      processUART();
      if(do_update_lcd)
      {
        //update_lcd();
        do_update_lcd=0;
      }
      if (bit_is_clear(PIND,3))
      {

        int r = init_get_gcode_line("rowmans", "He", 0, 0, 1, -1, 15, 0.23, 500, 3, 1, 'l');
        char buf[200];
        char lcdbuf[20];
        while((g_line = get_gcode_line (buf, 200))!=-1)
        {
          uart_puts(buf);
          uart_putc('\n');
          itoa(g_line,lcdbuf,10);
          lcd_gotoxy(0,0);
          for(uint8_t j=0;j<10;j++)
            _delay_ms(10);

          lcd_puts(lcdbuf);
          PORTA ^= (uint8_t)_BV(5);
        }
        PORTA &= (uint8_t)~_BV(4);


        /*
        const char *glyph=get_glyph_ptr("rowmans", 'H');
        char* current_glyph=0;

        current_glyph = malloc(strlen_PF(glyph)+1);
        strcpy_PF (current_glyph, glyph);
        uart_puts(current_glyph);
        uart_putc('\n');
        free(current_glyph);
        */
      }
      else
       PORTA |= _BV(4);
    }
    return 0;
}
