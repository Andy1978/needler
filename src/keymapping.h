#ifndef _KEYMAPPING_H_
#define _KEYMAPPING_H_

extern const char characters[64];

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

union _modifier_state
{
  unsigned char complete;
  struct
  {
    unsigned char SHIFT : 1;
    unsigned char ALT   : 1;
    unsigned char STRG  : 1;
    unsigned char CAPS  : 1;
    unsigned char OVERWRITE  : 1;
    unsigned char un2  : 1;
    unsigned char un3  : 1;
    unsigned char un4  : 1;
  };
};

#endif //_KEYMAPPING_H_
