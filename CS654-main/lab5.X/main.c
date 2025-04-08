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
int checkCount;
int counter_fire;
bool presscheck_fire;
bool unpressed_fire = FALSE;
bool unpressed_thumb = FALSE;
int i = 0;
int countbutton1 = 0;

//initialize variables for checking and setting x/y
int x = 512;
int y = 512;
int x_max = 300;
int y_max = 300;
int x_min = 1023;
int y_min = 1023;
//initialize values to represent the range of pulses to send to the motor
double pulseMin = 0.9;
double pulseMax = 2.1;

//initialize variables to use in performing actions on trigger pull
int step = 0;
bool minAndMaxDone = false;

//function to calculate pulse length given the value range and current value
// for x or y on joystick
double positionToPulse(int min, int max, int pos)
{
    double ratio = (double)(pos-min) / (double)(max-min);
    double pulse = ratio * (pulseMax-pulseMin) + pulseMin;
    return pulse;
}


//function to handle trigger pulls. Named during a period of confusion and never
// renamed. Does more than check min and max.
void checkMinAndMax(){

    TOGGLELED(LED3_PORT);   //led toggled just for testing purposes
    if(step == 0){ //Does x max
        x_max = x;
        step = 1;
    } else if(step == 1){ //Does x min 
        x_min = x;
        step = 2;
    } else if(step == 2){ // Does y max
        y_max = y;
        step = 3;
    } else if(step == 3){ // Does y min
        y_min = y;
        step = 4;
    } else if(step == 4){ // Shift step to end loop for setting x motor
        step = 5;
    } else if(step == 5){ // Shift step to end loop for setting y motor
        step = 6;
    }
}

//function to sample joystick x position and write it to a global variable
void checkXPosition(void)
{
    AD2CHS0bits.CH0SA = 0x4;  //sample RB4 pin
    SETBIT(AD2CON1bits.SAMP);   //begin to sample
    while(!AD2CON1bits.DONE);   //wait until sampling is done
    CLEARBIT(AD2CON1bits.DONE);     //reset done flag
    x = ADC2BUF0;                   //assign x to the sampled value in the buffer
}

//Same thing as checkXPosition() but for the y direction on the joystick
void checkYPosition(void)
{
    AD2CHS0bits.CH0SA = 0x5;  //sample RB5 pin
    SETBIT(AD2CON1bits.SAMP);   
    while(!AD2CON1bits.DONE);
    CLEARBIT(AD2CON1bits.DONE);
    y = ADC2BUF0;
}

//function to check if the trigger is currently pulled. Uses debouncing code
// from a previous lab.
void checkClick(void)
{
    checkCount = 10;
    counter_fire = 0;
    presscheck_fire = false;
    //sample the button value several times over a short period and collect results
    for (i = 0; i < checkCount; i++) {
        counter_fire += PORTEbits.RE8;
        __delay_ms(5);
    }
    //if the results are all 0, the button is considered pressed and the flag is set
    if(counter_fire == 0){
        presscheck_fire = true;
    } else if (counter_fire == checkCount){
        presscheck_fire = false;
    }

    //if the pressed flag is set and the button is currently flagged as 
    // not-yet-pressed, call the function to handle button presses and then set
    // the latter flag to false.
    // (the second flag helps with stickiness; holding the button will not be
    //   counted as more than one press)
    if (presscheck_fire && unpressed_fire) {
        checkMinAndMax();
        unpressed_fire = FALSE;
    }
    if (!presscheck_fire) {
        unpressed_fire = TRUE;
    }
}

//function to handle the sequence of prompting the user to calibrate x max and
// min and registering the input values
void setXExtrema(void)
{
    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Position stick at max x and click trigger");
    //first loop to get max x
    while(1){
        checkXPosition();   //sample and set x position
        lcd_locate(0,2);
        lcd_printf("x position: %d       ", x)
        checkClick();   //check for button click
        if(step != 0){  //if the button was clicked; the step will advance and the loop will end.
            break;
        }
    }
    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Position stick at min x and click trigger");
    //repeat a similar loop for the x min
    while(1){
        checkXPosition();
        lcd_locate(0,2);
        lcd_printf("x position: %d       ", x)
        checkClick();
        if(step != 1){
            break;
        }
    }
}

