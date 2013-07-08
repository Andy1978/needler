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

/*************************************************
 * Hardware:
 * PD0: IN : RxD
 * PD1: OUT: TxD
 * PD2: IN : Z_DIR
 * PD3: IN : Enable/FEED HOLD. 0=betätigt
 * PD4,5: not used
 * PD6: Relais Pneumatikventil. 1=EIN
 * PD7: not used
 *
 * PORTA+PORTC : Matrix Tastatur
 * PB0: LCD RS
 * PB1: IN: Z_STEP
 * PB2: LCD R/W
 * PB3: LCD E
 * PB4-7: LCD D04-D07
 *
 * ***********************************************/



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd.h"
#include "uart.h"
#include "keymatrix.h"
#include "../keymapping.h"
#include "../menu.h"
//#include "../../hf2gcode/src/libhf2gcode.h"

//#define UART_BAUD_RATE 38400
#define UART_BAUD_RATE 9600

volatile uint8_t do_update_lcd;
volatile uint8_t uart_error;
volatile uint16_t g_line;

char text_buffer[BUFFER_HEIGHT][BUFFER_WIDTH+1];
uint8_t cursor_x;  //0..BUFFER_WIDTH-1
uint8_t cursor_y;  //0..BUFFER_HEIGHT-1
uint8_t viewport_x;
uint8_t viewport_y;

//LCD großer Ausschnitt von text_buffer zeigen
//Cursor cx, cy wird dargestellt

//~ void draw_lcd(char *line, uint8_t cx, uint8_t cy)
//~ {
  //~ //lcd_command(LCD_DISP_OFF);
  //~ lcd_puts("huhu welt");
  //~ lcd_gotoxy(0,0);
  //~ //lcd_gotoxy(cx,cy);
  //~ //lcd_command(LCD_DISP_ON_CURSOR_BLINK);
//~ }


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

/*
void update_lcd(void)
{
  char buf[20];
  lcd_gotoxy(0,0);
  _delay_ms(2);   //sonst zickt gotoxy rum, TODO: nachprüfen, ggf. Zeit verkleinern
  //lcd_puts_P("hello world!");
  //dtostrf(M_PI,5,2,buf);
  itoa(g_line,buf,10);
  lcd_puts(buf);
  //snprintf(buf, 20, "F%.*f", _precision, _feed);
  int cnt;
  snprintf(buf, 20, "F%.2f%n", 123.45, &cnt);
  lcd_puts(buf);

  lcd_gotoxy(0,1);
  _delay_ms(2);   //sonst zickt gotoxy rum, TODO: nachprüfen, ggf. Zeit verkleinern
  itoa(cnt, buf, 10);
  lcd_puts(buf);
}
*/

//ISR(TIMER0_COMP_vect, ISR_NOBLOCK) //1kHz
ISR(TIMER0_COMP_vect) //1kHz
{
  static int16_t cnt=0;

  uint8_t temp=key_get();
  process_menu(temp);
  //~ if(temp!=255)
  //~ {
    //~ lcd_gotoxy(0,0);
    //~ lcd_puts("hello");
  //~ }
  if (cnt++ == 200)
  {
    cnt = 0;
    //lcd_command(LCD_DISP_ON);
    lcd_gotoxy(0,0);

    uint8_t x;
    for(x=0;x<LCD_WIDTH;x++)
      lcd_putc(text_buffer[viewport_y][viewport_x+x]);
    for(x=0;x<LCD_WIDTH;x++)
      lcd_putc(text_buffer[viewport_y+1][viewport_x+x]);

    lcd_gotoxy(cursor_x-viewport_x,cursor_y-viewport_y);
    lcd_command(LCD_DISP_ON_CURSOR);
  }
}

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
  uart_putc(c);
  return 0;
}

int main(void)
{
  stdout = &mystdout;
  //uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU));
  uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
  lcd_init(LCD_DISP_ON_CURSOR_BLINK);
  lcd_clrscr();
  lcd_gotoxy(0,0);
  lcd_puts_P("Needler v0.3\n");
  lcd_gotoxy(0,1);
  lcd_puts_P(__DATE__" aw");

  //Delay for Splash
  for(uint8_t i=0;i<160;++i)
    _delay_ms(15);

  //PD2: IN : Z_DIR
  //PD3: IN : Enable/FEED HOLD. 0=betätigt
  //PD6: OUT: Relais Pneumatikventil. 1=EIN
  DDRD  |= _BV(PD6);
  PORTD |= _BV(PD2) | _BV(PD3) | _BV(PD4) | _BV(PD5) | _BV(PD7);

  //PB1: IN: Z_STEP
  PORTB |= _BV(PB1);

  lcd_clrscr();
  clr_text_buffer();

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
  //TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1A0) | _BV(COM1B0) | _BV(WGM11) | _BV(WGM10);
  //Prescaler = 1 (page 110)
  //TCCR1B = _BV(CS10);

  //enable global interrupts
  sei();
  uint8_t running=0;
  for (;;)    /* main event loop */
    {
      //printf("abcdef\n");
      processUART();


      if (bit_is_clear(PIND,3) && !running)
      {
        running=1;
        uart_puts_P("$X\n");
        uart_puts_P("$H\n");
        uart_puts_P("G21\n");
        uart_puts_P("G90\n");
        uart_puts_P("G64\n");
        uart_puts_P("G40\n");
        uart_puts_P("G49\n");
        uart_puts_P("G94\n");
        uart_puts_P("G17\n");
        uart_puts_P("M3 S1000\n");
        uart_puts_P("F800.00\n");
        uart_puts_P("G0 Z1.00\n");
        uart_puts_P("G0 X15 Y15\n");
        uart_puts_P("G1 Z-1\n");
        uart_puts_P("G2 X25 Y25 I10\n");
        uart_puts_P("G0 Z1\n");
        uart_puts_P("G0 X35 Y15\n");
        uart_puts_P("G1 Z-1\n");
        uart_puts_P("G2 X25 Y5 I-10\n");
        uart_puts_P("G0 Z1\n");
        uart_puts_P("G0 X15 Y15\n");
        uart_puts_P("M5\n");
        uart_puts_P("M30\n");

        for(uint8_t i=0;i<160;++i)
          _delay_ms(15);
      }
      if (bit_is_set(PIND,3))
        running=0;


      //if(do_update_lcd)
      //{
        //update_lcd();
      //  do_update_lcd=0;
      //}
/*
      if (bit_is_clear(PIND,3))
      {

        int r = init_get_gcode_line("rowmans", "Hello world!", 0, 0, 1, -1, 15, 0.23, 500, 3, 1, 'l', 0);
        char buf[200];
        char lcdbuf[20];
        while((g_line = get_gcode_line (buf, 200))!=-1)
        {
          uart_puts(buf);
          uart_putc('\n');
          //itoa(g_line,lcdbuf,10);
          //lcd_gotoxy(0,0);
          for(uint8_t j=0;j<10;j++)
            _delay_ms(10);

          //lcd_puts(lcdbuf);
          PORTA ^= (uint8_t)_BV(5);
        }
        PORTA &= (uint8_t)~_BV(4);
*/

        /*
        const char *glyph=get_glyph_ptr("rowmans", 'H');
        char* current_glyph=0;

        current_glyph = malloc(strlen_PF(glyph)+1);
        strcpy_PF (current_glyph, glyph);
        uart_puts(current_glyph);
        uart_putc('\n');
        free(current_glyph);
        */
/*      }
      else
       PORTA |= _BV(4);
*/
    }
    return 0;
}