
#include "lpc824.h"
#include "serial.h"
#include <stdio.h>
#include <stdlib.h>

#define PIN_PERIOD (GPIO_B1)
#define F_CPU 60000000L

void pickmeup(void);
void waitms(int);
void wait_1ms(void);
long int GetPeriod (int);
void forward (void);
void backward (void);
void spin (void);
void stop(void);

volatile unsigned char pwm_count=0;
volatile unsigned char pwm_count2=0;
void ConfigPins()
{
	SWM_PINENABLE0 |=BIT5; // Disables SWDIO
	SWM_PINENABLE0 |=BIT4; // Disables SWCLK need this to enable pins 2 and 3
	//SWM_PINENABLE0 |=BIT4; // Disables reset functionality, uncomment if we need an extra output pin.

//--------------------------Outputs---------------------------------------------
//---------------------------ARM------------------------------------------
	GPIO_DIR0 |= BIT15;// ArmPWM1//works fine
	GPIO_DIR0 |= BIT9;  //magnet
//	GPIO_DIR0 |= BIT1; //ArmPWM2 fine 
//------------------------------WHEELS-----------------------------------
	GPIO_DIR0 |= BIT2;  //should work fine Wheel1For
	GPIO_DIR0 |= BIT3;	//should work fine Wheel1Back
	GPIO_DIR0 |= BIT8;  //PWM //works fine Wheel2For
	GPIO_DIR0 |= BIT12;  //Works fine	   Wheel2Back
//------------------------------Retarded duo-------------------------
	GPIO_DIR0 |= BIT11; //Doesnt work - its retarded
	GPIO_DIR0 |= BIT10; //Doesnt work - its retarded
//--------------------------------BACKUP---------------------------------------
	//GPIO_DIR0 |= BIT5;  //Enable if not enough pins, disable reset functionality first

	//--------------------------INPUTS---------------------------------------------

	GPIO_DIR0 &= ~(BIT1); //input for period reader
}

//copied from adc--------------------------------------------------------------------------------------
void ADC_Calibration(void){
	unsigned int saved_ADC_CTRL;

	// Follow the instructions from the user manual (21.3.4 Hardware self-calibration)
	
	//To calibrate the ADC follow these steps:
	
	//1. Save the current contents of the ADC CTRL register if different from default.	
	saved_ADC_CTRL=ADC_CTRL;
	// 2. In a single write to the ADC CTRL register, do the following to start the
	//    calibration:
	//    – Set the calibration mode bit CALMODE.
	//    – Write a divider value to the CLKDIV bit field that divides the system
	//      clock to yield an ADC clock of about 500 kHz.
	//    – Clear the LPWR bit.
	ADC_CTRL = BIT30 | ((300/5)-1); // BIT30=CALMODE, BIT10=LPWRMODE, BIT7:0=CLKDIV
	// 3. Poll the CALMODE bit until it is cleared.
	while(ADC_CTRL&BIT30);
	// Before launching a new A/D conversion, restore the contents of the CTRL
	// register or use the default values.
	ADC_CTRL=saved_ADC_CTRL;
}
void InitADC(void){
	// Will use Pin 1 (TSSOP-20 package) or PIO_23 for ADC.
	// This corresponds to ADC Channel 3.  Also connect the
	// VREFN pin (pin 17 of TSSOP-20) to GND, and VREFP the
	// pin (pin 17 of TSSOP-20) to VDD (3.3V).
	
	SYSCON_PDRUNCFG &= ~BIT4; // Power up the ADC
	SYSCON_SYSAHBCLKCTRL |= BIT24;// Start the ADC Clocks
	ADC_Calibration();
	ADC_SEQA_CTRL &= ~BIT31; // Ensure SEQA_ENA is disabled before making changes	
	
	ADC_CTRL =1;// Set the ADC Clock divisor
	SWM_PINENABLE0 &=~BIT23; // Enable the ADC function on PIO_23	
	SWM_PINENABLE0 &=~BIT16; // Enable the ADC function on PIO_23
	SWM_PINENABLE0 &=~BIT22; // Enable the ADC function on PIO_23
	SWM_PINENABLE0 &=~BIT15; // Enable the ADC function on PIO_23
	ADC_SEQA_CTRL |= BIT31 + BIT18; // Set SEQA and Trigger polarity bits
}		

