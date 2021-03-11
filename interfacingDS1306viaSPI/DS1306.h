/*
 * DS1306.h
 *
 *  Created on: 16-Jan-2021
 *      Author: anonymous
 */

//=========================RTC===================//
#define RTCCTL_WRITE    0x8F
#define RTCCTL_READ     0x0F

#define RTC_SEC_READ    0x00
#define RTC_SEC_WRITE   0x80

#define WP              BIT6
#define HZ_1            BIT2
#define AIE1            BIT1
#define AIE0            BIT0

#define RTCSTAT_READ    0x10
#define IRQF1           BIT1
#define IRQF0           BIT0

#define RTC_ALARM0_WRITE    0x87
#define RTC_ALARM1_WRITE    0x8B
#define RTC_ALARM_MASK_BIT  BIT7
//================================================//


#define RTC_CE_PXDIR  P2DIR
#define RTC_CE_PXOUT  P2OUT
#define CE            BIT3

volatile struct RTCTime{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t dayOfWeek;
    uint8_t day;
    uint8_t month;
    uint8_t year;

    uint8_t TXCount;
    uint8_t RXCount;
};

void readRTCTime(struct RTCTime *timeStructure){
    RTC_CE_PXOUT |= CE;
    udelay();                                       //Generates 64 us delay

    timeStructure -> RXCount = timeStructure -> TXCount = 0;
    IE2 |= UCB0TXIE + UCB0RXIE;

    while(RXCount < 7){
    __bis_SR_register(LPM0_bits);


    }

    RTC_CE_PXOUT &= ~CE;
    udelay();

}


void udelay(){
    WDTCTL = WDTPW + WDTTMSEL + WDTCNTL + WDTIS0 + WDTIS1;      //Timer Mode with SMCLK; interrupt generated after 64 cycles of SMCLK; approx 64 us delay
    IE1 |= WDTIE;
    __bis_SR_register(LPM0_bits);
}


//===================================ISR======================================================//
#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void){
    //Interrupt acknowledged automatically
    WDTCTL = WDTPW + WDTHOLD;
    IE1 &= ~WDTIE;
    __bic_SR_register_on_exit(LPM0_bits);
}
