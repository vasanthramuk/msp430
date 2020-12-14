#include <msp430.h> 
#include<inttypes.h>

#define RS BIT0
#define EN BIT1
#define D4 BIT2
#define D5 BIT3
#define D6 BIT4
#define D7 BIT5

#define LCD_OUT P2OUT
#define LCD_SEL P2SEL
#define LCD_SEL2 P2SEL2
#define LCD_DIR P2DIR

#define CMD 0
#define DAT 1


void delay(unsigned char);
void pulseEN();
void initialize_4_bit_mode();
void operation(unsigned char ,unsigned char);
void timerSetting(void);
void send_4_bit_to_D(unsigned char);
void sendString(unsigned char *);
void setCursor(unsigned char, unsigned char);
void sendNumber(uint16_t);

unsigned char data_arr[4] = {D4, D5, D6, D7};   //important array. The order should be the increasing PINS that are connected to DISPLAY from D4 to D7
unsigned char i;
volatile unsigned char millisec_count;
uint16_t countdown=1;

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	                    // stop watchdog timer
	
	timerSetting();                                 //Initialize the ACLK and Timer0A
	delay(150);                                     //Boot up time for display
	LCD_SEL &= ~(EN | RS | D4 | D5 | D6 | D7);      //P2 pins are initialized for OUTPUT
	LCD_SEL2 &= ~(EN | RS | D4 | D5 | D6 | D7);
	LCD_DIR |= (EN | RS | D4 | D5 | D6 | D7);

	initialize_4_bit_mode();                        //To initialize LCD in 4-bit mode

	operation(0x01, CMD); delay(5);                 //Clear the display
	operation(0x0F, CMD); delay(1);                 //DCB = on on on
	setCursor(0,0);
	sendString("Hello No: ");

	BCSCTL3 &= ~LFXT1S_3;
	TA1CTL |= TASSEL_1 | MC_1;
	TA1CCR0 = 32768;
	TA1CCTL0 |= CCIE;

	while(1)
	{
	    setCursor(1,0);
	    sendNumber(countdown);
	    __bis_SR_register(LPM3_bits + GIE);
	}


}


#pragma vector=TIMER1_A0_VECTOR
__interrupt void timer1a_ISR(void)
{
    countdown++;
    __bic_SR_register_on_exit(LPM3_bits + GIE);
}

void delay(unsigned char ms)
{
    TA0CCTL0 |= CCIE;                                   //Enabled Interrupt for CCR0
    TA0CCR0 = ms*13;                                    //Set CCR0 to suitable value that generates delay of ms milliseconds
    __bis_SR_register(LPM3_bits + GIE);                 //Go to LPM3 and wait for interrrupt
    TA0CCR0=0;                                          //Stops the timer after generating delay
    TA0CCTL0 &= ~CCIE;
}

void timerSetting()
{
    while(IFG1 & OFIFG)                                 //To stabilize the LFXT1S Oscillator
           IFG1 &= ~OFIFG;
    BCSCTL3 |= LFXT1S_2;                                //Source of ACLK is VLO
    TA0CTL |= TASSEL_1 | MC_1;                          //Timer0 source is ACLK which is VLO in this case and timer counts in UP mode
}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer0a_ISR(void)
{
    __bic_SR_register_on_exit(LPM3_bits + GIE);         //When Timer0A interrupts, exit LPM and disable Global Interrupt
}



void pulseEN()
{
    LCD_OUT |= EN;
    delay(3);                                           //Creates a FALLING EDGE in EN pin
    LCD_OUT &= ~EN;
    delay(3);
}

void initialize_4_bit_mode()
{
    unsigned char temp=0;
    LCD_OUT &= ~RS;
    while(temp<3)
    {
       send_4_bit_to_D(0x03); delay(5);                 //Initialize 8-bit mode
       temp++;
    }
    send_4_bit_to_D(0x02);                              //Initialized to 4 bit mode
}

void send_4_bit_to_D(unsigned char data)                //Sends the lower nibble of 'data' to Dx pins of LCD
        {
            unsigned char j=0;
            for(; j<4; j++)
            {
                if(data & (1<<j))
                    LCD_OUT |= data_arr[j];
                else
                    LCD_OUT &= ~(data_arr[j]);
            }
            pulseEN();
        }
void operation(unsigned char data, unsigned char mode)  //Choose to send DATA or COMMAND by choosing mode = 'DAT' and mode = 'CMD' respectively
{
    (mode & 1)? (LCD_OUT |= RS):( LCD_OUT &= ~RS);
    send_4_bit_to_D(data>>4);
    send_4_bit_to_D(data);
}

void sendString(unsigned char *string)                  //Makes the string to be copied into DDRAM of LCD
{
    while(*string)
    {
        operation(*string, DAT);delay(1);
        string++;
    }
}
void setCursor(unsigned char row, unsigned char column) //Sets the Address Counter of LCD to location specified by (row, column)
{
    operation(column + row * 0x40 + 0x80, CMD);delay(1);//Note: Row and Columns both start from 0 and have the usual meaning as one would use in Matrix Algebra
}

void sendNumber(uint16_t number)                         //Prints the 16-bit unsigned number on LCD
{

    unsigned char temporary[4]="   ";
    unsigned char lsb, output_length=2;
    while(number>0)
    {
        lsb = number % 10;
        temporary[output_length] = '0' + lsb;
        output_length--;
        number = (number - lsb)/10;
    }

    sendString(temporary);
}