int ReadADC(void){
	ADC_SEQA_CTRL |= BIT26; // Start a conversion:
	while( (ADC_SEQA_GDAT & BIT31)==0); // Wait for data valid
	return ( (ADC_SEQA_GDAT >> 4) & 0xfff);
}
//--------------------------------------------------------------------------------------------
void InitMRTimers(void){
	SYSCON_SYSAHBCLKCTRL |= BIT10; // Enables clock for multi-rate timer. (Table 35) 
	SYSCON_PRESETCTRL |=  BIT7; // Multi-rate timer (MRT) reset control. 1: Clear the MRT reset. (Table 23)

	MRT_INTVAL0=BIT31|(60000000/2500); // load inmediatly; 2 Hz interrupt
	MRT_CTRL0=BIT0; // Set timer 0 in repeat interrupt mode and enable interrupt

	MRT_INTVAL1=BIT31|(60000000/1); // load inmediatly; 1 Hz interrupt(60000000/1)
	MRT_CTRL1=BIT0; // Set timer 1 in repeat interrupt mode and enable interrupt
		
	NVIC_ISER0|=BIT10; // Enable MRT interrupts in NVIC. (Table 7, page 19)
}

void delay(int len)
{
	while(len--);
}

volatile char flag = 0;
volatile int ArmPWM1=0;
volatile int ArmPWM2=0;
volatile int counter = 0;
volatile int TimerCounter = 0;
volatile int state = 1;


// This IRQ handler is shared by the four timers in the MRT
void MRT_IRQ_Handler(void)//--------------------------------------------------------------------------------------------------
{
	if(MRT_IRQ_FLAG & BIT0)
	{
		TimerCounter ++;
		MRT_STAT0 = BIT0; // Clear interrupt flag for timer 0
		pickmeup();
	}
	if(MRT_IRQ_FLAG & BIT1)
	{
		MRT_STAT1 = BIT0; // Clear interrupt flag for timer 1
	}
	if(MRT_IRQ_FLAG & BIT2)
	{
		MRT_STAT2 = BIT0; // Clear interrupt flag for timer 2
	}
	if(MRT_IRQ_FLAG & BIT3)
	{
		MRT_STAT3 = BIT0; // Clear interrupt flag for timer 3
	}

}
int getReading(int channel){
	int channelreading, temp;
	if (channel == 10){
		 // Select Channel 9
		ADC_SEQA_CTRL &= ~BIT31;
		ADC_SEQA_CTRL ^= BIT10;	
		ADC_SEQA_CTRL |= BIT31 + BIT18; 
		temp=ReadADC();
		channelreading=(temp*33000)/0xfff;
		ADC_SEQA_CTRL ^= BIT10;                                       
	}
	else if(channel == 3){
		ADC_SEQA_CTRL &= ~BIT31;
		ADC_SEQA_CTRL ^= BIT3; // Select Channel 3
		ADC_SEQA_CTRL |= BIT31 + BIT18; 
				
		temp=ReadADC();
		channelreading=(temp*33000)/0xfff;
		ADC_SEQA_CTRL ^= BIT3;
	}
	else if(channel == 2){ 
		ADC_SEQA_CTRL &= ~BIT31;
		ADC_SEQA_CTRL ^= BIT2; // Select Channel 3
		ADC_SEQA_CTRL |= BIT31 + BIT18; 
		//ADC_SEQA_CTRL ^= BIT9;		
		temp=ReadADC();
		channelreading=(temp*33000)/0xfff;
		ADC_SEQA_CTRL ^= BIT2;
	}
	else if(channel == 9){
		ADC_SEQA_CTRL &= ~BIT31;
		ADC_SEQA_CTRL ^= BIT9; // Select Channel 3
		ADC_SEQA_CTRL |= BIT31 + BIT18; 		
		temp=ReadADC();
		channelreading=(temp*33000)/0xfff;
		ADC_SEQA_CTRL ^= BIT9;
	}
	fflush(stdout);	
	return channelreading;	           
}

