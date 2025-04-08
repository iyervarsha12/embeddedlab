/****************************************************/
/*                                                  */
/*   CS-454/654 Embedded Systems Development        */
/*   Instructor: Renato Mancuso <rmancuso@bu.edu>   */
/*   Boston University                              */
/*                                                  */
/*   Description: template file for digital and     */
/*                analog square wave generation     */
/*                via the LabJack U3-LV USB DAQ     */
/*                                                  */
/****************************************************/

#include "u3.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <string.h>

double MINV = 0;
double MAXV = 0;
int SENDHIGH = 1;

/* This function should initialize the DAQ and return a device
 * handle. The function takes as a parameter the calibratrion info to
 * be filled up with what obtained from the device. */
HANDLE init_DAQ(u3CalibrationInfo * caliInfo)
{
	HANDLE hDevice;
	int localID;
	
	/* Invoke openUSBConnection function here */
	hDevice = openUSBConnection(-1);
	
	/* Invoke getCalibrationInfo function here */
	while(getCalibrationInfo(hDevice, caliInfo) != 0){}
	
	return hDevice;
}
//==========================Analog Wave==============================================
int analog() {
  u3CalibrationInfo caliInfo;
  HANDLE hDevice;
  char str[255]; 
  hDevice = init_DAQ(&caliInfo);
	/* Invoke init_DAQ and handle errors if needed */
	

	/* Provide prompt to the user for a voltage range between 0
	 * and 5 V. Require a new set of inputs if an invalid range
	 * has been entered. */

  float voltageMin;
  float voltageMax;
  printf("Enter a minimum voltage between 0V and 5V:\n");
  while(scanf("%f", &voltageMin) <= 0 || voltageMin < 0 || voltageMin > 5){
    printf("No, ya goof. Just enter a number between 0 and 5.\n");
  }
  MINV = (double) voltageMin;
  printf("Enter a maximum  voltage between %f and 5V:\n", voltageMin);
  while(scanf("%f", &voltageMax) <= 0 || voltageMax <= voltageMin || voltageMax > 5){
    printf("No, ya goof. Just enter a number between %f and 5.\n", voltageMin);
  }
  MAXV = (double) voltageMax;
  
	
	/* Compute the maximum resolutiuon of the CLOCK_REALTIME
	 * system clock and output the theoretical maximum frequency
	 * for a square wave */
 printf("max: %f", MAXV);
  struct timespec resspec;
  if (clock_getres(CLOCK_REALTIME, &resspec) == -1) {
    printf("Failure to get resolution\n");
    return 0;
  }
  printf("check0\n");
  //time_t res = resspec.tv_sec;
  printf(" %10jd.%03ld (\n",  (intmax_t) resspec.tv_sec, resspec.tv_nsec / 1000000);
  printf("check1\n");
  float freq_max = 1000000000 / resspec.tv_nsec / 2;
  long long int frqmax = (long long int) freq_max;
  printf("Maximum frequency for this machine: %lld\n", frqmax);
	
	/* Provide prompt to the user to input a desired square wave
	 * frequency in Hz. */
  long long int frequency;
  printf("Enter a frequency lower than %lld and greater than 0:\n", frqmax);
  while(scanf("%lld", &frequency) <= 0 || frequency > frqmax || frequency <= 0){
    printf("No, ya goof, you entered %lld. Just enter a number less than %lld and greater than 0.\n", frequency, frqmax);
  }

	/* Program a timer to deliver a SIGRTMIN signal, and the
	 * corresponding signal handler to output a square wave on
	 * BOTH digital output pin FIO2 and analog pin DAC0. */

  timer_t timer1;
  struct sigevent timer1_event;
  memset(&timer1_event, 0, sizeof(timer1_event));
  timer1_event.sigev_notify = SIGEV_SIGNAL;
  timer1_event.sigev_signo = SIGRTMIN;
  
  if(timer_create(CLOCK_REALTIME, &timer1_event, &timer1) != 0){
    perror("no timer this time");
    exit(1);
  }

  long long int nSecs = 1000000000/(2*frequency);
  long long int totSecs = nSecs / 1000000000;
  long long int totNSecs = nSecs % 1000000000;
  printf("Times = %lld, %lld, %lld \n", nSecs, totSecs, totNSecs);
  struct itimerspec timer1_time;
  timer1_time.it_value.tv_sec = 2;
  timer1_time.it_value.tv_nsec = 0;
  timer1_time.it_interval.tv_sec = totSecs;
  timer1_time.it_interval.tv_nsec = totNSecs;
  printf("Timer1: %ld, %ld, %ld, %ld \n", timer1_time.it_value.tv_sec, timer1_time.it_value.tv_nsec, timer1_time.it_interval.tv_sec, timer1_time.it_interval.tv_nsec);

  if(timer_settime(timer1, 0, &timer1_time, NULL) != 0){
    perror("No timer for you");
    exit(1);
  }

  void send_wave(int signo){
    printf("Wave time\n");
    if(SENDHIGH == 1){
      printf("Sendhigh\n");
      SENDHIGH = 0;
      if(eDAC(hDevice, &caliInfo, 1, 0, MAXV, 0, 0, 0) < 0){
	printf("The signal failed to send. Dingus.\n");
      }
    } else {
      printf("Sendlow\n");
      SENDHIGH = 1;
      if(eDAC(hDevice, &caliInfo, 1, 0, MINV, 0, 0, 0) < 0){
	printf("The signal failed to send. Dingus.\n");
      }
    }
    printf("End wave time\n");
    
  }
  
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  action.sa_sigaction = (void *) send_wave;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN, &action, NULL);
  sigset_t satemp;
  sigemptyset(&satemp);
  sigaddset(&satemp, SIGRTMIN);
  
	/* The square wave generated on the DAC0 analog pin should
	 * have the voltage range specified by the user in the step
	 * above. */
  printf("Enter 'exit' to end the program.\n");
  while (1){
    int i = 0;
    gets(str);
    //printf(str);
    //printf("%d\n", strcmp(str,"exit"));
    if (strcmp(str,"exit")==0) {
    	//printf("EXIT TIME\n");
      	closeUSBConnection(hDevice);
      	return EXIT_SUCCESS;
    }
  }
	/* Display a prompt to the user such that if the "exit"
	 * command is typed, the USB DAQ is released and the program
	 * is terminated. */
 return EXIT_SUCCESS;
}

