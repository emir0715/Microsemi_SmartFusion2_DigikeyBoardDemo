/*******************************************************************************
 *Digi-Key Board Demo for Sample Sorting for SmartFusion2
 *Application code (c) Copyright 2017 Microsemi SoC Products Group & Calit2 at the University of California, Irvine.
 *Base Example code (c) Copyright 2015 Microsemi SoC Products Group.  All rights reserved.
 *Extended application by Dr. Michael Klopfer (Calit2) + Crystal Lai (Calit2), Paolo Caros (Calit2) + Calit2 student team with support from Tim McCarthy (Microsemi)
 *This example project uses a UART connection to an ESP8266 to control LEDs and display messages.
 *Initial Build v1.0: Dec 2017
 *
 */

#include "platform.h"
#include "drivers/CorePWM/core_pwm.h"
#include "drivers/mss_timer/mss_timer.h"
#include "CMSIS/system_m2sxxx.h"
#include "hal.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include "drivers/CoreGPIO/core_gpio.h"
#include "m2sxxx.h"
#include <stdio.h>  //used for ARM Semihosting (Console Debug)

//**************************************************************************************************
//Debugging Control     NOTE: Comment out on production eNVM builds to avoid Semihosting errors!!  (There is no target for the debug messages when running without a console)
//**************************************************************************************************

#define VERBOSEDEBUGCONSOLE //Verbose debugging in console using ARM Semihosting, comment out to disable console debug messages - do not go too crazy with Semihosting, it will slow down operation if used excessively.

#ifdef VERBOSEDEBUGCONSOLE
	   extern void initialise_monitor_handles(void); //ARM Semihosting enabled
#endif

/******************************************************************************
 * Sample baud value to achieve UART communication at a 74880 baud rate with a (50MHz Digikeyboard clock source.)
 * This value is calculated using the following equation:
 *      BAUD_VALUE = (CLOCK / (16 * BAUD_RATE)) - 1
 *      For CoreUART
 *****************************************************************************/
#define BAUD_VALUE    57
	   // 57 as calculated baud value for 74880, used for ESP8266
	   //56, 0F sent, same with 58


/******************************************************************************
 * Maximum UART receiver buffer size.
 *****************************************************************************/
#define MAX_RX_DATA_SIZE    10 //RX Buffer Length
 

/******************************************************************************
 * CoreGPIO instance data.
 *****************************************************************************/
gpio_instance_t g_gpio;


/******************************************************************************
 * CoreUARTapb instance data.
 *****************************************************************************/

UART_instance_t g_uart;


#define COREUARTAPB0_BASE_ADDR	0x50002000

/******************************************************************************
 * TX Send message (This was originally a heatbeat - now it is used to clear the display in the bowser)
 *****************************************************************************/
uint8_t g_message[] ="";  //Blanking message for webpage - can be used in example to send as heartbeat

/******************************************************************************
 * CorePWM declare base_addr.
 *****************************************************************************/
#define COREPWM_BASE_ADDR 0x50000000

/******************************************************************************
 * CorePWM instance data.
 *****************************************************************************/
pwm_instance_t the_pwm;
uint32_t duty_cycle = 1;  //Set PWM initial duty cycle

/******************************************************************************
 * Delay count used to time the delay between duty cycle updates.  (Unless fabric is changed please do not modify, unless you have an exceedingly good reason to do so)
 *****************************************************************************/
#define DELAY_COUNT     6500  //principle multiplier to set delays in function (changing this changes the length of all delays, modify with caution, should be set so a delay of 1000 = 1 second, hence a unit is equalled approximately to 1 ms)

/******************************************************************************
 * PWM prescale and period configuration values to set PWM frequency and duty cycle.  (Unless fabric is changed please do not modify, unless you have an exceedingly good reason to do so, these values work for hobby servos)
 *****************************************************************************/
