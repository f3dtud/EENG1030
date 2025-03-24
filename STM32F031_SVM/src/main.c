#include <stm32f031x6.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#define BIT(n) (1 << n)
#define enable_interrupts() asm(" cpsie i ")
#define disable_interrupts() asm(" cpsid i ")
#define CPU_FREQUENCY 48000000
#define PWM_FREQUENCY 10000
#define OUTPUT_FREQUENCY 50
#define SCALING_FACTOR 1000
void initClock(void);
void setup(void);
void initSerial(uint32_t baudrate);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF);
void eputc(char c);
void initPWM(void);


void delay(volatile uint32_t dly)
{
    while(dly--);
}
int main()
{
    setup();
    while(1)
    {
        printf("hello world\r\n");
        delay(100000);
    }

}
void initClock()
{
// This is potentially a dangerous function as it could
// result in a system with an invalid clock signal - result: a stuck system
        // Set the PLL up
        // First ensure PLL is disabled
        RCC->CR &= ~(1<<24);
        while( (RCC->CR & (1 <<25))); // wait for PLL ready to be cleared
        
  // Warning here: if system clock is greater than 24MHz then wait-state(s) need to be
        // inserted into Flash memory interface        
        FLASH->ACR |= (1 << 0);
        FLASH->ACR &=~((1 << 2) | (1<<1));
        // Turn on FLASH prefetch buffer
        FLASH->ACR |= (1 << 4);
        // set PLL multiplier to 12 (yielding 48MHz)
        RCC->CFGR &= ~((1<<21) | (1<<20) | (1<<19) | (1<<18));
        RCC->CFGR |= ((1<<21) | (1<<19) ); 

        // Need to limit ADC clock to below 14MHz so will change ADC prescaler to 4
        RCC->CFGR |= (1<<14);

// Do the following to push HSI clock out on PA8 (MCO)
// for measurement purposes.  Should be 8MHz or thereabouts (verified with oscilloscope)
/*
        RCC_CFGR |= ( (1<<26) | (1<<24) );
        RCC_AHBENR |= (1<<17);
        GPIOA_MODER |= (1<<17);
*/
        // and turn the PLL back on again
        RCC->CR |= (1<<24);        
        // set PLL as system clock source 
        RCC->CFGR |= (1<<1);
}
void setup(void)
{
    initClock();
    RCC->AHBENR |= (1 << 18); // enable GPIOB

    pinMode(GPIOB,4,1);
    initSerial(9600);
    initPWM();
}
void initSerial(uint32_t baudrate)
{
    
	/* On the nucleo board, TX is on PA2 while RX is on PA15 */
	RCC->AHBENR |= (1 << 17); // enable GPIOA
	RCC->APB2ENR |= (1 << 14); // enable USART1
	pinMode(GPIOA,2,2); // enable alternate function on PA2
	pinMode(GPIOA,15,2); // enable alternate function on PA15
	// AF1 = USART1 TX on PA2
    
	GPIOA->AFR[0] &= 0xfffff0ff;
	GPIOA->AFR[0] |= (1 << 8);
	// AF1 = USART1 RX on PA2
	GPIOA->AFR[1] &= 0x0fffffff;
	GPIOA->AFR[1] |= (1 << 28);
	// De-assert reset of USART1 
	RCC->APB2RSTR &= ~(1 << 14);
	
	USART1->CR1 = 0; // disable before configuration
	USART1->CR3 |= (1 << 12); // disable overrun detection
	USART1->BRR = 48000000/9600; // assuming 48MHz clock and 9600 bps data rate
	USART1->CR1 |= (1 << 2) + (1 << 3); // enable Transmistter and receiver
	USART1->CR1 |= 1; // enable the UART
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
        eputc(*data);    
        data++;
    }    
    return 0;
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF)
{
    // The alternative function control is spread across two 32 bit registers AFR[0] and AFR[1]
    // There are 4 bits for each port bit.
    if (BitNumber < 8)
    {
        Port->AFR[0] &= ~(0x0f << (4*BitNumber));
        Port->AFR[0] |= (AF << (4*BitNumber));
    }
    else
    {
        BitNumber = BitNumber - 8;
        Port->AFR[1] &= ~(0x0f << (4*BitNumber));
        Port->AFR[1] |= (AF << (4*BitNumber));
    }
}
void eputc(char c)
{
    while( (USART1->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
    USART1->TDR=c;
}       

void initPWM()
{
    /*
        GPIOA.7 = CH1N
        GPIOA.8 = CH1
        GPIOA.9 = CH2
        GPIOA.10 = CH3
        GPIOB.0 = CH2N
        GPIOB.1 = CH3N
    */
    RCC->AHBENR |= (1 << 18) | (1 << 18); // enable GPIOA,GPIOB
    pinMode(GPIOA,7,2); // select Alternate function for PA7
    pinMode(GPIOA,8,2); // select Alternate function for PA8
    pinMode(GPIOA,9,2); // select Alternate function for PA9
    pinMode(GPIOA,10,2);// select Alternate function for PA10
    pinMode(GPIOB,0,2); // select Alternate function for PB0
    pinMode(GPIOB,1,2); // select Alternate function for PB1

    selectAlternateFunction(GPIOA,7,2);
    selectAlternateFunction(GPIOA,8,2);
    selectAlternateFunction(GPIOA,9,2);
    selectAlternateFunction(GPIOA,10,2);
    selectAlternateFunction(GPIOB,0,2);
    selectAlternateFunction(GPIOB,1,2);
    RCC->APB2ENR |= (1 << 11);
    TIM1->ARR = CPU_FREQUENCY/PWM_FREQUENCY;
    TIM1->CCR1 = TIM1->ARR/2;
    TIM1->CCR2 = TIM1->ARR/2;
    TIM1->CCR3 = TIM1->ARR/2;
    TIM1->CR2 = 0;
    TIM1->CCMR1=BIT(14) + BIT(13) + BIT(11) + BIT(10) + BIT(6) + BIT(5) + BIT(3) + BIT(2); // PWM mode 1 for CH1,CH2, Fast mode enable, preload enable
    TIM1->CCMR2= BIT(6) + BIT(5) + BIT(3) + BIT(2); // PWM mode 1 for CH3 Fast mode enable, preload enable    
    TIM1->CCER = BIT(0) + BIT(2) + BIT(4) + BIT(6) + BIT(8) + BIT(10); // // Enable OC1,OC2 outputs and their inverses
    TIM1->BDTR |= BIT(15)+10; // +0x08 main output enable pulse dead time.  Deadtime value is multiplied by 125ns
    TIM1->CR1 |= BIT(7)+BIT(2); // set the ARPE bit
    TIM1->EGR |= BIT(0); // force update of registers
    TIM1->PSC = 0;
    TIM1->DIER |= BIT(0); // enable update events
    TIM1->CR1 = BIT(0);
    NVIC->ISER[0] |= BIT(13);    
    enable_interrupts();
}
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
/*
 * Warning: it is really important to do something that consumes a few clock cycles in this ISR after the interrupt flags are cleared 
 * see : https://developer.arm.com/documentation/ka003795/latest
 */
	TIM1->SR =0; 
    GPIOB->ODR ^= (1 << 4);
}