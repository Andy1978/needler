//************************************************
//* Matrix keyboard decoder
//* Main driver source file
//*
//* Filename: keymatrix.c
//* Version:  1.0
//*
//* Device:   ATmega162
//* Clock:    Internal 1.000 MHz RC oscillator
//*
//************************************************
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "keymatrix.h"

#define COLPORT PORTA             // Column io registers
#define COLPIN PINA
#define COLDIR DDRA

#define ROWPORT PORTC            // Row io registers
#define ROWPIN PINC
#define ROWDIR DDRC

/*** Global variables ***/
volatile unsigned char key_altState;      // Holding current alternation flags
volatile union _key_code key_code;        // Scan result structure


/*** Driver initialization ***/
//*
//* Sets up necessary ports and variables.
//* Use this at startup or when
//* restarting the driver after a stop.
//*
void key_init()
{
  /* Init global variables */
  key_altState = 0;             // Clear alternation flags
  key_code.complete = 0;            // Clear scan result


  /* Init ports */
  ROWDIR = 0xFF;                // Set row lines to output
  ROWPORT = 0x00;             // Drive all row lines low

  COLDIR = 0x00;              // Set column lines to input
  COLPORT = 0xFF;               // Pull column lines high

}

unsigned char key_get()
{
  /* Local variables */
  uint8_t lineResult=0xff;         // Resulting column lines
  uint8_t rowResult;          // Resulting row lines
  uint8_t tempScan;           // Temporary scancode
  uint8_t debcnt=0;


  //wait until COLPIN is stable for at least 100us
  while(debcnt<100)
  {
    if(COLPIN!=lineResult)
      debcnt=0;
    else
      debcnt++;
    lineResult = COLPIN;
  }

  if( lineResult != 0xFF )          // Any column lines low ?
  {
    /* Invert port directions */
    ROWPORT = 0xFF;               // Drive all row lines high
    COLPORT = 0x00;               // Disable pull-up on column lines
    ROWDIR = 0x00;                // Set row lines to input, already pulled up
    COLDIR = 0xFF;                // Set column lines to output, already driven low

    tempScan = 0;                 // Init temp scan code

    while( lineResult & 0x01 )          // Loop while column line high
    {
      lineResult >>= 1;           // Next column line into LSB
      tempScan++;                 // Increment column part of scancode
    }
    _delay_us(40);

    /* Find row of keypress */
    rowResult = ROWPIN;            // Get row lines

    COLPORT = 0xFF;               // Drive all column lines high
    ROWPORT = 0x00;               // Disable pull-up on row lines
    COLDIR = 0x00;                // Set column lines to input, already pulled up
    ROWDIR = 0xFF;                // Set row lines to output, alreay driven low

    if( rowResult != 0xFF )          // Any row lines low ?
    {
      while( rowResult & 0x01 )          // Loop while row line high
      {
        rowResult >>= 1;             // Next row line into LSB
        tempScan += 8;                // Increment row part of scancode
      }

      /* Process scancode */
      key_code.scan = tempScan;         // Save scancode
      key_code.updated = 1;           // Set update flag

      #ifdef ALTKEYS
      key_processAltKeys();           // Process alternation keys if implemented
      #endif

    }
    else
    {
      key_code.scan = 0xFF;           // Indicate no keys pressed
    }

  }
  else
  {
    key_code.scan = 0xFF;           // Indicate no keys pressed
  }

  return key_code.scan;
}

/*** Process alternation keys ***/
//*
//* This compares generated scancode to
//* predefined alternation codes and
//* updates flags accordingly.
//* Executed from timer overflow interrupt.
//*
#ifdef ALTKEYS
void key_processAltKeys()
{
  key_code.flags = key_altState | 0x80;   // Save alternation flags

  if     ( key_code.scan == ALTKEY0 ) key_altState ^= (1<<0);
  else if( key_code.scan == ALTKEY1 ) key_altState ^= (1<<1);
  else if( key_code.scan == ALTKEY2 ) key_altState ^= (1<<2);
  else if( key_code.scan == ALTKEY3 ) key_altState ^= (1<<3);
  else if( key_code.scan == LCKKEY0 ) key_altState ^= (1<<4);
  else if( key_code.scan == LCKKEY1 ) key_altState ^= (1<<5);
  else if( key_code.scan == LCKKEY2 ) key_altState ^= (1<<6);
  else                    // Not alternation key ?
  {
    key_altState &= ALTLOCKMASK;        // Clear one-shot key flags
  }
}
#endif