#define PWM_PRESCALE    250  //Pre-scale value 4
#define PWM_PERIOD      499  //full period
//Set PWM Period and duty cycle values


/******************************************************************************
//LED Name mapping to PWM Outputs
 ******************************************************************************/
#define LED2 2 //(Digi-Key Board- Pin 118 )
#define LED3 3 //(Digi-Key Board- Pin 122)
#define LED4 4 //(Digi-Key Board- Pin 123)
#define LED5 5 //(Digi-Key Board- Pin 124)
#define LED6 6 //(Digi-Key Board- Pin 125)
#define LED7 7 //(Digi-Key Board- Pin 128)
#define LED8 8 //(Digi-Key Board -Pin 129)

/******************************************************************************
 * Local function prototypes.
 *****************************************************************************/
void delay( int mult );


/******************************************************************************
 * Program MAIN function.
 *****************************************************************************/
int main( void )
{
#ifdef VERBOSEDEBUGCONSOLE
	initialise_monitor_handles();
	iprintf("Debug messages via ARM Semihosting initialized\n");  //Notification of Semihosting enabled
#endif
/**************************************************************************
 * Initialize the CorePWM instance setting the prescale and period values.
 *************************************************************************/
		PWM_init( &the_pwm, COREPWM_BASE_ADDR, PWM_PRESCALE, PWM_PERIOD );
	    delay(200);  //add ~200ms delay to prevent HAL assertion issue

/**************************************************************************
* Initialize CoreUARTapb with its base address, baud value, and line
* configuration.
*************************************************************************/

	UART_init( &g_uart, COREUARTAPB0_BASE_ADDR,
	BAUD_VALUE, (DATA_8_BITS | NO_PARITY) ); //set to communicate with 115200/8/N/1

	/**************************************************************************
* Initialize communication components of application
*************************************************************************/
//Initialize UART RX buffer

	    uint8_t rx_data[MAX_RX_DATA_SIZE]={0}; //initialize buffer as all 0's
	    size_t rx_size;



#ifdef VERBOSEDEBUGCONSOLE
	iprintf("CoreUARTapb initialized\n");
#endif

// MSS Timer Initialization
		/*--------------------------------------------------------------------------
		* Ensure the CMSIS-PAL provided g_FrequencyPCLK0 global variable contains
		* the correct frequency of the APB bus connecting the MSS timer to the
		* rest of the system.
		*/
		SystemCoreClockUpdate();

#ifdef VERBOSEDEBUGCONSOLE
		iprintf("MSS Timer Initialized\n");
#endif

/**************************************************************************
* Initialize the CoreGPIO driver with the base address of the CoreGPIO
* instance to use and the initial state of the outputs.
*************************************************************************/
    GPIO_init( &g_gpio,    COREGPIO_BASE_ADDR, GPIO_APB_32_BITS_BUS );

/**************************************************************************
* Configure the GPIOs for the indicator LEDs
*************************************************************************/
    GPIO_config( &g_gpio, GPIO_0, GPIO_OUTPUT_MODE | GPIO_IRQ_LEVEL_LOW );  //LED1
    GPIO_config( &g_gpio, GPIO_1, GPIO_OUTPUT_MODE | GPIO_IRQ_LEVEL_HIGH  );  //GPIO-0 held high for normal operation mode
    GPIO_config( &g_gpio, GPIO_2, GPIO_OUTPUT_MODE | GPIO_IRQ_LEVEL_HIGH );  //GPIO-2(EEPROM) held high


/**************************************************************************
* Configure the GPIOs for Inputs.
*************************************************************************/
    GPIO_config( &g_gpio, GPIO_0, GPIO_INOUT_MODE | GPIO_IRQ_EDGE_POSITIVE  );  //Button 1
    GPIO_config( &g_gpio, GPIO_1, GPIO_INOUT_MODE | GPIO_IRQ_EDGE_POSITIVE  ); //Button 2
   /*CHECK*/ GPIO_config( &g_gpio, GPIO_2, GPIO_INOUT_MODE | GPIO_IRQ_EDGE_POSITIVE ); 

   GPIO_set_outputs(&g_gpio, GPIO_1_MASK | GPIO_2_MASK);

#ifdef VERBOSEDEBUGCONSOLE
	iprintf("CoreGPIO configured\n");
#endif
    
/**************************************************************************
* Enable individual GPIO interrupts. The interrupts must be enabled both at
* the GPIO peripheral and Cortex-M3 interrupt controller levels.
*************************************************************************/
 
	GPIO_enable_irq( &g_gpio, GPIO_0 ); //enable IRQ for button 1
	GPIO_enable_irq( &g_gpio, GPIO_1 ); //enable IRQ for button 2
/*
 * The following section was modified by TM on 4/18/17
 */
	NVIC_EnableIRQ(FabricIrq1_IRQn);
    NVIC_EnableIRQ(FabricIrq2_IRQn);

	// FabricIrq1 is from GPIN[0] which is connected to SW1; FabricIrq2 is from GPIN[1] which is connected to SW2
	NVIC_SetPriority(FabricIrq1_IRQn, 6u);
	NVIC_SetPriority(FabricIrq2_IRQn, 5u);	// set IRQ2 to a higher level than IRQ1; If SW2 is pressed while SW1 is depresses SW2 IRQ2 ISR will be executed

/**************************************************************************
* Configure Timer1
* Use the timer input frequency as load value to achieve a one second
* periodic interrupt.
*************************************************************************/
	uint32_t timer1_load_value;
    timer1_load_value = g_FrequencyPCLK0 *5;		// modify for 10 second delay  - this is the rate of clear for the displayed messages
    MSS_TIM1_init(MSS_TIMER_PERIODIC_MODE);
    MSS_TIM1_load_immediate(timer1_load_value);
    MSS_TIM1_start();
    MSS_TIM1_enable_irq();

#ifdef VERBOSEDEBUGCONSOLE
	iprintf("Timer initialized and started\n");
#endif

	char commandcontrol[6]={0};

/**************************************************************************
     * For-loop to check for input ffrom the ESP8266 via UART from Browser action.
*************************************************************************/
		while(1)
	    {
	        /*************************************************************************
	         * Check for any errors in communication while receiving data
	         * FIFO buffers must be enabled in the CoreUART design for this functionality to work!
	         ************************************************************************/
	        if(UART_APB_NO_ERROR == UART_get_rx_status(&g_uart))
	        {

	            /**********************************************************************
	             * Read data received by the UART.
	             *********************************************************************/
	            rx_size = UART_get_rx( &g_uart, rx_data, sizeof(rx_data) );

	            /**********************************************************************
	             * Echo back data received, if any.
	             *********************************************************************/

	            if ( rx_size > 0 && rx_data[0] == '*')  //seems like enough trash is observed and flushed before the next * is seen so the buffer is effectively cleared

	            {
							#ifdef VERBOSEDEBUGCONSOLE  //Test Printout
							iprintf("Test Array Printout of UART Buffer:\n"); //Identify operation via semihosting debug print (only check two not to overload semihosting)
							for(int y=0; y<MAX_RX_DATA_SIZE; y++) //read in the full buffer
							{
								iprintf("%c", rx_data[y]); //print out full buffer as integers
							}
							#endif
							#ifdef VERBOSEDEBUGCONSOLE  //Test Printout
							iprintf("Printout of buffer complete:\n"); //Identify operation via semihosting debug print (only check two not to overload semihosting)
							#endif
							for(int y=0; y<(MAX_RX_DATA_SIZE-2); y++) //Blank the C copy of FIFO Buffer
							{
								if (rx_data[y+1] == '0' && rx_data[y+2] == 'O') //Search for the business end of L1O
								{
									#ifdef VERBOSEDEBUGCONSOLE  //Test Printout
									iprintf("Found  LED 0 ON Command\n");
									#endif
									PWM_set_duty_cycle(&the_pwm, PWM_2, 255); //turn on LED3
									GPIO_set_outputs(&g_gpio, 0x00000000); //update GPIO pattern 1 for indicator LED - remember, this board's LEDs are inverted - a 0 is on!
								}
								else if (rx_data[y+1] == '0' && rx_data[y+2] == 'F') //Search for the business end of L1O
							    {
									#ifdef VERBOSEDEBUGCONSOLE  //Test Printout
									iprintf("Found  LED 0 OFF Command\n");
									#endif
									PWM_set_duty_cycle(&the_pwm, PWM_2, 0); //turn off LED3
									GPIO_set_outputs(&g_gpio, 0x00000001); //update GPIO pattern 1 for indicator LED - remember, this board's LEDs are inverted - a 0 is on!
								}

								else if (rx_data[y+1] == '1' && rx_data[y+2] == 'O') //Search for the business end of L1O
								{
									#ifdef VERBOSEDEBUGCONSOLE  //Test Printout
									iprintf("Found  LED 1 ON Command\n");
									#endif
									PWM_set_duty_cycle(&the_pwm, PWM_3, 0); //turn on LED3, remember LEDS are tied to 3.3 V such that state of 0 is on!
								}
								else if (rx_data[y+1] == '1' && rx_data[y+2] == 'F') //Search for the business end of L1O
								{
									#ifdef VERBOSEDEBUGCONSOLE  //Test Printout
									iprintf("Found  LED 1 OFF Command\n");
									#endif

									PWM_set_duty_cycle(&the_pwm, PWM_3, 200); //turn on LED3, remember LEDS are tied to 3.3 V such that state of 0 is on!
								}
								else
								{}  //Rounding out the IF statements

							}
							}


	        }
	    }
}


