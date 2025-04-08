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

int timer2cnt = 0;
int timer1cnt = 0;
long long timeSinceReset = 0;

//timer 1 interrupt
void __attribute__((__interrupt__)) _T1Interrupt(void)
{
    TOGGLELED(LED2_PORT);
    //LED2_PORT = !LED2_PORT;
    timer1cnt++;

    CLEARBIT(IFS0bits.T1IF);
}

//Timer 2 ISR
void __attribute__((__interrupt__)) _T2Interrupt(void)
{
    timeSinceReset++;   //increment time since counter reset
    timer2cnt++;        //increment counter for blinking LED1
    if (timer2cnt == 5) {
        TOGGLELED(LED1_PORT);
        //LED1_PORT = !LED1_PORT;
        timer2cnt = 0;
    }
    
    IFS0bits.T2IF = 0;
}

//joystick interrupt
void __attribute__((__interrupt__)) _INT1Interrupt(void)
{
    
    timeSinceReset = 0;
        
    IFS1bits.INT1IF = 0;
}

int main(){
	/* Initialization Sequence */
	__C30_UART=1;	
	lcd_initialize();
	lcd_clear();
    led_initialize();
    lcd_locate(0,0);
    
    int loopCount = 0;  //the number of main loops that have run
    long long TSRFreeze = 0;  //stores time since reset when being displayed to avoid mid-display updates
    int minutes = 0;    //used to display minutes on time since reset
    int seconds = 0;    //used to display seconds on time since reset
    int mseconds = 0;   //used to display ms on time since reset
    
    // Joystick initialization 
    AD1PCFGHbits.PCFG20 = 1;    //setting to digital input
    TRISEbits.TRISE8 = 1;       //setting trigger button to give input
    IEC1bits.INT1IE = 1;        //enable external interrupts
    IPC5bits.INT1IP = 0x05;     //set interrupt priority
    IFS1bits.INT1IF = 0;        //clear interrupt flag
    INTCON2bits.INT1EP = 0;     //rising edge trigger
    
    //Initialize and start timer 1 for 1 sec. intervals
    __builtin_write_OSCCONL(OSCCONL | 2);
    T1CONbits.TON = 0; //Disable Timer
    T1CONbits.TCS = 1; //Select external clock
    T1CONbits.TSYNC = 0; //Disable Synchronization
    T1CONbits.TCKPS = 0b00; //Select 1:1 Prescaler
    TMR1 = 0x00; //Clear timer register
    PR1 = 32767; //Load the period value
    IPC0bits.T1IP = 0x07; // Set Timer1 Interrupt Priority Level
    IFS0bits.T1IF = 0; // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1;// Enable Timer1 interrupt
    T1CONbits.TON = 1;// Start Timer
    
    //Initialize and start timer 2 for 1 ms. intervals
    CLEARBIT(T2CONbits.TON); //Disable timer
    CLEARBIT(T2CONbits.TCS); //select internal instruction clock cycle
    CLEARBIT(T2CONbits.TGATE); //disable gated timer mode
    TMR2 = 0x00; //Clear timer register
    T2CONbits.TCKPS = 0b11; //Select 1:256 prescaler 
    PR2 = 50; //LOad period value 
    IPC1bits.T2IP = 0x06; //timer2 interrupt priority level
    CLEARBIT(IFS0bits.T2IF); //clear timer2 IF 
    SETBIT(IEC0bits.T2IE); //Enable timer2 interrupt 
    SETBIT(T2CONbits.TON); //start timer2
    
    //Initialize and start timer 3
    CLEARBIT(T3CONbits.TON); //Disable timer
    CLEARBIT(T3CONbits.TCS); //select internal instruction clock cycle
    CLEARBIT(T3CONbits.TGATE); //disable gated timer mode
    TMR3 = 0x00; //Clear timer register
    T3CONbits.TCKPS = 0b00; //Select 1:1 prescaler 
    PR3 = 0xffff; //Load period value 
    SETBIT(T3CONbits.TON); //start timer 3
    lcd_locate(0,0);
    lcd_printf("Time since reset:");
    lcd_locate(0, 4);
    lcd_printf("Main loop execution");
    lcd_locate(0, 5);
    lcd_printf("time:");
    
     
	while(1){
        TOGGLELED(LED4_PORT);   //toggle led 4 every iteration of this loop
        TMR3 = 0x0;     //clear timer 3 at the start of each loop iteration
        loopCount++;    //increment the loop counter, only display time at 25000 loops
        if (loopCount == 25000) {
            //convert ms counter into minutes, seconds, and milliseconds
            TSRFreeze = timeSinceReset; //store time here to avoid updates as it is read
            minutes = (TSRFreeze - (TSRFreeze % 60000)) / 60000;
            TSRFreeze -= (minutes*60000);
            seconds = (TSRFreeze - (TSRFreeze % 1000)) / 1000;
            TSRFreeze -= (seconds*1000);
            mseconds = TSRFreeze;
            lcd_locate(0,1);
            lcd_printf("%2d:%2d:%3d", minutes, seconds, mseconds)
            
            loopCount = 0;
            lcd_locate(0,6);
            lcd_printf("cycle(ms): %.4f\r", (float)TMR3/12800); //divide cycles by frequency * 1000 to get ms loop execution time
            lcd_locate(0,7);
            lcd_printf("cycles: %u\r", TMR3);
        }
	}
    return 0;
}


