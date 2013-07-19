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

/*
 * TODO:
 * Kontaktprellen Tastatur
 *
 *
 *
 */

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
#include "../menu.h"
#include "../../../hf2gcode/src/libhf2gcode.h"

//#define UART_BAUD_RATE 38400
#define UART_BAUD_RATE 9600

volatile uint8_t do_update_lcd;
volatile uint8_t uart_error;
volatile uint16_t g_line;
volatile uint8_t running;

volatile uint16_t show_settings_time;  //in ms
uint8_t updated_settings;
uint8_t font_size=5;
char font_name[10];

uint16_t grbl_num_ok=0;
uint16_t grbl_num_err=0;
char grbl_error_msg[17];
uint8_t show_status_win;

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

void update_lcd(void)
{
  if(show_settings_time) //Einstellungen anzeigen
  {
    lcd_clrscr();
    lcd_gotoxy(0,0);
    lcd_puts(font_name);
    char buf[3];
    itoa(font_size,buf,10);
    lcd_puts(buf);
  }
  else //normaler Editor
  {
    lcd_command(LCD_DISP_ON);
    lcd_gotoxy(0,0);
    //_delay_us(30);
    uint8_t x;
    uint8_t cx, cy, vx, vy;
    get_viewport(&vx, &vy);
    get_cursor(&cx, &cy);
    for(x=0;x<LCD_WIDTH;x++)
      lcd_putc(get_text_buffer(vy)[vx+x]);
    for(x=0;x<LCD_WIDTH;x++)
      lcd_putc(get_text_buffer(vy+1)[vx+x]);
    lcd_gotoxy(cx-vx,cy-vy);
    lcd_command(LCD_DISP_ON_CURSOR);
  }
}

void update_status_lcd(void)
{
  lcd_gotoxy(0,0);
  //_delay_ms(1);
  //_delay_us(30);
  lcd_puts_P("OK:");
  lcd_put_int(grbl_num_ok,4);
  lcd_puts_P(" ERR:");
  lcd_put_int(grbl_num_err,4);
  lcd_gotoxy(0,1);
  //_delay_us(30);
  lcd_puts(grbl_error_msg);
  uint8_t l=16-strlen(grbl_error_msg);
  while(l--) lcd_putc(' ');
}

ISR(INT0_vect)
{
  TCNT1=0;
}

//ISR(TIMER0_COMP_vect, ISR_NOBLOCK) //1kHz
ISR(TIMER0_COMP_vect) //1kHz
{
  static int16_t cnt=0;
  if (cnt++ == 200)
  {
    cnt = 0;
    if(running && key_get()==51) // TAB
      uart_putc('~');
    //do_update_lcd=1;
  }

  if(updated_settings)
  {
      show_settings_time=2000;
      do_update_lcd=1;
      updated_settings=0;
  }
  else
  {
    if(show_settings_time>0)
    {
      show_settings_time--;
      if(!show_settings_time)
        do_update_lcd=1;
    }
  }
  if(TCNT1L>5 && bit_is_set(PIND,2))
    PORTD |= _BV(PD6);
  if(TCNT1L>1 && bit_is_clear(PIND,2))
    PORTD &= (uint8_t) ~_BV(PD6);
}

void get_grbl_response(void)
{
  uint16_t rx_tmp;
  char c=0;
  char temp[17];
  uint8_t i=0;

  //auf GRBL "ok\r\n" oder Fehlermeldung warten
  //Lesen bis 16 Zeichen oder \n empfangen

  //while(uart_GetRXCount()<4);
  //for(i=0;i<4;++i)
  do
  {
    rx_tmp = uart_getc ();
    if (rx_tmp & 0xFF00)
      uart_error=((rx_tmp & 0xFF00) >> 8);
    if(rx_tmp != UART_NO_DATA) //Zeichen empfangen
    {
      c = rx_tmp & 0xFF;
      temp[i++] = c;
    }
  }
  while(i<16 && c!='\n');
  if(c=='\n' && i>1) //kurze Antwort mit \r\n
    temp[i-2]=0;
  else
    temp[i]=0;
  if(i==16) //lange Fehlermeldung? Warten und dann empty read
  {
    _delay_ms(30);
    while(uart_getc()!=UART_NO_DATA); //empty read
  }

  if(!strcmp(temp,"ok"))
    grbl_num_ok++;
  else //wird wohl ein Fehler sein
  {
    grbl_num_err++;
    strncpy(grbl_error_msg, temp, 17);
    grbl_error_msg[16]=0;
  }
  update_status_lcd();
}

