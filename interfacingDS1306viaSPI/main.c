#include<msp430.h>
#include<stdint.h>

//=====================GPIO===============//
#define LED BIT0
#define CE BIT3
#define RTC_ALARM BIT0
#define SPI_SOMI BIT6
#define SPI_SIMO BIT7
#define SPI_CLK BIT5
#define UART_TX BIT2
#define UART_RX BIT1
//========================================//

#define UPNIBBLE(x) (x >> 4) + '0'
#define LOWNIBBLE(x) (x & 0x0f) + '0'

//SECOND-0, MINUTE-1, HOUR-2, DAY-3, DATE-4, MONTH-5, YEAR-6
#define FORMAT_DATE currentTime[4]
#define FORMAT_MONTH currentTime[5]
#define FORMAT_YEAR currentTime[6]
#define FORMAT_HOUR currentTime[2]
#define FORMAT_MINUTE currentTime[1]
#define FORMAT_SECOND currentTime[0]


//=========================RTC===================//
#define RTCCTL_WRITE 0x8F
#define RTCCTL_READ 0x0F

#define WP BIT6
#define HZ_1 BIT2
#define AIE1 BIT1
#define AIE0 BIT0

#define RTCSTAT 0x10
#define IRQF1 BIT1
#define IRQF0 BIT0
//================================================//


#define FORMATTED_TIME_LENGTH 20

//=============================CURRENT TIME AND DAY================================//
#define SECOND 0x00
#define MINUTE 0x00
#define HOUR 0x11
#define DAY 0x04
#define DATE 0x13
#define MONTH 0x01
#define YEAR 0x21
//=================================================================================//

//=============================Function declaration=============================//
void configPort(void);
void configSPI(void);
void setRTC(void);
void configUART(void);
void enableWriteProtectionRTC(uint8_t);                           //If parameter is 1, enables Write Protection. If 0, disables Write Protection
void readRTC();                                         //Reads the current RTC registers and stores them in currentTime[]
void waitForTX();
void waitForRX();
void setRTCLoop();
void readRTCLoop();
void printTimeOnPC();
void prepareTimeForPC();

//=============================Global variables=============================//
uint8_t timeProfile[7] = {SECOND, MINUTE, HOUR, DAY, DATE, MONTH, YEAR};         //The first element of the array contains the array from which the

volatile uint8_t currentTime[7];                                                      //This array would have the most recent info from RTC

volatile uint8_t timeForPC[FORMATTED_TIME_LENGTH];                              //This array contains formatted time to be sent via UART
volatile uint8_t TXCount, RXCount;                                                    //To control the number of TX and RX

void main(){


    WDTCTL = WDTPW + WDTHOLD;


    //Configuring DCO to operate at 1 MHz
    DCOCTL = 0;
    BCSCTL1 = 135;
    DCOCTL = 47;


    configPort();
    configSPI();
    configUART();

    while(1){

    readRTC();
    prepareTimeForPC();

    printTimeOnPC();

    P2IE |= RTC_ALARM;
    __bis_SR_register(LPM4_bits + GIE);
    }

}

//=============================Interrupt Service Routines=============================//

#pragma vector = USCIAB0RX_VECTOR
__interrupt void RX_ISR(void){
    if(IFG2 & UCB0RXIFG){
        currentTime[RXCount++] = UCB0RXBUF;         //This updates currentTime with RTC value
                                                    //It also acknowledges the interrupt by clearing UCB0RXIFG
        if(RXCount >= 7){
            IE2 &= ~UCB0RXIE;                           //After reception of all 7 bytes, disable further RX interrupts
            IFG2 &= ~UCB0RXIFG;
            __bic_SR_register_on_exit(LPM4_bits + GIE); //Also exit LPM4
        }
    }


}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void TX_ISR(void){
    if((IFG2 & UCB0TXIFG) && (IE2 & UCB0TXIE)){               //TO make sure the interrupt is caused by USCIB0

       UCB0TXBUF = 0x55;       //This sends 0x55 which has no significance as we are only interested in RXed data
                                //This acknowledges interrupt by clearing UCB0TXIFG
       TXCount++;
        if(TXCount >= 7){                    //Since we are Transmitting 7 bytes, the 8th TX is used for RECEIVING the 7 th character
            IE2 &= ~UCB0TXIE;
        }
    }

    if((IFG2 & UCA0TXIFG) && (IE2 & UCA0TXIE)){

        UCA0TXBUF = timeForPC[TXCount];            //This would acknowledge by clearing UCA0TXIFG

        TXCount++;
        if(TXCount >= FORMATTED_TIME_LENGTH){
            IE2 &= ~UCA0TXIE;
            __bic_SR_register_on_exit(LPM4_bits + GIE);
        }
    }

}

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void){
    if(P2IFG & RTC_ALARM){
        P2IFG &= ~RTC_ALARM;                            //Acknowledges the Interrupt
        __bic_SR_register_on_exit(LPM4_bits + GIE);
    }
}

//=============================Function definition=============================//
void configPort(){
    //Configuring GPIO
    P2OUT &= ~CE;
    P2DIR |= CE;

    P2DIR &= ~RTC_ALARM;        //ALARM is input from RTC's INT1 pin
    P2IES &= ~RTC_ALARM;        //Alarm is active high. So EDGE is rising

    //Configuring USCIB0
    P1SEL = SPI_SOMI + SPI_SIMO + SPI_CLK + UART_TX + UART_RX;
    P1SEL2 = SPI_SOMI + SPI_SIMO + SPI_CLK + UART_TX + UART_RX;

    P1REN |= ~(SPI_SOMI + SPI_SIMO + SPI_CLK + UART_TX + UART_RX);      //PORT1 Unused pulled-down
    P1OUT &= SPI_SOMI + SPI_SIMO + SPI_CLK + UART_TX + UART_RX;


    P2REN |= ~CE ;                                                      //PORT2 Unused pulled-down
    P2OUT &= CE;

}

