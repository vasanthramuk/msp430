#include<msp430.h>
#include"lcdDisplay.h"
#include<stdint.h>

//-----------------------------Macro definition--------------------------------------//
#define BPM_UPPER_THRESHOLD 150
#define BPM_LOWER_THRESHOLD 60

//-----------------------------Function definitions--------------------------------------//
void portConfig();
void comparatorConfig();
void timerConfig();

//-----------------------------Global variables--------------------------------------//
volatile uint16_t pulsePeriodInInt;
uint16_t pulseInBPM;

unsigned int adcRawData = 0;

void main(){
    WDTCTL = WDTPW + WDTHOLD;

    __bis_SR_register(GIE);

    timerConfigForLCD();                        //Initialize the ACLK and Timer1A
    delay(150);                                 //Boot up time for display

    portConfig();

    initialize_4_bit_mode();                        //To initialize LCD in 4-bit mode
    operation(0x01, CMD); delay(5);                 //Clear the display
    operation(0x0F, CMD); delay(1);                 //DCB = on on on
    setCursor(0,0);
    sendString("Place your");
    setCursor(1,0);
    sendString("finger and wait");

    comparatorConfig();
    timerConfig();

    while(1){

        __bis_SR_register(LPM0_bits);             //Enter sleep and wait till time is captured

        pulseInBPM = 1966080 / (float)pulsePeriodInInt;

        if((pulseInBPM > BPM_UPPER_THRESHOLD) || (pulseInBPM < BPM_LOWER_THRESHOLD)){
            operation(0x01, CMD); delay(5);                 //Clear the display
            setCursor(0,0);
            sendString("You removed");
            setCursor(1,0);
            sendString("your finger!");
        }else{
            operation(0x01, CMD); delay(5);                 //Clear the display
            setCursor(0,0);
            sendString("BPM is "); sendNumber(pulseInBPM);
        }
    }
}

//-----------------------------ISR--------------------------------------//

#pragma vector=TIMER0_A1_VECTOR
__interrupt void timerA0_ISR(void){
    static uint16_t prevTime = 0;
    switch(TA0IV){
    case TA0IV_TACCR1:
        pulsePeriodInInt = TA0CCR1 - prevTime;
        prevTime = TA0CCR1;
        __bic_SR_register_on_exit(LPM0_bits);               //Wake up from sleep after calculating Pulse Period
        break;
    }
}
//-----------------------------Function definitons--------------------------------------//
void portConfig(){

    P1DIR |= BIT7;              //Comparator A output enabled in P1.7
                                //P1.2 used as TA0.CCI1A
    P1SEL |= BIT7 + BIT2;

    //Connection of LCD display
    LCD_SEL &= ~(EN | RS | D4 | D5 | D6 | D7);      //P2 pins are initialized for OUTPUT
    LCD_SEL2 &= ~(EN | RS | D4 | D5 | D6 | D7);
    LCD_DIR |= (EN | RS | D4 | D5 | D6 | D7);

    //Configuration for unused pins

}


void comparatorConfig(){
    //Reference is applied externally. Reference applied to - terminal
    //Rising edge causes interrupt
    CACTL1 =  CARSEL + CAON;

    //+ input applied to CA0; CA filter disabled currently, change it if needed
    //- input applied to CA3
    CACTL2 = P2CA0 + P2CA2 + P2CA1 + CAF;
    CAPD =  CAPD0 + CAPD3; //disables input buffer in P1.0 and P1.3
}

void timerConfig(){
    //Timer runs from ACLK in  continuous mode
    TA0CTL = TASSEL_1 + MC_2 + TACLR;               //ACLK running at 32768 Hz. Time period = 30.518 us
    TA0CCTL1 = CM_1 + CCIS_0  + SCS + CAP + CCIE;   //CCI is P1.2
}
