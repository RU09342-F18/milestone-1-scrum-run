#include <msp430.h> 
/* Authors: Tiernan Cuesta and Kevin Purcell
 * Version 1
 * Communicating with Will Byers, Stranger Things Milestone
*/


int bytecount = 0;
volatile unsigned int total_bytes;



void LED_setup(void)
{
    P1DIR |= BIT2;            // Sets P1.2 as an output --- RED LED
    P1SEL |= BIT2;            // Sets P1.2 to TimerA CCR1 --- RED LED
    P1DIR |= BIT3;            // Sets P1.3 as an output --- GREEN LED
    P1SEL |= BIT3;            // Sets P1.3 to TimerA CCR2 --- GREEN LED
    P1DIR |= BIT4;            // Sets P1.4 as an output --- BLUE LED
    P1SEL |= BIT4;            // Sets P1.4 to TimerA CCR3 --- BLUE LED
}

void PWM_Setup(void)
{
    TA0CTL = TASSEL_2 + MC_1 + ID_0 + TACLR;   // Sets timer0 to smclk and mode control up
    TA0CCR0 = 0xFF;             // Sets register
    TA0CCR1 = 0x00;
    TA0CCR2 = 0x00;
    TA0CCR3 = 0x00;
    TA0CCTL1 |= OUTMOD_3;
    TA0CCTL2 |= OUTMOD_3;
    TA0CCTL3 |= OUTMOD_3;
}

void UART_Setup(void)
{
    P4SEL |= BIT4;              // UART TX
    P4SEL |= BIT5;              // UART RX
    P3SEL |= BIT3;              // Enables RX for P3.3
    P3SEL |= BIT4;              // Enables TX for P3.4

    UCA1CTL1 |= UCSWRST;        // Clears the UART control register 1
    UCA1CTL1 |= UCSSEL_2;       // Sets SMCLK
    UCA1BR0 = 104;              // For baud rate of 9600
    UCA1BR1 = 0;                // For baud rate of 9600

    UCA1MCTL |= UCBRS_1;        // Enables modulation control register 1
    UCA1MCTL |= UCBRF_0;        // Enables modulation control register 0
    UCA1CTL1 &= ~UCSWRST;       // Enables the UART control register 1
    UCA1IE |= UCRXIE;           // Enables the UART RX interrupt
    UCA1IFG &= ~UCRXIFG;        // Clears the UART interrupt flag
}



int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    __bis_SR_register(GIE);     // Set low power mode
    __enable_interrupt();
    UART_Setup();               // Includes UART setup in main function
    LED_setup();                // Includes LED setup in main function
    PWM_Setup();                // Includes PWM setup in main function
    for(;;){};
}

// UART interrupt vector protocol
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
        switch(bytecount)
        {
        case 0:
            //while(!(UCA1IFG & UCTXIFG));    // USCI_A1 TX buffer ready?
            total_bytes = UCA1RXBUF;        // Total byte length
            //UCA1TXBUF = total_bytes - 3;    // Sends new number of bytes to next node
            break;
        case 1:
            TA0CCR1 = UCA1RXBUF;
            break;
        case 2:
            TA0CCR2 = UCA1RXBUF;
            break;
        case 3:
            TA0CCR3 = UCA1RXBUF;
            while(!(UCA1IFG & UCTXIFG));    // USCI_A1 TX buffer ready?
            UCA1TXBUF = total_bytes - 3;    // Sends new number of bytes to next node
            break;
        default:
            if(bytecount > total_bytes)
            {
                bytecount = -1;
                total_bytes = 0;
            }
            else
            {
                while(!(UCA1IFG & UCTXIFG));// USCI_A1 TX buffer ready?
                UCA1TXBUF = UCA1RXBUF;      // Transmit bytes to next board
            }
            break;
        }
        bytecount++;
}