//==================================Digital Wave=========================================================
int digital() {
  u3CalibrationInfo caliInfo;
  HANDLE hDevice;
  char str[255]; 
  hDevice = init_DAQ(&caliInfo);
	/* Invoke init_DAQ and handle errors if needed */
	

	/* Provide prompt to the user for a voltage range between 0
	 * and 5 V. Require a new set of inputs if an invalid range
	 * has been entered. */
  
	
	/* Compute the maximum resolutiuon of the CLOCK_REALTIME
	 * system clock and output the theoretical maximum frequency
	 * for a square wave */
  struct timespec resspec;
  if (clock_getres(CLOCK_REALTIME, &resspec) == -1) {
    printf("Failure to get resolution\n");
    return 0;
  }
  printf("check0\n");
  //time_t res = resspec.tv_sec;
  printf(" %10jd.%03ld (\n",  (intmax_t) resspec.tv_sec, resspec.tv_nsec / 1000000);
  printf("check1\n");
  float freq_max = 1000000000 / resspec.tv_nsec / 2;
  long long int frqmax = (long long int) freq_max;
  printf("Maximum frequency for this machine: %lld\n", frqmax);
	
	/* Provide prompt to the user to input a desired square wave
	 * frequency in Hz. */
  long long int frequency;
  printf("Enter a frequency lower than %lld and greater than 0:\n", frqmax);
  while(scanf("%lld", &frequency) <= 0 || frequency > frqmax || frequency <= 0){
    printf("No, ya goof, you entered %lld. Just enter a number less than %lld and greater than 0.\n", frequency, frqmax);
  }

	/* Program a timer to deliver a SIGRTMIN signal, and the
	 * corresponding signal handler to output a square wave on
	 * BOTH digital output pin FIO2 and analog pin DAC0. */

  timer_t timer1;
  struct sigevent timer1_event;
  memset(&timer1_event, 0, sizeof(timer1_event));
  timer1_event.sigev_notify = SIGEV_SIGNAL;
  timer1_event.sigev_signo = SIGRTMIN;
  
  if(timer_create(CLOCK_REALTIME, &timer1_event, &timer1) != 0){
    perror("no timer this time");
    exit(1);
  }

  long long int nSecs = 1000000000/(2*frequency);
  long long int totSecs = nSecs / 1000000000;
  long long int totNSecs = nSecs % 1000000000;
  printf("Times = %lld, %lld, %lld \n", nSecs, totSecs, totNSecs);
  struct itimerspec timer1_time;
  timer1_time.it_value.tv_sec = 2;
  timer1_time.it_value.tv_nsec = 0;
  timer1_time.it_interval.tv_sec = totSecs;
  timer1_time.it_interval.tv_nsec = totNSecs;
  printf("Timer1: %ld, %ld, %ld, %ld \n", timer1_time.it_value.tv_sec, timer1_time.it_value.tv_nsec, timer1_time.it_interval.tv_sec, timer1_time.it_interval.tv_nsec);

  if(timer_settime(timer1, 0, &timer1_time, NULL) != 0){
    perror("No timer for you");
    exit(1);
  }

  void send_wave(int signo){
    //printf("Wave time\n");
    if(SENDHIGH == 1){
      printf("Sendhigh\n");
      SENDHIGH = 0;
      if(eDO(hDevice, 1, 3, 1) < 0){
	printf("The signal failed to send. Dingus.\n");
      }
    } else {
      printf("Sendlow\n");
      SENDHIGH = 1;
      if(eDO(hDevice, 1, 3, 0) < 0){
	printf("The signal failed to send. Dingus.\n");
      }
    }
    //printf("End wave time\n");
    
  }
  
  struct sigaction action;

  memset(&action, 0, sizeof(action));

  action.sa_sigaction = (void *) send_wave;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN, &action, NULL);
  sigset_t satemp;
  sigemptyset(&satemp);
  sigaddset(&satemp, SIGRTMIN);
  
	/* The square wave generated on the DAC0 analog pin should
	 * have the voltage range specified by the user in the step
	 * above. */
  printf("Enter 'exit' to end the program.\n");
  while (1){
    int i = 0;
    gets(str);
    //printf(str);
    //printf("%d\n", strcmp(str,"exit"));
    if (strcmp(str,"exit")==0) {
    	//printf("EXIT TIME\n");
      	closeUSBConnection(hDevice);
      	return EXIT_SUCCESS;
    }
  }
	/* Display a prompt to the user such that if the "exit"
	 * command is typed, the USB DAQ is released and the program
	 * is terminated. */
 return EXIT_SUCCESS;
}


int main(int argc, char **argv)
{
  char str1;
  printf("Digital or analog? (d/a)\n");
  gets(&str1);
  if (str1 == 'd') {
  digital();
  }
  else if (str1 == 'a') {
  analog();
  }
}

