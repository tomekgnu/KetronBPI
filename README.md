# KetronBPI
1. Bananapi:
  - uart0: console
  - uart7: MIDI
  - SPI
  - CS0:  chip select for spimega (Arduino)
  - CS1:  chip select for LCD
  - PH2 (IO-1): interrupt from Arduino (falling edge)
  - PH20 (IO-4): lcd reset
  - PH21 (IO-5): lcd DC
  
2. Arduino:
  - PD2, PD3, PD4: buttons on Midi shield
  - PD5: joystick press
  - PC5: button on Screw Shield
  - ADC0, ADC1: pots on Midi shield
  - ADC2, ADC3: joystick X/Y axes
  - PD6: interrupt signal for BananaPi
