#include<msp430.h>
#include<stdint.h>


#define LED BIT0
//=============================Function declaration=============================//
void configSPI(void);

//=============================Global variables=============================//
int main(){

    WDTCTL = WDTPW + WDTHOLD;


    //Configuring DCO to operate at 1 MHz
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;


    //Configuring GPIO
    P1OUT &= ~LED;
    P1DIR |= LED;

    P1REN |= ~LED;      //Unused pulled-down
    P1OUT &= LED;


    P2REN = 0xff;       //Unused pulled-down
    P2OUT = 0x0;

    configSPI();


    while(1){
        __bis_SR_register(LPM3_bits + GIE);
    }

    return 0;
}

//=============================Interrupt Service Routines=============================//

#pragma vector = USCIAB0RX_VECTOR
__interrupt void RX_ISR(void){
    if(IFG2 & UCB0RXIFG){
        if(UCB0RXBUF == 'V'){
            P1OUT ^= LED;
        }

    }

}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void TX_ISR(void){
    if(IFG2 & UCB0TXIFG){               //TO make sure the interrupt is caused by USCIB0
        UCB0TXBUF = 'V';
    }

}



//=============================Function definition=============================//
void configSPI(){
    //Using SPI mode-0(CKPOL = 0; CKPHA = 0) -->> UCCKPOL = 0; UCCKPH = 1)
    UCB0CTL0 = UCCKPH + UCMSB + UCMST + UCMODE_0 + UCSYNC;

    UCB0CTL1 = UCSWRST + UCSSEL_3;                      //Using SMCLK for powering USCIA0 module

    UCB0STAT = UCLISTEN;

    UCB0CTL1 &= ~UCSWRST;

    IE2 |= UCB0TXIE + UCB0RXIE;
}
