#include<msp430.h>
#include<stdint.h>

#define LED_R   BIT2
#define LED_L   BIT3
#define LED_U   BIT4
#define LED_D   BIT5
#define SERVO_MOTOR BIT6

void configADC(void);
void configTimer(void);
void portConfig(void);
uint16_t mapJoysticToMotor(uint16_t *);

unsigned int xyLocation[2];
volatile uint8_t currentChannel = 0;

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	DCOCTL = 0;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

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

        uint16_t onTimeInMilli = mapJoysticToMotor(xyLocation);

        TA0CCR1 = onTimeInMilli;

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

    TA0CCR0 = 20000;                                     //Generates interrupt once in 20 ms. So PWM frequency of 50 Hz
    TA0CCTL0 = CCIE;

    TA0CCR1 = 1000;                                      //1 ms ON time ==> 0 degree(This should only be in the range of 1000 to 2000) 0 degree to 180 degree
    TA0CCTL1 = OUTMOD_7;

    TA0CTL = TASSEL_2 + MC_1 + TACLR;                    //SMCLK operating @ calibrated 1 MHz
}
void portConfig(void){
    P1OUT = 0;
    P1DIR |= LED_R + LED_L + LED_U + LED_D + SERVO_MOTOR;             //Port pins declared output
    P1SEL |= SERVO_MOTOR;                                             //Timer0_A output
}
uint16_t mapJoysticToMotor(uint16_t *value){             //maps change in ADC result into TA0CCR1
    uint16_t temp = *value + 1000;                       //0-1023 ===>> 1000-2000
    return (temp > 10000)? 10000:temp;
}
