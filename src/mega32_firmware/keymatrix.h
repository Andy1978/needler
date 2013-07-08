//************************************************
//* Matrix keyboard decoder
//* Main driver header file
//*
//* Filename: keymatrix.h
//* Version:  1.0
//*
//************************************************
#ifndef KEYMATRIX_H
#define KEYMATRIX_H

union _key_code
{
  //*
  //* Examples on use of this scan result structure
  //*
  //* key_code.complete - Get complete 16 bit result
  //* key_code.flags    - Get flags only, include update flag
  //* key_code.altKey1  - Get flag for alternation (one-shot) key 1
  //* key_code.lckKey2  - Get flag for lock key 2
  //* key_code.updated  - Get update flag
  //* key_code.scan   - Get scancode only
  //* key_code.row    - Get row part of scancode
  //* key_code.col    - Get column part of scancode
  //*

  unsigned int complete;            // Access all 16 bits

  struct
  {
    union
    {
      unsigned char flags;        // Access flag status only

      struct
      {
        unsigned char altKey0 : 1;    // Access the flags separately
        unsigned char altKey1 : 1;
        unsigned char altKey2 : 1;
        unsigned char altKey3 : 1;
        unsigned char lckKey0 : 1;
        unsigned char lckKey1 : 1;
        unsigned char lckKey2 : 1;

        unsigned char updated : 1;    // Access updateflag
      };
    };

    union
    {
      unsigned char scan;         // Access scancode

      struct
      {
        unsigned char col : 3;      // Access column of keypress
        unsigned char row : 3;      // Access row of keypress

        unsigned char unused : 2;
      };
    };
  };
};



extern volatile union _key_code key_code;   // Scan result structure
extern volatile unsigned char key_altState;   // Current alternation flags

void key_init(void);                // Driver initialization
unsigned char key_get(void);

#endif
