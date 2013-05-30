AVR Needler
===========

Pneumatic CNC engraving machine.

1.) Text input via PS2 Keyboard on Atmega32 (CPU1). User feedback via HD44780 based LCD. 
2.) G-code generation from entered text with libhf2gcode.
3.) CPU1 sends g-code via UART to CPU2 with grbl (https://github.com/grbl/grbl)
4.) grbl generates the stepping sequences from the g-code

Used hardware: MKBoard Rev2.0 (Atmega32), Atmega328 for grbl, AMW 102 as stepper amp.

Links
-----

*  https://github.com/Andy1978/hf2gcode
*  http://wiki.fablab-muenchen.de/display/WIKI/Programme+die+G-Code+erzeugen
*  https://github.com/grbl/grbl