/*==============================================================================
 * Toggle LEDs and send message via UART on TIM1 interrupt.
 */
void Timer1_IRQHandler(void)
{
	 size_t size_in_fifo;
     size_in_fifo = UART_fill_tx_fifo( &g_uart, g_message, sizeof(g_message)); //send the hearbeat message to let the tablet know the link is still active
     UART_send( &g_uart, (const uint8_t *)&g_message, size_in_fifo );
#ifdef VERBOSEDEBUGCONSOLE
     initialise_monitor_handles();
     iprintf("Blanking message sent\n");
#endif

/* Clear TIM1 interrupt */
    MSS_TIM1_clear_irq();
}


/**********************************************************************
* Interrupt driven control of function execution.
*********************************************************************/
//Buttons are handled through interrupts.  GPIO 0 interrupt service routine is FabricIrq1 ISR.  GPIO 0: This interrupt service routine function will be called when the SW1.  
//It will keep getting called as long as the SW1 button is pressed because the GPIO 0 input is configured as a level interrupt source.

//Example:
//Enable Interrupt for Button 1 - GPIO_disable_irq( &g_gpio, GPIO_0 ); is changed to GPIO_enable_irq( &g_gpio, GPIO_0 );
//Enable Interrupt for Button 2 - GPIO_disable_irq( &g_gpio, GPIO_1 ); is changed to GPIO_enable_irq( &g_gpio, GPIO_1 );