static int uart_putchar(char c, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

static int uart_putchar(char c, FILE *stream)
{
  uart_putc(c);
  return 0;
}

/*
 *      //~ uart_puts_P("G21\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G90\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G94\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G17\n");
        //~ get_grbl_response();
        //~ uart_puts_P("M3 S1000\n");
        //~ get_grbl_response();
        //~ uart_puts_P("F800.00\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G0 Z1.00\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G0 X15 Y15\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G1 Z-1\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G2 X25 Y25 I10\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G0 Z1\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G0 X35 Y15\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G1 Z-1\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G2 X25 Y5 I-10\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G0 Z1\n");
        //~ get_grbl_response();
        //~ uart_puts_P("G0 X15 Y15\n");
        //~ get_grbl_response();
        //~ uart_puts_P("M5\n");
        //~ get_grbl_response();
        //~ uart_puts_P("M30\n");
*/
int main(void)
{
  stdout = &mystdout;
  //uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU));
  uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
  lcd_init(LCD_DISP_ON_CURSOR);
  lcd_clrscr();
  lcd_gotoxy(0,0);
  lcd_puts_P("Needler v0.7\n");
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
  strncpy(font_name,"rowmans",10);

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

  /** TIMER1 **/
  //External clock source on T1 pin. Clock on rising edge.
  TCCR1B = _BV(CS12) | _BV(CS11) | _BV(CS10);
  //OCR1x=5;

  /** External Interrupt INT0 PD2 **/
  //Any logical change on INT0 generates an interrupt request.
  MCUCR = _BV(ISC00);
  GICR = _BV (INT0);

  //enable global interrupts
  sei();
  uint8_t debounce_key=0, last_key=0;
  uint8_t key, event=0;
  for (;;)    /* main event loop */
    {
      key=key_get();
      if(key==last_key)
        debounce_key++;
      else
        debounce_key=0;
      if(debounce_key>50)
      {
        event=process_menu(key);
        debounce_key=0;
      }
      last_key=key;

      if(do_update_lcd || event)
      {
        update_lcd();
        do_update_lcd=0;
      }

      if (bit_is_clear(PIND,3) && !running)
      {
        grbl_num_err=0;
        grbl_num_ok=0;
        //empty read
        while(uart_getc()!=UART_NO_DATA);
        running=1;
        update_status_lcd();
        uart_puts_P("$X\n");
        get_grbl_response();
        strncpy(grbl_error_msg, "$H:Referenzfahrt",17);
        update_status_lcd();
        uart_puts_P("$H\n");
        get_grbl_response();
        strncpy(grbl_error_msg, "Gravur laeuft...",17);
        update_status_lcd();

       //int init_get_gcode_line ( char *font, char *text, double X0, double Y0, double Z_up, double Z_down, double yinc, double scale,
       //double feed, int precision, char verbose, char align, char use_inch);

        //int r = init_get_gcode_line("rowmans", "Hello world!", 1, 1, 1, -1, 7, 0.3, 1100, 3, 0, 'l', 0);
        uint8_t line_nr;
        double scale=font_size*0.047619;
        double x0, y0;
        char line[BUFFER_WIDTH];
        for(line_nr=0; line_nr<BUFFER_HEIGHT; line_nr++)
        {
          strncpy(line, get_text_buffer(line_nr), BUFFER_WIDTH);
          //Leerzeichen am Ende mit 0 füllen
          int8_t len = BUFFER_WIDTH-1;
          while(len>=0 && line[len]==' ') line[len--]=0;

          //Positionsberechnung
          //Die Alukärtchen haben 85x54mm
          x0 = 7; //10mm vorerst fix, wie einstellbar?
          y0 = 50 - (line_nr+1) * (font_size*1.7);  //70% der Zeichenhöhe als Zeilenabstand

          //~ uart_puts_P("-");
          //~ uart_puts(line);
          //~ uart_puts_P("-");
          //~ char xtemp[5];
          //~ itoa(len,xtemp,10);
          //~ uart_puts(xtemp);
          //~ uart_puts_P("----");

          if(len>=0)
          {
            init_get_gcode_line(font_name, line, x0, y0, 1, -1, 0, scale, 1300, 2, 0, 'l', 0);
            char buf[200];
            while((g_line = get_gcode_line (buf, 200))!=-1)
            {
              uart_puts(buf);
              uart_putc('\n');
              get_grbl_response();
            }
          }
        }
        strncpy(grbl_error_msg, "*** BEENDET  ***",17);
        update_status_lcd();
        PORTD &= (uint8_t) ~_BV(PD6);
        uart_puts_P("G0X1Y1\n");
        uart_puts_P("M30\n");
        //empty read
        while(uart_getc()!=UART_NO_DATA);
        //BEENDET etwas stehen lassen
        for(uint8_t i=0;i<200;++i)
          _delay_ms(10);
        grbl_error_msg[0]=0;
        do_update_lcd=1;
        running=0;
      }
    }
    return 0;
}
