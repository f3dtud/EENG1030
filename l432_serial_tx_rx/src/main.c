// This example uses the USART2 to talk to the host PC using PA2 TX (from the Nucleo) 
// It works by replacing the "_write" function which is used by printf function to output data.
// Note: startup_stm32l432xx.S can be found here : .platformio/packages/framework-cmsis-stm32l4/Source/Templates/gcc/
// Cortex M4 core stuff (NVIC) can be found here : .platformio/packages/framework-cmsis/CMSIS/Include/
#include <eeng1030_lib.h>
#include <stdio.h>
#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "circular_buffer.h"
#define enable_interrupts() asm(" cpsie i ")
#define diable_interrupt() asm (" cpsid i")
void setup(void);
void delay(volatile uint32_t dly);
void initSerial(uint32_t baudrate);
int count;
circular_buffer rx_buf;

int main()
{
    setup();
    init_circ_buf(&rx_buf);
    NVIC->ISER[1] |= (1 << (38-32));
    EXTI->IMR1 |= (1 << 27);
    EXTI->EMR1 |= (1 << 27);
    enable_interrupts();
    while(1)
    {
        printf("test %d\r\n",rx_buf.count);
        delay(500000);
    }
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void setup()
{
    initClocks();    
    RCC->AHB2ENR |= (1 << 0) + (1 << 1); // enable GPIOA and GPIOB
    initSerial(9600);
}
void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX

    pinMode(GPIOA,15,2); // alternate function mode for PA15
    selectAlternateFunction(GPIOA,15,3); // AF3 = USART2 RX


    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=80000000;    
	uint32_t BaudRateDivisor;
	
	BaudRateDivisor = CLOCK_SPEED/baudrate;	
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 =  (1 << 3) | (1 << 2);  // enable the transmitter and receiver
    USART2->CR1 |= (1 << 5); // enable receiver interrupts
    USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->CR1 |= (1 << 0); // enable the UART
    USART2->ICR = (1 << 1); // clear any old framing errors
    
}
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    while(len--)
    {
        while( (USART2->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
        USART2->TDR=*data;    
        data++;
    }    
    return 0;
}
void USART2_IRQHandler()
{
    char c;
    if ((USART2->ISR &(1<<5)) )
    {
        c = USART2->RDR;
        put_circ_buf(&rx_buf,c);
    }
}