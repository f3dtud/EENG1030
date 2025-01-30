// DMA to the DAC triggered by Timer 7 
// DAC output is on PA4
// 

#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
void initClocks(void);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void initDAC(void);
void writeDAC(int value);
void initTimer7(void);
/*
Octave/matlab code to generate the sine wave table:
clear
angle_step=pi/128;
angle=[0:angle_step:2*pi-angle_step];
s=sin(angle);
s=(s+1)/2;
s=s*4095;
s=floor(s);
f=fopen('sine.txt','wt');
for(i=1:length(s))
  fprintf(f,"%d,",s(i));
end
fclose(f);

*/
const uint32_t sine_table[]={127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,179,182,184,\
                             187,190,193,195,198,200,203,205,208,210,213,215,217,219,221,224,226,228,229,231,233,\
                             235,236,238,239,241,242,244,245,246,247,248,249,250,251,251,252,253,253,254,254,254,\
                             254,254,255,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246,245,244,242,\
                             241,239,238,236,235,233,231,229,228,226,224,221,219,217,215,213,210,208,205,203,200,\
                             198,195,193,190,187,184,182,179,176,173,170,167,164,161,158,155,152,149,146,143,139,\
                             136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,75,72,70,67,\
                             64,61,59,56,54,51,49,46,44,41,39,37,35,33,30,28,26,25,23,21,19,18,16,15,13,12,10,9,8,\
                             7,6,5,4,3,3,2,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,2,3,3,4,5,6,7,8,9,10,12,13,15,16,18,19,21,\
                             23,25,26,28,30,33,35,37,39,41,44,46,49,51,54,56,59,61,64,67,70,72,75,78,81,84,87,90,93,\
                             96,99,102,105,108,111,115,118,121,124};
int vin;
int main()
{

    int i=0;
    
    setup();
    while(1)
    {        
       // writeDAC(sine_table[i]);
        i++;
        delay(100000);
        if (i > 255)
            i=0;
    }
}
void initClocks()
{
	// Initialize the clock system to a higher speed.
	// At boot time, the clock is derived from the MSI clock 
	// which defaults to 4MHz.  Will set it to 80MHz
	// See chapter 6 of the reference manual (RM0393)
	    RCC->CR &= ~(1 << 24); // Make sure PLL is off
	
	// PLL Input clock = MSI so BIT1 = 1, BIT 0 = 0
	// PLLM = Divisor for input clock : set = 1 so BIT6,5,4 = 0
	// PLL-VCO speed = PLL_N x PLL Input clock
	// This must be < 344MHz
	// PLL Input clock = 4MHz from MSI
	// PLL_N can range from 8 to 86.  
	// Will use 80 for PLL_N as 80 * 4 = 320MHz
	// Put value 80 into bits 14:8 (being sure to clear bits as necessary)
	// PLLSAI3 : Serial audio interface : not using leave BIT16 = 0
	// PLLP : Must pick a value that divides 320MHz down to <= 80MHz
	// If BIT17 = 1 then divisor is 17; 320/17 = 18.82MHz : ok (PLLP used by SAI)
	// PLLQEN : Don't need this so set BIT20 = 0
	// PLLQ : Must divide 320 down to value <=80MHz.  
	// Set BIT22,21 to 1 to get a divisor of 8 : ok
	// PLLREN : This enables the PLLCLK output of the PLL
	// I think we need this so set to 1. BIT24 = 1 
	// PLLR : Pick a value that divides 320 down to <= 80MHz
	// Choose 4 to give an 80MHz output.  
	// BIT26 = 0; BIT25 = 1
	// All other bits reserved and zero at reset
	    RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) + (1 << 17) + (80 << 8) + (1 << 0);	
	    RCC->CR |= (1 << 24); // Turn PLL on
	    while( (RCC->CR & (1 << 25))== 0); // Wait for PLL to be ready
	// configure flash for 4 wait states (required at 80MHz)
	    FLASH->ACR &= ~((1 << 2)+ (1 << 1) + (1 << 0));
	    FLASH->ACR |= (1 << 2); 
	    RCC->CFGR |= (1 << 1)+(1 << 0); // Select PLL as system clock
}
void setup(void)
{
    initClocks();
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1); // digital output
    pinMode(GPIOB,4,0); // digital input
    enablePullUp(GPIOB,4); // pull-up for button
    pinMode(GPIOA,0,3);  // analog input

    RCC->AHB1ENR |= (1 << 0); // enable DMA1
    DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CNDTR = 256;
    DMA1_Channel3->CPAR = (uint32_t)(&(DAC->DHR12L1));
    DMA1_Channel3->CMAR = (uint32_t)sine_table;
    DMA1_CSELR->CSELR = (0b0110 << 8); // DMA Trigger = DAC channel 1.
    DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CCR = 0;
    //DMA1_Channel3->CCR = (1 << 10)  | (1 < 8) | (1 << 7) | (1 << 5) | (1 << 4);    
    DMA1_Channel3->CCR =  (1 << 7) | (1 << 5) | (1 << 4);
    DMA1_Channel3->CCR |= (1 << 11)  | (1 < 9);
    DMA1_Channel3->CCR |= (1 << 0);
    initTimer7();
    initDAC();
}


void initDAC()
{
    // Configure PA4 as a DAC output
    RCC->AHB2ENR |= (1 << 0);     // ensure GPIOA is enabled    
    GPIOA->MODER |= (1 << 8) | (1 << 9); // Set mode to analogue (DAC) 
    RCC->APB1ENR1 |= (1 << 29);   // Enable the DAC
    RCC->APB1RSTR1 &= ~(1 << 29); // Take DAC out of reset
    DAC->CR = 0;         // Enable = 0
    DAC->CR |= (1 << 12) | (0b010 << 3) | (1 << 2);  // enable DMA and TIM7 trigger
    DAC->CR |= (1 << 0);          // Enable = 1
    writeDAC(0);
}
void writeDAC(int value)
{
    DAC->DHR12R1 = value;
}
void delay(volatile uint32_t dly)
{
    while(dly--);
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
void initTimer7(void)
{
    // Timer 2 can be used to trigger the DAC
    
    RCC->APB1ENR1 |= (1 << 5); // enable Timer 7
    TIM7->CR1 = 0;    
    TIM7->CR2 = (0b010 << 4); // update event is selected as trigger output
    TIM7->DIER = (1 << 8)+(1 << 0); // update dma request enabled - NECESSARY?
    TIM7->PSC = 0;
    TIM7->ARR = 20-1;
    TIM7->EGR = (1 << 0); // enable update event generation 
    TIM7->CR1 = (1 << 7);    
    TIM7->CR1 |= (1 << 0);  
}