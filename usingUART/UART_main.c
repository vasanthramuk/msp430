#include <msp430.h>
#include <stdint.h>

//CLOCK CONFIG
#define DCO_1MHZ CALDCO_1MHZ
#define BC1_1MHZ CALBC1_1MHZ
#define BR_GENERATOR 104

//GPIO CONFIG
#define LED_GREEN BIT7
#define LED_ORANGE BIT6
#define LED_BLUE BIT4
#define RXD BIT1
#define TXD BIT2

//FLAGS
#define RECEIVED_ERROR_FLAG BIT0
#define RECEIVED_FLAG BIT1
#define NON_ZERO_RECEIVED BIT2

//------------------------------------function declaration------------------------------------//
    void sendToMonitor(uint8_t * );
    void sendNoToMonitor(uint16_t);
    uint32_t  uintToBCD(uint16_t);
    void watchDogDelay(void);
//--------------------------------------------------------------------------------------------//

//------------------------------------Global variables------------------------------------//
    uint8_t allFlags=0;
    uint8_t RX_buff=0;
//--------------------------------------------------------------------------------------------//

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

//Configuring the DCO for 1 MHZ
    DCOCTL = 0;
    BCSCTL1 = BC1_1MHZ;
    DCOCTL = DCO_1MHZ;

//Configuring the ACLK with VLO and dividing by 2
        BCSCTL1 |= DIVA_1;
        BCSCTL3 |= LFXT1S_2;            //VLO runs at 6kHz


//Configuring the UART module
    UCA0CTL1 |= UCSWRST;       //USCI RESET
    UCA0CTL1 |= UCSSEL_2;       //SMCLK
    UCA0BR0 = BR_GENERATOR;   //9600-8-O CONFIGURATION
    UCA0MCTL = UCBRS_1;
    UCA0CTL1 &= ~UCSWRST;       //USCI enabled for operation

    //UCA0CTL1 |= UCRXEIE;        //Interrupt  for invalid input
    //IE2 |= UCA0RXIE;


//Port Configuration
    P1SEL |= RXD + TXD;       //P1.1 - RXD ; P1.2 - TXD
    P1SEL2 |= RXD + TXD;

    P1OUT &= ~(LED_GREEN + LED_ORANGE + LED_BLUE); //To show success or failure status of transmission
    P1DIR |= LED_GREEN + LED_ORANGE + LED_BLUE;

//Unused Port configuration
    P2REN = 0xff;              //All P2.x are pulled down inputs
    P2OUT = 0x00;

    P1REN |= ~(LED_GREEN + LED_ORANGE + LED_BLUE + RXD + TXD); //Unused P1.x are pulled down inputs
    P2OUT &= LED_GREEN + LED_ORANGE + LED_BLUE + RXD + TXD;





    while(1)
        {

#ifdef LATER

        if(allFlags & RECEIVED_ERROR_FLAG){

            allFlags &= ~RECEIVED_ERROR_FLAG;           //Acknowledging the FLAG by RESET

            sendToMonitor(" Error in user input\n\r");

        }
        else if(allFlags & RECEIVED_FLAG){

                allFlags &= ~(RECEIVED_FLAG);           //Acknowledging the FLAG by RESET

                if(RX_buff == 'o' | RX_buff == 'O'){
                    P1OUT &= ~(LED_ORANGE + LED_GREEN + LED_BLUE);
                    P1OUT |= LED_ORANGE;
                }
                else if(RX_buff == 'g' | RX_buff == 'G'){
                    P1OUT &= ~(LED_ORANGE + LED_GREEN + LED_BLUE);
                    P1OUT |= LED_GREEN;
                }
                else{
                    P1OUT &= ~(LED_ORANGE + LED_GREEN + LED_BLUE);
                    P1OUT |= LED_BLUE;
                }

                sendToMonitor("Received\n\r");
            }

        __bis_SR_register(LPM0_bits + GIE);     //Go to sleep until the character is received
#endif


            uint16_t num=0;
            for(;num < 100; num++)
            {
            sendNoToMonitor(num);
            sendToMonitor("\n\r");

            }
            watchDogDelay();

            __bis_SR_register(LPM0_bits + GIE);
        }
    return 0;
}

//--------------------------------------------Interrupt Service Routines-------------------------------------------------------------//

#pragma vector = USCIAB0RX_VECTOR
__interrupt void RX_ISR(void)
{
    if(UCA0STAT & UCRXERR){
        allFlags |= RECEIVED_ERROR_FLAG;
    }
    else{
    allFlags |= RECEIVED_FLAG;
    }
    RX_buff = UCA0RXBUF;


    __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void){
    WDTCTL = WDTPW + WDTHOLD;
    __bic_SR_register_on_exit(LPM0_bits + GIE);
}
 
//--------------------------------------------function definition-------------------------------------------------------------//
void sendToMonitor(uint8_t * byte)
{
    while(*byte)
    {

        while(!(IFG2 & UCA0TXIFG));     //Wait till TXBUF is available
        UCA0TXBUF = *byte;               //Write every character of ack[]
        byte++;
    }
}


uint32_t  uintToBCD(uint16_t input)
{
    uint32_t converted=0;                          //This is where we are going to store the BCD equivalent of 'input'
    uint8_t i;                                      //For counting the loop

    //---Double-Dabble algorithm of converting binary to BCD---//
    for(i = 16 ; i > 0; i--)                                    //Replace 16 by no. of bits the input has, if needed
    {
        converted = __bcd_add_long(converted, converted);    //Double the present value of converted
        if( input & 0x8000) {                                //Checking the MSB of input ---> if 0, add 0 to converted; else, add 1 to converted
            converted = __bcd_add_long(converted, 1);
        }
        input <<= 1;                                         //Shift input one bit to left
    }
    return converted;
}

void sendNoToMonitor(uint16_t input)
{
    uint32_t noInBCD = uintToBCD(input);

    int8_t count=7; uint8_t whatToSend;
    uint32_t bitMask = 0xf0000000;

    while(count >= 0 ){

        whatToSend = (noInBCD & bitMask) >> (4*count);
        if(whatToSend ){                                         //Only send Non-Zero number
            while(!(IFG2 & UCA0TXIFG));
            UCA0TXBUF = '0' + whatToSend;
            allFlags |= NON_ZERO_RECEIVED;                       //Acknowledges that a non-zero number is received
        }
        else if(allFlags & NON_ZERO_RECEIVED ){
           while(!(IFG2 & UCA0TXIFG));
           UCA0TXBUF = '0' + whatToSend;                        //WhatToSend will be 0 here
        }

        bitMask >>= 4;
        count--;
    }
    allFlags &= ~NON_ZERO_RECEIVED;                             //Reset non-zer-received  to clear up for next function call

}

void watchDogDelay(){
    //Configuring the WDT for providing delay when need
        WDTCTL = WDTPW + WDTTMSEL + WDTSSEL + WDTCNTCL;         //Generates interrupt in 5 s of calling
        IE1 |= WDTIE;
}
