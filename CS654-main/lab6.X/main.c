#include <p33Fxxxx.h>
//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
#include <libpic30.h>
#include <stdlib.h>
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


//initialize variables for debouncing trigger
int X_READ = 0; //110
int Y_READ = 1; // 001

//initialize values to represent the range of pulses to send to the motor
double pulseMin = 0.9;
double pulseMax = 2.1;


//function to set the pulse width sent to the x motor based on current x position.
//Note that OC8RS is set to PR2 minus the desired pulse width. Since the output
// compare sends an inverted signal, we set it to produce the desired high pulse
// at the END of the 20 ms period. Combining that with the low pulse at the start
// of the following period, we can mimic the actual desired signal.
void setXMotor(float x)
{
        //the value of OC8RS is 200 times the desired pulse width in ms, so
        // we must multiply the calculated pulse width by 200.
        OC8RS = PR2 - 200*x;
        //__delay_ms(50);
}

//function to set pulse width send to y motor based on y position.
//follows same logic as setXMotor()
void setYMotor(float y)
{
        OC7RS = PR2 - 200*y;
}

//  sets i/o pins to sample the x position and put it into X_READ
void readXBoard() 
{
    //set up the I/O pins E1, E2, E3 so that the touchscreen X-coordinate pin
    //connects to the ADC
    CLEARBIT(LATEbits.LATE1);
    SETBIT(LATEbits.LATE2);
    SETBIT(LATEbits.LATE3);
    AD1CHS0bits.CH0SA = 0xF;  //sample AN15
    __delay_ms(20);
    SETBIT(AD1CON1bits.SAMP);   //begin to sample
    while(!AD1CON1bits.DONE);   //wait until sampling is done
    X_READ = ADC1BUF0;      
    CLEARBIT(AD1CON1bits.DONE);     //reset done flag

}
//  sets i/o pins to sample the y position and put it into Y_READ
void readYBoard() 
{
    //set up the I/O pins E1, E2, E3 so that the touchscreen Y-coordinate pin
    //connects to the ADC
    SETBIT(LATEbits.LATE1);
    CLEARBIT(LATEbits.LATE2);
    CLEARBIT(LATEbits.LATE3);
    AD1CHS0bits.CH0SA = 0x9;  //sample AN9
    __delay_ms(20);
    SETBIT(AD1CON1bits.SAMP);   //begin to sample
    while(!AD1CON1bits.DONE);   //wait until sampling is done
    Y_READ = ADC1BUF0;  
    CLEARBIT(AD1CON1bits.DONE);     //reset done flag
    
}

// Sorts an int array
//@param nums: the array to sort
//@param n: size of the array
void sortinTime(int nums[], int n)
{
    int i = 0;
    int j = 0;
    int temp = 0;
    for(i=0; i < n-1; i++){
        for(j=0; j < n-i-1; j++){
            if(nums[j] > nums[j+1]){
                temp = nums[j];
                nums[j] = nums[j+1];
                nums[j+1] = temp;
            }
        }
    }
}
// sets the servo position to x and y, then samples the position values, get the median, and outputs it to the screen
//@param x: x position to set
//@param y: y position to set
//@param line: line of lcd to write to
void labThing(float x, float y, int line){
    int i = 0;
    int xsamples[5] = {0,0,0,0,0};
    int ysamples[5] = {0,0,0,0,0};
    //setting motor
    setXMotor(x);
    setYMotor(y);
    __delay_ms(3000);
    // sampling from the board
    for(i = 0; i < 5; i++){
        readXBoard();
        xsamples[i] = X_READ;
        readYBoard();
        ysamples[i] = Y_READ;
    }
    // finding median
    sortinTime(xsamples, 5);
    sortinTime(ysamples, 5);
    lcd_locate(0,line);
    lcd_printf("C%d pos.: %d. %d", line, xsamples[2], ysamples[2]);
}

int main(){
	/* Initialization Sequence */
	__C30_UART=1;	
	lcd_initialize();
	//lcd_clear();
    led_initialize();
    lcd_locate(0,0);
    
    //set up ADC 1
    CLEARBIT(AD1CON1bits.ADON);
    SETBIT(AD1CON1bits.AD12B); //set 12b Operation Mode
    AD1CON1bits.FORM = 0; //set integer output
    AD1CON1bits.SSRC = 0x7; //set automatic conversion
    //Configure AD1CON2
    AD1CON2 = 0; //not using scanning sampling
    //Configure AD1CON3
    CLEARBIT(AD1CON3bits.ADRC); //internal clock source
    AD1CON3bits.SAMC = 0x1F; //sample-to-conversion clock = 31Tad
    AD1CON3bits.ADCS = 0x2; //Tad = 3Tcy (Time cycles)
    //Leave AD1CON4 at its default value
    //enable ADC
    SETBIT(AD1CON1bits.ADON);
    
    //set up ADC 2
    CLEARBIT(AD2CON1bits.ADON); //disable ADC
    CLEARBIT(AD2CON1bits.AD12B);    //set 10 bit operation mode (as in manual example)
    AD2CON1bits.FORM = 0;       //set integer output
    AD2CON1bits.SSRC = 0x7;     //set auto conversion
    AD2CON2 = 0;                //disable scanning sampling
    CLEARBIT(AD2CON3bits.ADRC); //set internal clock source
    AD2CON3bits.SAMC = 0x1F;    //sample to conversion clock set as shown in manual
    AD2CON3bits.ADCS = 0x2;     //time cycles set as in manual
    //AD2CON3bits.ADCS4 = 0;    
    //AD2CON3bits.ADCS5 = 0;
    SETBIT(AD2CON1bits.ADON);   //turn on ADC
    
    //setup Timer 2
    CLEARBIT(T2CONbits.TON); // Disable Timer
    CLEARBIT(T2CONbits.TCS); // Select internal instruction cycle clock
    CLEARBIT(T2CONbits.TGATE); // Disable Gated Timer mode
    TMR2 = 0x00; // Clear timer register
    T2CONbits.TCKPS = 0b10; // Select 1:64 Prescaler
    CLEARBIT(IFS0bits.T2IF); // Clear Timer2 interrupt status flag
    CLEARBIT(IEC0bits.T2IE); // Disable Timer2 interrupt enable control bit
    PR2 = 4000; // Set timer period 20ms:
    
    CLEARBIT(TRISDbits.TRISD7); /* Set OC8 as output */
    OC8R = 3700; /* Set the initial duty cycle to 5ms*/
    OC8RS = 3700; /* Load OCRS: next pwm duty cycle */
    OC8CON = 0x0006; /* Set OC8: PWM, no fault check, Timer2 */
    
    CLEARBIT(TRISDbits.TRISD6); /* Set OC7 as output */
    OC7R = 3700; /* Set the initial duty cycle to 5ms*/
    OC7RS = 3700; /* Load OCRS: next pwm duty cycle */
    OC7CON = 0x0006; /* Set OC8: PWM, no fault check, Timer2 */
    
    SETBIT(T2CONbits.TON); // Enable Timer

    //set up the I/O pins E1, E2, E3 to be output pins
    CLEARBIT(TRISEbits.TRISE1); //I/O pin set to output
    CLEARBIT(TRISEbits.TRISE2); //I/O pin set to output
    CLEARBIT(TRISEbits.TRISE3); //I/O pin set to output


    //while loop just to keep the program from ending
	while(1){
        lcd_clear();
        labThing(2.1, 2.1, 0);
        labThing(2.1, .9, 1);
        labThing(.9, .9, 2);
        labThing(.9, 2.1, 3);
        __delay_ms(5000);
	}
    return 0;
}