//same as setXExtrema() but for y
void setYExtrema(void)
{   
    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Position stick at max y and click trigger");
    while(1){
        checkYPosition();
        lcd_locate(0,2);
        lcd_printf("y position: %d       ", y)
        checkClick();
        if(step != 2){
            break;
        }
    }
    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Position stick at min y and click trigger");
    while(1){
        checkYPosition();
        lcd_locate(0,2);
        lcd_printf("y position: %d       ", y)
        checkClick();
        if(step != 3){
            break;
        }
    }
}

//function to set the pulse width sent to the x motor based on current x position.
//Note that OC8RS is set to PR2 minus the desired pulse width. Since the output
// compare sends an inverted signal, we set it to produce the desired high pulse
// at the END of the 20 ms period. Combining that with the low pulse at the start
// of the following period, we can mimic the actual desired signal.
void setXMotor()
{
        checkXPosition();
        //the value of OC8RS is 200 times the desired pulse width in ms, so
        // we must multiply the calculated pulse width by 200.
        OC8RS = PR2 - 200*positionToPulse(x_min, x_max, x);
}

//function to set pulse width send to y motor based on y position.
//follows same logic as setXMotor()
void setYMotor()
{
        checkYPosition();
        OC7RS = PR2 - 200*positionToPulse(y_min, y_max, y);
}

//function for the sequence of prompting the user to set an x position for the
// board and freezing the board in that position when the button is pressed
setXBoard()
{
    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Click to freeze x pos.");
    //continue to wait for input as long as the step does not change.
    //since the motor is only set as long as this loop runs, once the button
    // is clicked and the loop condition ends the board will cease to be updated
    // and stay in place.
    while(step == 4){   
        setXMotor();    //set the x motor according to the current position
        lcd_locate(0,1);
        lcd_printf("Current pulse width: %d", OC8RS);
        checkClick();   //check if user clicked button
        __delay_ms(5);
    }
}

//Same as setXBoard() but for y position
setYBoard()
{
    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Click to freeze y pos.");
    while(step == 5){
        setYMotor();
        lcd_locate(0,1);
        lcd_printf("Current pulse width: %d", OC7RS);
        checkClick();
        __delay_ms(5);
    }
}

int main(){
	/* Initialization Sequence */
	__C30_UART=1;	
	lcd_initialize();
	//lcd_clear();
    led_initialize();
    lcd_locate(0,0);
    
    // Joystick button initialization 
    SETBIT(AD1PCFGHbits.PCFG20);    //setting AN20 pin to digital input
    TRISEbits.TRISE8 = 1;       //setting pin to input
    IEC1bits.INT1IE = 0;        //DISABLE interrupts. We don't use them to handle clicks this time.
    IPC5bits.INT1IP = 0x05;     //set interrupt priority
    IFS1bits.INT1IF = 0;        //clear interrupt flag
    INTCON2bits.INT1EP = 0;     //rising edge trigger
    
    //set up ADC
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

    //first calibrate joystick by setting and and y ranges
    setXExtrema();
    setYExtrema();    
    //then set the board positions
    setXBoard();
    setYBoard();

    lcd_clear();
    lcd_locate(0,0);
    lcd_printf("x min = %d", x_min);
    lcd_locate(0,1);
    lcd_printf("x max = %d", x_max);
    lcd_locate(0,2);
    lcd_printf("y min = %d", y_min);
    lcd_locate(0,3);
    lcd_printf("y max = %d", y_max);
    lcd_locate(0,4);
    lcd_printf("Current x pulse: %d", OC8RS);
    lcd_locate(0,5);
    lcd_printf("Current y pulse: %d", OC7RS);
    
    //while loop just to keep the program from ending
	while(1){
        __delay_ms(50);

	}
    return 0;
}


