//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
#include <libpic30.h>

#include <p33Fxxxx.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "uart.h"
#include "crc16.h"
#include "lab3.h"
#include "lcd.h"
#include "timer.h"

//macros for ack and nack
#define MSG_ACK 1
#define MSG_NACK 0

bool msg_done = false;

// Primary (XT, HS, EC) Oscillator without PLL
_FOSCSEL(FNOSC_PRIPLL);
// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystanl
_FOSC(OSCIOFNC_ON & POSCMD_XT);
// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);
// Disable Code Protection
_FGS(GCP_OFF);   


//timer 1 interrupt
void __attribute__((__interrupt__)) _T1Interrupt(void)
{
    // Timer 1 ISR
    msg_done = true;
    T1CONbits.TON = 0;
    TMR1 = 0x00;
    CLEARBIT(IFS0bits.T1IF);
}

int main(void)
{	
	/* Q: What is my purpose? */
	/* A: You pass butter. */
	/* Q: Oh. My. God. */
    // :(
    
    // Variable initialisation 
    uint8_t databyte = 0; // Contains the data being read
    int recret = 0; // holds return value of uart2_recv
    int attempts = 0; // Number of failed attempts 
    int start = 0; // Keeping track of where we are in the payload
    int i;
    bool msgwrong = false;
    bool msgtoprocess = false;
    uint16_t crcclient = 0; // Reiteratively computing CRC in their weirdass method.
    uint8_t crcserver1 = 0, crcserver2 = 0; // CRC value that we get from  server
    uint16_t fullcrcserver;     //combination of crcserver1 and 2
    uint8_t msglength = 0; // Msg length we get from client.
    char buf[256]; // Compile the message together, what we receive from payload from server.
    int cntbuf = 0; //Keeping track of count of length data we're reading in
    for (i = 0; i<256; i++) {
        buf[i] = 0;
    }
    
    // initialize UART
    uart2_init(9600);
    
    // initialize LCD
    __C30_UART=1;	
    lcd_initialize();
	lcd_clear();
    lcd_locate(0,0);
    lcd_printf("Lab 3");
    lcd_locate(0,1);
    lcd_printf("Attempts: 0 \n");
    lcd_locate(0,2);
    lcd_printf("Message: \n");
    
    // Timer1 for every 1 sec
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
    //T1CONbits.TON = 1;// Start Timer
    
    while(1) {
        start = 0;  //indicates the section of the message being read
        databyte = 0;   //holds the byte being read
        crcclient = 0;
        cntbuf = 0;
        msglength = 0;
        recret = 0;
        crcserver1 =  0;
        crcserver2 = 0;
        fullcrcserver = 1;
        msgwrong = false;   //if the message must be wrong
        msg_done = false;   //if there is a complete message
        msgtoprocess = false;   //if there is any message at all
        for (i = 0; i<256; i++) {
            buf[i] = 0;
        }

        while(!msg_done) {
            
            //while the message is not complete, read the next byte
            recret = uart2_recv(&databyte);
            
            //reread if nothing was in buffer
            if (recret != 0) {
                continue;
            }
            
            TMR1 = 0x00; //Clear timer register
            T1CONbits.TON = 1;  //start timer
            //conditions for starting 0 byte
            if (start == 0 && databyte != 0) {
                msgwrong = true;
                msgtoprocess = true;
            } else if (start == 0 && databyte == 0x0) {
                start = 1;
                msgtoprocess = true;
            //conditions for reading crc bytes
            } else if (start == 1)  { //first CRC byte
                start = 2;
                crcserver1 = databyte;
            } else if (start == 2) { //second CRC byte
                start = 3;
                crcserver2 = databyte;
            //condition for reading length
            } else if (start == 3) { // length byte
                msglength = databyte;
                start = 4;
            //condition for reading message bytes
            } else if (start==4) { //starting msg read.
                buf[cntbuf++]= (char) databyte;
                // Compute CRC byte by byte. 
                crcclient = crc_update(crcclient, databyte);
                if (cntbuf >= msglength) { //end of msg reached. We keep going if we keep reading in overflows.
                    msg_done = true; //done reading stuff.
                }
                else if(databyte == 0){
                    msg_done = true;
                    msgwrong = true; 
                }
            }
            T2CONbits.TON = 0;
        }
        if (msgtoprocess) {
            if (!msgwrong) {
                // Process the message you have. 
                //combine the two halves of the received crc to create one 16 bit crc
                fullcrcserver = (( (uint16_t) crcserver1) << 8) | (crcserver2);
                //if the crcs do not match, the message has failed to properly send. Reply with NACK
                if(fullcrcserver - crcclient != 0) {
                    uart2_send_8(MSG_NACK);
                    lcd_locate(0,1);
                    lcd_printf("Attempts:        ");
                    lcd_locate(0,1);
                    lcd_printf("Attempts: %d", ++attempts);
                } else if (fullcrcserver - crcclient == 0) {   //else send ACK
                    uart2_send_8(MSG_ACK);
                    lcd_locate(0,1);
                    lcd_printf("Attempts:        ");
                    lcd_locate(0,1);
                    lcd_printf("Attempts: %d", ++attempts);
                    lcd_locate(0,3);
                    lcd_printf("                    ");
                    lcd_locate(0,4);
                    lcd_printf("                    ");
                    lcd_locate(0,5);
                    lcd_printf("                    ");
                    lcd_locate(0,3);
                    for (i=0; i<msglength; i++) {
                        lcd_printf("%c", buf[i]);
                    }
                    lcd_locate(0,5);
                    lcd_printf("%x", fullcrcserver);
                    lcd_locate(0,6);
                    lcd_printf("%x", crcclient);
                    
                    attempts = 0;
                }

            } else {    //if the message is already known to be wrong, do not check crc. Just send NACK.
                uart2_send_8(MSG_NACK);
                lcd_locate(0,1);
                lcd_printf("Attempts:        ");
                lcd_locate(0,1);
                lcd_printf("Attempts: %d", ++attempts);
            }
        }
    }
}	

