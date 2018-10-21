#include <msp430.h> 
/* Authors: Tiernan Cuesta and Kevin Purcell
 * Version 1
 * Communicating with Will Byers, Stranger Things Milestone
*/


int bytecount = 0;                  // Initializes parameter bytecount that influences the switch-case statements in the UART protocol
volatile unsigned int total_bytes;  // Initializes the total byte parameter that is used to track how many packets are going in and out of the system


// Port assignment protocol for off-board LED
void LED_setup(void)
{
    P1DIR |= BIT2;            // Sets P1.2 as an output --- RED LED
    P1SEL |= BIT2;            // Sets P1.2 to TimerA CCR1 --- RED LED
    P1DIR |= BIT3;            // Sets P1.3 as an output --- GREEN LED
    P1SEL |= BIT3;            // Sets P1.3 to TimerA CCR2 --- GREEN LED
    P1DIR |= BIT4;            // Sets P1.4 as an output --- BLUE LED
    P1SEL |= BIT4;            // Sets P1.4 to TimerA CCR3 --- BLUE LED
}

// TimerA assignments to set desired Pulse Width Modulation functionality
void PWM_Setup(void)
{
    TA0CTL = TASSEL_2 + MC_1 + ID_0 + TACLR;   // Sets timer0 to smclk and mode control up
    TA0CCR0 = 0xFF;             // Sets CCR0 as max 255 to cut-off packets to 8 bytes
    TA0CCR1 = 0x00;             // Initializes CCR1 as 0 to receive red LED duty cycle
    TA0CCR2 = 0x00;             // Initializes CCR2 as 0 to receive green LED duty cycle
    TA0CCR3 = 0x00;             // Initializes CCR3 as 0 to receive blue LED duty cycle

    // Sets all three control registers to outmode 3, set/reset mode. This allows for each LED to have its own duty cycle with respect to CCR0
    TA0CCTL1 |= OUTMOD_3;
    TA0CCTL2 |= OUTMOD_3;
    TA0CCTL3 |= OUTMOD_3;
}

// Various UART protocols to enable receiving and transfer ports to communicate between nodes
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
            total_bytes = UCA1RXBUF;        // Total byte length
            break;
        case 1:
            TA0CCR1 = UCA1RXBUF;            // Transfers desired duty cycle, or brightness, to the red LED P1.2
            break;
        case 2:
            TA0CCR2 = UCA1RXBUF;            // Transfers desired duty cycle, or brightness, to the green LED P1.3
            break;
        case 3:
            TA0CCR3 = UCA1RXBUF;            // Transfers desired duty cycle, or brightness, to the blue LED P1.4
            while(!(UCA1IFG & UCTXIFG));    // Checks to see if the USCI_A1 TX buffer is ready
            UCA1TXBUF = total_bytes - 3;    // Sends new number of total bytes to next node
            break;
        // Protocol for sending the remaining packets down to the cascading nodes
        default:
            if(bytecount > total_bytes)
            {
                bytecount = -1;
                total_bytes = 0;
            }
            else
            {
                while(!(UCA1IFG & UCTXIFG));// Checks to see if the USCI_A1 TX buffer is ready
                UCA1TXBUF = UCA1RXBUF;      // Transmits bytes to next node
            }
            break;
        }
        bytecount++;                        // Increments bytecount to initialize the next CCRX transfer
}
