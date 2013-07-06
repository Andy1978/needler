#ifndef _KEYMAPPING_H_
#define _KEYMAPPING_H_

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

#endif //_KEYMAPPING_H_
