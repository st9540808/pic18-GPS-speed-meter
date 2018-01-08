#include <pic18f4520.h>

#include "lcd.h"

void oled_init(void)
{
    _rs_pin = 0;
    _rw_pin = 0;
    _en_pin = 0;

    oled_write_command(0x38);
    oled_write_command(0x08);
    oled_write_command(0x01);
    oled_write_command(0x0c);
    oled_write_command(0x06);
    oled_write_command(0x02);
}

void oled_clear_display(void)
{
    _en_pin = 0;
    oled_write_command(0x01);
}

void oled_write_upper_line(unsigned char *string)
{
    // using snprintf() with string
    unsigned char i = 0;
    for (; i < 12 && string[i] != NULL; i++) {
        oled_set_DDRAM(i, 0);
        oled_write_data(string[i]);
    }
    for (; i < 12; i++) {
        oled_set_DDRAM(i, 0);
        oled_write_data(' ');
    }
    oled_set_DDRAM(0, 0);
}

void oled_write_lower_line(unsigned char *string)
{
    unsigned char i = 0;
    for (; i < 12 && string[i] != NULL; i++) {
        oled_set_DDRAM(i, 1);
        oled_write_data(string[i]);
    }
    for (; i < 12; i++) {
        oled_set_DDRAM(i, 1);
        oled_write_data(' ');
    }
    oled_set_DDRAM(0, 0);
}


void oled_write_data(unsigned char value)
{
    _en_pin_write = 0;
    _rs_pin_write = 1;
    _rw_pin_write = 0;
    oled_write_8bits(value);
    oled_check_busy();
}

void oled_set_DDRAM(unsigned char x, unsigned char y)
{
    oled_write_command(0x80 | (y << 6) | x);
}

void oled_write_command(unsigned char value)
{
    _en_pin_write = 0;
    _rs_pin_write = 0;
    _rw_pin_write = 0;
    oled_write_8bits(value);
    oled_check_busy();
}

void oled_write_8bits(unsigned char value)
{
    _data_pins = 0x00; //set output
    _data_pins_write = value;

    _en_pin_write = 0;
    __delay_us(50);
    _en_pin_write = 1;
}

void oled_check_busy(void)
{
    unsigned char busy = 1;
    _busy_pin = 1; // make busy pin input
    _rs_pin_write = 0;
    _rw_pin_write = 1;

    do {
        _en_pin_write = 0;
        _en_pin_write = 1;

        __delay_us(10);
        busy = _busy_pin_read;

        _en_pin_write = 0;
    } while(busy);

    _busy_pin = 0; // make busy pin output
    _rw_pin_write = 0;
}