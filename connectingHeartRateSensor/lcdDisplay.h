
#include<stdint.h>

//pin Definition for LCD display
#define RS BIT0
#define EN BIT1
#define D4 BIT2
#define D5 BIT3
#define D6 BIT4
#define D7 BIT5

//Port definition for uController
#define LCD_OUT P2OUT
#define LCD_SEL P2SEL
#define LCD_SEL2 P2SEL2
#define LCD_DIR P2DIR

#define CMD 0
#define DAT 1
#define INS CMD

//function declaration
void delay(unsigned char);                      //Parameter is delay in milli second (only use positive integer values within 255)
void pulseEN();                                 //Creates a falling edge in the EN pin of LCD display
void initialize_4_bit_mode();                   //As the name says
void operation(unsigned char ,unsigned char);   //First parameter : COMMAND or ASCII character of operation
                                                // Second parameter : mode (CMD or DAT)
void timerConfigForLCD(void);                   //Uses Timer1A for producing delay()
                                                //Source of TA1 CLK is crystall oscillator operating @ 32768 Hz
void send_4_bit_to_D(unsigned char);            //Parameter is character whose lower nibble is sent Data line of LCD
void sendString(unsigned char *);               //Parameter is base address of any string
void setCursor(unsigned char, unsigned char);   //Parameter is (row, column)
                                                //0 headed index
void sendNumber(uint16_t);                      //Parameter is any Positive integer between 0 and 65535

//Global variables
unsigned char data_arr[4] = {D4, D5, D6, D7};   //important array. The order should be the increasing PINS that are connected to DISPLAY from D4 to D7
unsigned char i;
volatile unsigned char millisec_count;
uint16_t countdown=1;


void delay(unsigned char ms)
{
    TA1CCTL0 |= CCIE;                                   //Enabled Interrupt for CCR0
    TA1CCR0 = ms*33;                                    //Set CCR0 to suitable value that generates delay of ms milliseconds
    __bis_SR_register(LPM3_bits);                       //Go to LPM3 and wait for interrrupt
    TA1CCR0=0;                                          //Stops the timer after generating delay
    TA1CCTL0 &= ~CCIE;
}

void timerConfigForLCD()
{
    while(IFG1 & OFIFG)                                 //To stabilize the LFXT1S Oscillator
           IFG1 &= ~OFIFG;
    BCSCTL3 |= LFXT1S_0;                                //Source of ACLK is Crystal Oscillator
    TA1CTL |= TASSEL_1 | MC_1;                          //Timer1 source is ACLK which is Crystal osc in this case and timer counts in UP mode
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void timer1a_ISR(void)
{
    __bic_SR_register_on_exit(LPM3_bits);                //When Timer0A interrupts, exit LPM and disable Global Interrupt
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

    uint8_t temporary[6];                                   //This function leaves no leading spaces in front of the number

    temporary[0] = (number/10000) + '0'; number %= 10000;
    temporary[1] = (number/1000) + '0'; number %= 1000;
    temporary[2] = (number/100) + '0'; number %= 100;
    temporary[3] = (number/10) + '0'; number %= 10;
    temporary[4] = number + '0';
    temporary[5] = '\0';

    uint8_t counter = 0;
    while(*(temporary + counter) == '0')
        counter++;

    if(counter < 5 ) sendString(temporary + counter);       //When the number is non-zero start printing from the first number
    else sendString( temporary + 4 );                       //When the number is zero only print zero

}
