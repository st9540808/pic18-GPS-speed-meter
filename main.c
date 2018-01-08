/*
 * File:   main.c
 * Author: X220
 *
 * Created on 2017/12/31, 下午 11:57
 */
#pragma config OSC = INTIO67      // Oscillator Selection bits (HS oscillator)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 3         // Brown Out Reset Voltage bits (Minimum setting)

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 1        // Watchdog Timer Postscale Select bits (1:1)

// CONFIG3H
#pragma config CCP2MX = PORTC   // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#pragma config PBADEN = OFF     // PORTB A/D Enable bit (PORTB<4:0> pins are configured as analog input channels on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = OFF      // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) not code-protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) not code-protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) not protected from table reads executed in other blocks)


#include <xc.h>
#include <pic18f4520.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "gps.h"

#ifndef _XTAL_FREQ
#define _XTAL_FREQ 1000000UL
#endif
#define NUM_OF_MODES 5

const char header_GPRMC[] = "GPRMC";
const char header_GPGGA[] = "GPGGA";
const unsigned char speed_jp[] = {
//  su    pi         -     to    "
    0xbd, 0xcb, 0xdf, 0xb0, 0xc4, 0xde, '\0'
};
const unsigned char distance_jp[] = {
    0xb7, 0xae, 0xd8, '\0'
};

volatile uint8_t mode = 0;
char buf[2][120];
unsigned char oled_buf[40];
volatile bool both_sentence_ready = false;
float distance = 0.0;
extern char lat, lon, mag;

void low_priority interrupt externInterrupt0(void)
{
    if (INTCON3bits.INT1IE && INTCON3bits.INT1IF) {
        INTCON3bits.INT1IE = 0;
        INTCON3bits.INT1IF = 0;
        TMR1 = 54597; // around 0.35 sec
        PIE1bits.TMR1IE = 1;
        PIR1bits.TMR1IF = 0;
        T1CONbits.TMR1ON = 1;
        mode = (mode + 1) % NUM_OF_MODES;
    } else if (PIE1bits.TMR1IE && PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;
        PIE1bits.TMR1IE = 0;
        T1CONbits.TMR1ON = 0;
        INTCON3bits.INT1IE = 1;
        INTCON3bits.INT1IF = 0;
    }
}

void nmea_read(void)
{
    char c = RCREG;
    static bool sentence_start = 0;
    static unsigned char line_idx = 0; // to select which line of buf to write
    static unsigned char idx = 0;

    if (c == '$') {
        sentence_start = 1;
        idx = 0;
    }

    if (!sentence_start) return;
    if (c == '\n' && sentence_start) {
        sentence_start = 0;
        buf[line_idx][idx++] = c;
        buf[line_idx][idx] = '\0';

        if (strncmp(header_GPRMC, &buf[line_idx][1], 5) == 0)
            line_idx++;
        else if (strncmp(header_GPGGA, &buf[line_idx][1], 5) == 0)
            line_idx++;

        if (line_idx == 2) {
            PIE1bits.RCIE = 0;
            RCSTAbits.CREN = 0;
            line_idx = 0;
            both_sentence_ready = 1;
        }
        return;
    }

    buf[line_idx][idx++] = c;
}

void high_priority interrupt uartInterrupt(void)
{
    if (PIE1bits.RCIE && PIR1bits.RCIF) {
        if (RCSTAbits.OERR == 1) {
            RCSTAbits.OERR = 0;
        }
        nmea_read();
        PIR1bits.RCIF = 0;
    }
}

void uart_init(void)
{
    TRISCbits.TRISC6 = 1;  // Setting by data sheet
    TRISCbits.TRISC7 = 1;
    OSCCONbits.IRCF = 4;   // 1MHz
    BAUDCONbits.BRG16 = 1; // Read Baud rate table
    TXSTAbits.BRGH = 1;
    SPBRG = 25;

    // Serial enable
    TXSTAbits.SYNC = 0;    // choose the async moode
    RCSTAbits.SPEN = 1;    // open serial port

    //setting TX/RX
    PIR1bits.TXIF = 0;
    PIR1bits.RCIF = 0;
    TXSTAbits.TXEN = 0;    // Disable Tx
    RCSTAbits.CREN = 1;    // Enable Rx
    //setting TX/RX interrupt
    PIE1bits.TXIE = 0;     // Tx interrupt
    IPR1bits.TXIP = 0;     // Setting Tx as high/low priority interrupt
    PIE1bits.RCIE = 1;     // Rx interrupt
    IPR1bits.RCIP = 1;     // Setting Rc as high/low priority interrupt
}

