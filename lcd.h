/*
 * File:   lcd.h
 * Author: X220
 *
 * Created on 2018年1月3日, 下午 7:31
 */

#ifndef LCD_H
#define	LCD_H
#ifdef	__cplusplus
extern "C" {
#endif

#include <pic18.h>
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 1000000UL
#endif

#include <stdio.h>

#define _busy_pin  TRISDbits.RD7
#define _rs_pin    TRISAbits.RA0
#define _rw_pin    TRISAbits.RA1
#define _en_pin    TRISAbits.RA2
#define _data_pins TRISD // 8bit mode

#define _busy_pin_read PORTDbits.RD7 // HIGH means not ready for next command
#define _rs_pin_write  LATAbits.LA0  // LOW: command.  HIGH: character.
#define _rw_pin_write  LATAbits.LA1  // LOW: write to LCD.  HIGH: read from LCD.
#define _en_pin_write  LATAbits.LA2  // activated by a HIGH pulse.
#define _data_pins_write LATD // output
#define _data_pins_read PORTD // input

void oled_init(void);
void oled_clear_display(void);
void oled_write_upper_line(unsigned char *string);
void oled_write_lower_line(unsigned char *string);
void oled_write_data(unsigned char value);
void oled_set_DDRAM(unsigned char x, unsigned char y);
void oled_write_8bits(unsigned char value);
void oled_check_busy(void); // polling busy flag
void oled_write_command(unsigned char value);

#ifdef	__cplusplus
}
#endif
#endif	/* LCD_H */