#include <msp430.h>
#include <stdint.h>

#define SW BIT5
#define LED BIT4



//--------------------------------------------global variables--------------------------------------------------------------//
struct flags{                       //This structure holds all the flags used in the program
    uint8_t debState : 1;           //debState stores the last debounced state recorded; 0 - LOW, 1 - HIGH
    uint8_t watchDogTimeElapsed : 1;
}allFlags = {1,0};                  //debState is 1 initially because of pull-up resistor

uint8_t debounceShiftReg=0xff;      //To store the history of voltage level on SW

//--------------------------------------------function declaration--------------------------------------------------------------//
void portConfig();
void watchDogDelay();
void ACLKConfig();
void debounce();

//--------------------------------------------main()--------------------------------------------------------------//
int main(void){
    WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

    ACLKConfig();
    portConfig();

    while(1){

        if(allFlags.watchDogTimeElapsed){
            allFlags.watchDogTimeElapsed = 0;
            debounce();

        }
        watchDogDelay();
    }
	
	return 0;
}


//--------------------------------------------ISR--------------------------------------------------------------//

#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void){
    WDTCTL = WDTPW + WDTHOLD;               //Stop WDT
    IE1 &= ~WDTIE;                          //Disable WDT interrupts
    allFlags.watchDogTimeElapsed = 1;
    __bic_SR_register_on_exit(LPM3_bits + GIE);

}

//--------------------------------------------function definition--------------------------------------------------------------//
void portConfig(){
    P1OUT &= ~SW;                   //SW  is input
    P1REN |= SW;                    //USE pull-up for SW
    P1OUT |= SW;

    //Unused Pin config
    //All uninitialized Pins are input with pull-down
    P1REN |= ~SW;                   //Except SW all are pulled
    P1OUT &= SW;                    //Except SW all are pulled down

    P2REN = 0xff;                   //All P2 pins are input with Pulled down
    P2OUT = 0x00;
}

void watchDogDelay(){
    WDTCTL = WDTPW + WDTTMSEL + WDTIS0 + WDTIS1 + WDTCNTCL + WDTSSEL;       //WDT counts upto 64 before requesting Interrupt
    IE1 |= WDTIE;
    __bis_SR_register(LPM3_bits + GIE);
}


void ACLKConfig(){
//Configuring the ACLK with VLO
        BCSCTL3 |= LFXT1S_2;            //VLO runs at 12kHz
}


#define THRESHOLD_FOR_PRESS 0x3f            //Change this as per your need
#define THRESHOLD_FOR_RELEASE 0xfc


void debounce(){
    debounceShiftReg >>= 1;
    if(P1IN & SW){
        debounceShiftReg |= BIT7;               //puts the current state of SW in MSB of debounceShiftReg()
    }

    if(allFlags.debState){              //checks if last debounced state was RELEASED

        if(debounceShiftReg <= THRESHOLD_FOR_PRESS){        //Checking if button is pressed
            allFlags.debState = 0;
            P1OUT ^= LED;
        }

    } else{
        if(debounceShiftReg >= THRESHOLD_FOR_RELEASE){       //Checking if button is released
            allFlags.debState = 1;
        }
    }

}