void main(void)
{
	int j, rc3,rc10,rc2,rc9,channel3, channel10,channel9,channel2, PerimeterDetected = 1, CleaningFlag = 1;
	long int count;
	float T, f;
	ConfigPins();
	initUART(115200);
//	InitMRTimers();
	//InitADC();
//	enable_interrupts();
	
	//copied from adc-----------------------------------------------------------------
	printf("Start\r\n");
	fflush(stdout);
	//--------------------------------------------------------------------------------
	
	GPIO_B9 = 1; //magnet off
	
	while(1)	{    	
    //	channel3 = getReading(3);
   		//	channel2 = getReading(2); Disabled RN Pin configured as input and should read 1 or 0.                             
	//	channel10 = getReading(10);
	//	channel9 = getReading(9); 
	//	rc3 = channel2;
		rc9 = channel3;
		rc10 = channel9;
		rc2 = channel10;
		
		count=GetPeriod(100);
		if(count>0)
		{
			T=count/(F_CPU*100.0);
			f=1/T;
			eputs("f=");
			PrintNumber(f, 10, 7);
			eputs("Hz, count=");
			PrintNumber(count, 10, 6);
			eputs("\r");
		}
		else
		{
			eputs("NO SIGNAL                     \r");
		}
		//waitms(200);
		//GPIO_B9 = 1;
	
//------------------------------------Perimeter Detected---------------------------------------------------------
	/*	if(PerimeterDetected) {
			if(CleaningFlag){
				TimerCounter = 0;
				CleaningFlag = 0;
			}
			if(TimerCounter<2){
				//backwards();
				printf("%i|backwards",TimerCounter);	
			}
			else if (TimerCounter<5){
				//spin();
				printf("%i|spinning",TimerCounter);
			}
			
		}	*/
		
		//	stop();	
	//--------------------------------------------------------------------------	
	}
}


//------------------------------------Picking up a coin--------------------------------------------------------------
void pickmeup(void) {
	pwm_count++;
	counter++;
	if(counter>3000){
		counter = 0;
		state++;
		if (state>6){
			state = 0;
			ArmPWM1 = 100;
			ArmPWM2 = 100;
		}
	}
		
	if(pwm_count>100)pwm_count=0;
	GPIO_B15=pwm_count>ArmPWM1?1:0;
//	GPIO_B1=pwm_count>ArmPWM2?1:0;

	if (state == 1){
		ArmPWM1 = 98;
		ArmPWM2 = 98;
	}
	else if (state == 2){
		ArmPWM1 = 100;
		ArmPWM2 = 97;
		GPIO_B9 = 0; //magnet on
	}
	else if (state == 3){
		ArmPWM1 = 95;
		ArmPWM2 = 100;
	}
	else if (state == 4){
		ArmPWM1 = 100;
		ArmPWM2 = 95;
	}
	else if (state == 5){
		ArmPWM1 = 98;
		ArmPWM2 = 100;
	}
	else if (state == 6){
		ArmPWM1 = 98;
		ArmPWM2 = 98;
	}
	else if (state == 0){
		ArmPWM1 = 100;
		ArmPWM2 = 100;
		GPIO_B9 = 1; //magnet off
	}
}
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	SYST_RVR = 0xffffff;  // 24-bit counter set to check for signal present
	SYST_CVR = 0xffffff; // load the SysTick counter
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(SYST_CSR & BIT16) return 0;
	}
	SYST_CSR = 0x00; // Disable Systick counter

	SYST_RVR = 0xffffff;  // 24-bit counter set to check for signal present
	SYST_CVR = 0xffffff; // load the SysTick counter
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(SYST_CSR & BIT16) return 0;
	}
	SYST_CSR = 0x00; // Disable Systick counter
	
	SYST_RVR = 0xffffff;  // 24-bit counter reset
	SYST_CVR = 0xffffff; // load the SysTick counter to initial value
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(SYST_CSR & BIT16) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(SYST_CSR & BIT16) return 0;
		}
	}
	SYST_CSR = 0x00; // Disable Systick counter

	return 0xffffff-SYST_CVR;
}
void wait_1ms(void){
	// For SysTick info check the LPC824 manual page 317 in chapter 20.
	SYST_RVR = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	SYST_CVR = 0; // load the SysTick counter
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while((SYST_CSR & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SYST_CSR = 0x00; // Disable Systick counter
}

void waitms(int len){
	while(len--) wait_1ms();
}
void forward (void){
	GPIO_B2 = 1;  // WHeel 1 for
	GPIO_B3 = 0;  //wheel 1 back
	GPIO_B8 = 1;  //wheel 2 for
	GPIO_B12 = 0;  //wheel 2 back
}
void backward (void){
	GPIO_B2 = 0;  // WHeel 1 for
	GPIO_B3 = 1;  //wheel 1 back
	GPIO_B8 = 0;  //wheel 2 for
	GPIO_B12 = 1;  //wheel 2 back
}
void spin (void){
	GPIO_B2 = 1;  // WHeel 1 for
	GPIO_B3 = 0;  //wheel 1 back
	GPIO_B8 = 0;  //wheel 2 for
	GPIO_B12 = 1;  //wheel 2 back
}
void stop(void){
	GPIO_B2 = 0;  // WHeel 1 for
	GPIO_B3 = 0;  //wheel 1 back
	GPIO_B8 = 0;  //wheel 2 for
	GPIO_B12 = 0;  //wheel 2 back
}	