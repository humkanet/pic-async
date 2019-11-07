#ifndef _CONFIG_H
#define _CONFIG_H


// PIC16F18326 Configuration Bit Settings
// CONFIG1
#pragma config FEXTOSC  = ECH     // FEXTOSC External Oscillator mode Selection bits (EC (external clock) above 8 MHz)
#pragma config RSTOSC   = EXT1X   // Power-up default value for COSC bits (EXTOSC operating per FEXTOSC bits)
#pragma config CLKOUTEN = OFF     // Clock Out Enable bit (CLKOUT function is disabled; I/O or oscillator function on OSC2)
#pragma config CSWEN    = ON      // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN    = ON      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)
// CONFIG2
#pragma config MCLRE    = OFF     // Master Clear Enable bit (MCLR/VPP pin function is digital input; MCLR internally disabled; Weak pull-up under control of port pin's WPU control bit.)
#pragma config PWRTE    = OFF     // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE     = OFF     // Watchdog Timer Enable bits (WDT disabled)
#pragma config LPBOREN  = OFF     // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN    = ON      // Brown-out Reset Enable bits (Brown-out Reset enabled, SBOREN bit ignored)
#pragma config BORV     = LOW     // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY  = OFF     // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be set and cleared repeatedly (subject to the unlock sequence))
#pragma config STVREN   = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a Reset)
#pragma config DEBUG    = OFF     // Debugger enable bit (Background debugger disabled)
// CONFIG3
#pragma config WRT      = ALL     // User NVM self-write protection bits (0000h to 3FFFh write protected, no addresses may be modified)
#pragma config LVP      = OFF     // Low Voltage Programming Enable bit (High Voltage on MCLR/VPP must be used for programming.)
// CONFIG4
#pragma config CP       = ON      // User NVM Program Memory Code Protection bit (User NVM code protection enabled)
#pragma config CPD      = ON      // Data NVM Memory Code Protection bit (Data NVM code protection enabled)


#endif /* _CONFIG_H */