void FabricIrq1_IRQHandler( void )
{
/**********************************************************************
* Disable GPIO interrupts while updating the delay counter and
* GPIO pattern since these can also be modified within the GPIO
* interrupt service routines.
*********************************************************************/
	GPIO_disable_irq( &g_gpio, GPIO_0 );
    GPIO_disable_irq( &g_gpio, GPIO_1 );

/******************************************************************************
* Write a message to the SoftConsole host via OpenOCD and the debugger
*****************************************************************************/
#ifdef VERBOSEDEBUGCONSOLE
    	initialise_monitor_handles();
		iprintf("BUTTON 1 pressed - \n");
#endif
	uint8_t button_1[] = "B1" ;
	UART_fill_tx_fifo(&g_uart, button_1 , sizeof(button_1) );
	UART_send(&g_uart, button_1 , sizeof(button_1) ); //Sending message through UART;
	for (int i=0; i<sizeof(button_1); i++)
	{
		button_1[i] = 0; //Blank out buffer after send with null characters
	}
	

/********************************************************************
* Clear interrupt both at the GPIO levels.
*********************************************************************/
    GPIO_clear_irq( &g_gpio, GPIO_0 );
    GPIO_clear_irq( &g_gpio, GPIO_1 );

/********************************************************************
* Clear the interrupt in the Cortex-M3 NVIC.
*********************************************************************/
    NVIC_ClearPendingIRQ(FabricIrq1_IRQn);
    GPIO_enable_irq( &g_gpio, GPIO_0 ); //re-enable IRQ for Button 1
    GPIO_enable_irq( &g_gpio, GPIO_1 );  //re-enable IRQ for Button 2

#ifdef VERBOSEDEBUGCONSOLE
    iprintf("Button 1 pressed\n"); //Just a notation that the interrupt routine has finished
#endif

}


