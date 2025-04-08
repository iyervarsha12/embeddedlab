#include <p33Fxxxx.h>
//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
#include <libpic30.h>
#include <uart.h>
#include "lcd.h"
#include "led.h"
#include "types.h"
#include <stdbool.h>
/* Initial configuration by EE */
// Primary (XT, HS, EC) Oscillator with PLL
_FOSCSEL(FNOSC_PRIPLL);

// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystal
_FOSC(OSCIOFNC_OFF & POSCMD_XT); 

// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);

// Disable Code Protection
_FGS(GCP_OFF);  

int main(){
	/* Initialization Sequence */
    int i = 0;
    bool unpressed_fire = FALSE;
    bool unpressed_thumb = FALSE;
    int countbutton1 = 0;
	__C30_UART=1;	
	lcd_initialize();
	lcd_clear();
    led_initialize();
    lcd_locate(0,0);
    
    int checkCount;
    int counter_fire;
    int counter_thumb;
    bool presscheck_fire;
    bool presscheck_thumb;
    
    int numflashes = 0;
    
//  blinking LED4
    for(i = 0; i<3;i++) {
        SETLED(LED4_PORT);
        __delay_ms(1000);
        CLEARLED(LED4_PORT);
        __delay_ms(1000);
    }
    
    // Joystick initialisation 
    AD1PCFGHbits.PCFG20 = 1;
    TRISEbits.TRISE8 = 1;
    TRISDbits.TRISD10 = 1;
    
    //Display initial count
    lcd_clear();
	lcd_locate(0,0);
	lcd_printf("Varsha\n");
    lcd_locate(0,1);
    lcd_printf("Noah\n");
    lcd_locate(0,2);
    lcd_printf("Paul\n");
    lcd_locate(0,3);
    lcd_printf("number(hex) = 0x0");
    lcd_locate(0,4);
    lcd_printf("number(dec) = 0");

    
	while(1){
        checkCount = 10;
        counter_fire = 0;
        counter_thumb = 0;
        presscheck_fire = false;
        presscheck_thumb = false;
        for (i = 0; i < checkCount; i++) {
            counter_fire += PORTEbits.RE8;
            counter_thumb += PORTDbits.RD10;
            __delay_ms(5);
        }
        if(counter_fire == 0){
            presscheck_fire = true;
        } else if (counter_fire == checkCount){
            presscheck_fire = false;
        }
        if (counter_thumb == 0) {
            presscheck_thumb = true;
        } else if (counter_thumb == checkCount) {
            presscheck_thumb = false;
        }
        
        // This counts for stickiness, not for debouncing.
		if (presscheck_fire && unpressed_fire) {
            countbutton1++;
            lcd_locate(0,3);
            lcd_printf("number(hex) = 0x%x", countbutton1);
            lcd_locate(0,4);
            lcd_printf("number(dec) = %d", countbutton1);
            unpressed_fire = FALSE;
        }
        if (!presscheck_fire) {
            unpressed_fire = TRUE;
        }
        if (presscheck_thumb && unpressed_thumb) {
            unpressed_thumb = FALSE;
            
        } else if (!presscheck_thumb) {
            unpressed_thumb = TRUE;
        }
        if (!unpressed_fire) {
            SETLED(LED1_PORT);            
        } else {
            CLEARLED(LED1_PORT);
        }
        if (!unpressed_thumb){
            SETLED(LED2_PORT);
        } else {
            CLEARLED(LED2_PORT);
        }
        if (unpressed_fire != unpressed_thumb) {
            SETLED(LED3_PORT);
        } else {
            CLEARLED(LED3_PORT);
        }
        
	}
    return 0;
}


