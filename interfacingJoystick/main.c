#include<msp430.h>
#include<stdint.h>

#define LED_R   BIT2
#define LED_L   BIT3
#define LED_U   BIT4
#define LED_D   BIT5

void configADC(void);
void configTimer(void);
void portConfig(void);

volatile unsigned int xyLocation[2];
volatile uint8_t currentChannel = 0;

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	configADC();
	portConfig();
	configTimer();
	_bis_SR_register(GIE);
	while(1){
	    ADC10SA = xyLocation;
	    __bis_SR_register(LPM0_bits);

        ADC10CTL0 &= ~ENC;
        ADC10CTL1 |= INCH_1;
        ADC10CTL0 |= ENC;

	    if(xyLocation[0] > 700){
	        P1OUT = LED_R;
	    }else if(xyLocation[0] < 300){
	        P1OUT = LED_L;
	    }else if(xyLocation[1] > 700){
	        P1OUT = LED_D;
	    }else if(xyLocation[1] < 300){
	        P1OUT = LED_U;
	    }else{
	        P1OUT = 0;
	    }

	}
	return 0;
}


#pragma vector=ADC10_VECTOR
__interrupt void adc_ISR(void){
    __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer_ISR(void){
    ADC10CTL0 |= ADC10SC;
}
void configADC(void){
    ADC10CTL0 = MSC + ADC10ON + ENC + ADC10IE;
    ADC10CTL1 = INCH_1 + ADC10SSEL_3 + CONSEQ_1;
    ADC10AE0 = BIT0 + BIT1;                             //Using only A0 and A1;
    ADC10DTC1 = 2;                                      //Two transfers
    //One Block mode

}
void configTimer(void){
    TA0CTL = TASSEL_2 + MC_1 + TACLR;
    TA0CCR0 = 5000;                                     //Generates interrupt once in 5 ms
    TA0CCTL0 = CCIE;
}
void portConfig(void){
    P1OUT = 0;
    P1DIR |= LED_R + LED_L + LED_U + LED_D;             //Port pins declared output
}