void configSPI(){
    //Using SPI mode-1(CKPOL = 0; CKPHA = 1) -->> UCCKPOL = 0; UCCKPH = 0)
    UCB0CTL0 = UCMSB + UCMST + UCMODE_0 + UCSYNC;

    UCB0CTL1 = UCSWRST + UCSSEL_3;                      //Using SMCLK for powering USCIA0 module


    UCB0CTL1 &= ~UCSWRST;

}

void setRTC(){
       P2OUT |= CE;                     //Select the CHIP

       waitForTX();
       UCB0TXBUF = 0x80;                //Setting the RTC address in WRITE mode

       IE2 &= ~UCB0RXIE;                //As we are only interested in writing to the RTC registers
       IE2 |= UCB0TXIE;                 //Enable interrupts only for Transmitting

       TXCount = 0;
       __bis_SR_register(LPM4_bits + GIE);

       P2OUT &= ~CE;                    //Deselect the CHIP
}

void enableWriteProtectionRTC(uint8_t option){

    P2OUT |= CE;

    waitForTX();
    UCB0TXBUF = RTCCTL_WRITE;

    waitForTX();
    if(option){
        UCB0TXBUF = WP;                       //Write Protection, 1 Hz enabled and alarm interrupts disabled

    }
    else{
        UCB0TXBUF = 0x00;                         //Write Protection, all alarms and 1 Hz output disabled
    }

    waitForTX();

    P2OUT &= ~CE;
}

void configUART(void){
    //9600 BAUD, No parity, 8-bit, LSB first,  1 SPB

    UCA0CTL1 = UCSWRST + UCSSEL_2;      //Choose BRCLK from SMCL operating @ 1 MHz

    UCA0BR0 = 104;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_1;                 //9600-8-N configuration for UART

    UCA0CTL1 &= ~UCSWRST;
}

void readRTC(){
    P2OUT |= CE;

    waitForTX();
    UCB0TXBUF = 0x00;                       //0x00 - Writing the base address for READING IN BURST MODE

    while(!(IFG2 & UCB0RXIFG));
    IFG2 &= ~UCB0RXIFG;

    TXCount = RXCount = 0;

    IE2 |= UCB0RXIE + UCB0TXIE;                     //Enabling RXIE to copy received data

    __bis_SR_register(LPM4_bits + GIE);


    P2OUT &= ~CE;
}

void prepareTimeForPC(){

    //Format : dd/mm/yy
        timeForPC[0] = UPNIBBLE(FORMAT_DATE);
        timeForPC[1] = LOWNIBBLE(FORMAT_DATE);
        timeForPC[2] = '\\';
        timeForPC[3] = UPNIBBLE(FORMAT_MONTH);
        timeForPC[4] = LOWNIBBLE(FORMAT_MONTH);
        timeForPC[5] = '\\';
        timeForPC[6] = UPNIBBLE(FORMAT_YEAR);
        timeForPC[7] = LOWNIBBLE(FORMAT_YEAR);
        timeForPC[8] = ' ';

        //Format : hh:mm:ss
        timeForPC[9] = UPNIBBLE(FORMAT_HOUR);
        timeForPC[10] = LOWNIBBLE(FORMAT_HOUR);
        timeForPC[11] = ':';
        timeForPC[12] = UPNIBBLE(FORMAT_MINUTE);
        timeForPC[13] = LOWNIBBLE(FORMAT_MINUTE);
        timeForPC[14] = ':';
        timeForPC[15] = UPNIBBLE(FORMAT_SECOND);
        timeForPC[16] = LOWNIBBLE(FORMAT_SECOND);
        timeForPC[17] = ' ';

        //to insert Carriage Return
        timeForPC[18] = 0x0d;
        timeForPC[19] = 0x0a;

}
void printTimeOnPC(){
    IE2 |= UCA0TXIE;
    TXCount = 0;                            //To keep a tab on number of Bytes sent
    __bis_SR_register(LPM4_bits + GIE);     //Go to sleep and wait till TXBUF is empty. When empty, service the ISR

}

void waitForTX(){
    while(!(IFG2 & UCB0TXIFG));
}

void waitForRX(){
    while(UCB0STAT & UCBUSY);
}

void setRTCLoop(){                              //Sets the current time in RTC by a blocking routine


    enableWriteProtectionRTC(0);

    P2OUT |= CE;
    UCB0TXBUF = 0x80;
    waitForTX();

    for(TXCount = 0; TXCount < 7; TXCount++){
        UCB0TXBUF = timeProfile[TXCount];
        waitForTX();
    }
    P2OUT &= ~CE;

    enableWriteProtectionRTC(1);
}


void readRTCLoop(){                             //Read and place the current time in currentTime[] by a blocking routine

    P2OUT |= CE;

    waitForTX();
    UCB0TXBUF = 0x00;

    RXCount = 0;

    while(RXCount <7){
        waitForTX();
        UCB0TXBUF = 0x00;
        waitForRX();
        currentTime[RXCount++] = UCB0RXBUF;
    }

    P2OUT &= ~CE;

}