//GPIO 1 interrupt service routine is FabricIrq2 ISR.
//GPIO 1 : This interrupt service routine function will be called when the SW2 button is pressed. It will only be called once even if the SW2 button is kept pressed because the GPIO 1 input is configured as a rising edge interrupt source. This ISR loops forever after the SW2 button is pressed.  The debugger must be terminated and re-launched or the board reset (if running without the debugger) to run the application again.

void FabricIrq2_IRQHandler( void )
{
/**********************************************************************
* If SW2 (stop switch) is pressed, disable PWM outputs and halt until 
*board is reset or debugger is re-launched, this is an effective E-STOP
***********************************************************************/

	GPIO_disable_irq( &g_gpio, GPIO_0 );	// disable the GPIO_0 (SW1) interrupt
    GPIO_disable_irq( &g_gpio, GPIO_1 );	// keep the GPIO_1 (SW2) interrupt enabled


/******************************************************************************
 * Write a message to the SoftConsole host via OpenOCD and the debugger
*****************************************************************************/
	uint8_t button_2[]="B2";//message sent when button 2 pressed

#ifdef VERBOSEDEBUGCONSOLE
	initialise_monitor_handles();
	iprintf("Switch 2 pressed \n");  //Print message that SW2 was pressed
#endif
	UART_fill_tx_fifo(&g_uart, button_2 , sizeof(button_2));
	UART_send(&g_uart, button_2, sizeof(button_2)); //Sending message through UART;
	for (int i=0; i<sizeof(button_2); i++)
	{
		button_2[i] = 0; //Blank out buffer after send with null characters
	}


#ifdef VERBOSEDEBUGCONSOLE
	iprintf("Button 2 pressed\n");  //Print message that PWM outputs were disabled
#endif
/********************************************************************
* Clear interrupt both at the GPIO levels.
*********************************************************************/
    GPIO_clear_irq( &g_gpio, GPIO_0 );
    GPIO_clear_irq( &g_gpio, GPIO_1 );

/********************************************************************
* Clear the interrupt in the Cortex-M3 NVIC.
*********************************************************************/
    NVIC_ClearPendingIRQ(FabricIrq1_IRQn);
    GPIO_enable_irq( &g_gpio, GPIO_0 ); //re-enable IRQ for Button 1
    GPIO_enable_irq( &g_gpio, GPIO_1 );  //re-enable IRQ for Button 2

#ifdef VERBOSEDEBUGCONSOLE
    iprintf("Button Pressed\n"); //Just a notation that the interrupt routine has finished
#endif

}

/******************************************************************************
 * Delay function.
 *****************************************************************************/
void delay( int mult )
{
    volatile int counter = 0;

    while ( counter < (DELAY_COUNT*mult) )
    {
        counter++;
    }
}