void timer1_extInt1_init(void)
{
    IPR1bits.TMR1IP = 0;
    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 0;

    T1CONbits.RD16 = 1; // 16 bit timer1
    T1CONbits.T1CKPS = 3; // 1:8 prescale

    INTCON3bits.INT1IP = 0;
    INTCON3bits.INT1IF = 0;
    INTCON2bits.INTEDG1 = 1; // rising edge
    INTCON3bits.INT1IE = 1;
}


void main(void)
{
    const unsigned char starbrust[] = {
        //  su    ta    -     ha    "     -     su    to   .
        0xbd, 0xc0, 0xb0, 0xca, 0xde, 0xb0, 0xbd, 0xc4, 0xa5, '\0'
    };
    const unsigned char stream[] = {
        //  su    to    ri    -     mu
        0xbd, 0xc4, 0xd8, 0xb0, 0xd1, '\0'
    };

    __delay_ms(400);

    oled_init();
    oled_write_upper_line(starbrust);
    oled_write_lower_line(stream);
    GPS_common_init();

    timer1_extInt1_init();
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;
    RCONbits.IPEN = 1;

    uart_init();

    for (;;) {
        if (both_sentence_ready) {
            boolean isOK_1 = GPS_parse(buf[0]);
            boolean isOK_2 = GPS_parse(buf[1]);

            if (isOK_1 && isOK_2) {
                GPS_SIGNAL_INFO info = GPS_getSignalInfo();

                if (info.fix) {
                    GPS_DATE_INFO date = GPS_getDateInfo();
                    GPS_LOCATION_INFO loc = GPS_getLocationInfo();

                    switch (mode) {
                    case 0:
                        oled_write_upper_line(speed_jp);
                        sprintf(oled_buf, "%7.2f km/h", loc.speed * 1.852f);
                        oled_write_lower_line(oled_buf);
                        break;
                    case 1:
                        oled_write_upper_line(distance_jp);
                        sprintf(oled_buf, "%10.0f m", distance);
                        oled_write_lower_line(oled_buf);
                        break;
                    case 2:
                        sprintf(oled_buf, "HDOP %7.2f", loc.HDOP);
                        oled_write_upper_line(oled_buf);
                        sprintf(oled_buf, "satellite %2d", info.satellites);
                        oled_write_lower_line(oled_buf);
                        break;
                    case 3: {
                        int lat_deg, lat_min, lat_sec;
                        float lat_min_frac;
                        int lon_deg, lon_min, lon_sec;
                        float lon_min_frac;

                        lat_deg = ((int) loc.latitude) / 100;
                        lat_min = ((int) loc.latitude) % 100;
                        lat_min_frac = fmod(loc.latitude, 1.0);
                        lat_sec = (int) (60.0f * lat_min_frac);

                        lon_deg = ((int) loc.longitude) / 100;
                        lon_min = ((int) loc.longitude) % 100;
                        lon_min_frac = fmod(loc.longitude, 1.0);
                        lon_sec = (int) (60.0f * lon_min_frac);

                        sprintf(oled_buf, "  %2d %2d'%2d\"%c", lat_deg, lat_min, lat_sec, lat);
                        oled_buf[4] = 0xdf;
                        oled_write_upper_line(oled_buf);
                        sprintf(oled_buf, " %3d %2d'%2d\"%c", lon_deg, lon_min, lon_sec, lon);
                        oled_buf[4] = 0xdf;
                        oled_write_lower_line(oled_buf);
                        break;
                    }
                    case 4:
                        sprintf(oled_buf, "  20%d/%d/%d", date.year, date.month, date.day);
                        oled_write_upper_line(oled_buf);
                        sprintf(oled_buf, "    %2d:%02d", date.hour, date.minute);
                        oled_write_lower_line(oled_buf);
                        break;
                    } // switch (mode)

                    if (loc.HDOP < 2.1 && info.satellites >= 6) {
                        if (loc.speed > 1.079913)  // around 2 km/h
                            distance += loc.speed * 0.5144445f;
                    }
                } else {
                    oled_write_upper_line(starbrust);
                    oled_write_lower_line(stream);
                }
            }

            both_sentence_ready = 0;
            RCSTAbits.CREN = 1;
            PIE1bits.RCIE = 1;
        }
    }
}