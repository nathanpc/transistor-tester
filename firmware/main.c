/**
 *	main.c
 *	Transistor tester firmware for the MSP430G2553 microcontroller.
 *
 *	@author Nathan Campos <nathanpc@dreamintech.net>
 */

#include <msp430g2553.h>
#include <stdio.h>
#include <string.h>

#include "boolean.h"
#include "delay.h"
#include "bitio.h"

#include "HD44780.h"

// Probes.
#define A  BIT0
#define B  BIT1
#define _C BIT2
#define AR BIT3
#define BR BIT4
#define CR BIT5

// Global variables ('cause we all love them)
#define CHG_DELAY 20
bool RECOGNIZED = FALSE;

// Function prototypes.
void clear_pins();
void try_npn_nchannel();
void try_pnp_pchannel();
void try_diode();

/**
 *	Where the magic starts.
 */
void main() {
	WDTCTL = WDTPW + WDTHOLD;  // Stop watchdog timer.
	//BCSCTL1 = CALBC1_1MHZ;     // Set range.
	//DCOCTL = CALDCO_1MHZ;      // SMCLK = DCO = 1MHz
	//BCSCTL2 &= ~(DIVS_3);

	//_BIS_SR(GIE);  // TODO: Do (LPMX + GIE) for low-power + interrupts.

	// Set inputs and outputs.
	P1DIR &= ~(A + B + CR);
	P1DIR |= (AR + BR + _C);

	// Setup the LCD driver.
	lcd_init(FALSE, FALSE);

	while (TRUE) {
		unsigned int curr_test = 0;
		while (!RECOGNIZED) {
			clear_pins();

			switch (curr_test) {
			case 0:
				// NPN or N-Channel MOSFET.
				try_npn_nchannel();
			break;
			case 1:
				// PNP or P-Channel MOSFET.
				try_pnp_pchannel();
				break;
			case 2:
				// Diode.
				try_diode();
				break;
			/*case 3:
				// Short.
				try_short();
				break;*/
			default:
				// No luck.
				lcd_print("Nothing ", 0, 0);
			}
			
			curr_test++;
			if (curr_test > 4 || RECOGNIZED) {
				curr_test = 0;
			}
		}
		
		delay_ms(500);
		if (RECOGNIZED) {
			RECOGNIZED = FALSE;
			delay_ms(500);
		}
	}
}

/**
 *  Make all pins LOW.
 */
void clear_pins() {
	P1OUT &= ~(AR + BR + _C);
}

/**
 *  Checks if the device being tested is a NPN or a N-Channel MOSFET.
 */
void try_npn_nchannel() {
	// Assumes the Gate is in B.
	P1OUT |= AR;   // Collector (Drain) -> HIGH.
	P1OUT |= BR;   // Base (Gate)       -> HIGH.
	P1OUT &= ~_C;  // Emitter (Source)  -> LOW.
	delay_ms(CHG_DELAY);

	if ((P1IN & A) == 0) {
		// May be a NPN or a N-Channel MOSFET.
		P1OUT &= ~BR;  // Base (Gate) -> LOW.
		lcd_print("PASS 1", 1, 0);
		delay_ms(CHG_DELAY);

		if ((P1IN & A) == A) {
			lcd_print("PASS 2", 1, 0);
			// It's a NPN or a N-Channel MOSFET.
			RECOGNIZED = TRUE;

			// Now check if it's a BJT or a MOSFET.
			P1OUT |= BR;  // Base (Gate) -> HIGH.
			delay_ms(CHG_DELAY);

			if ((P1IN & B) == B) {
				// N-Channel MOSFET.
				lcd_print("N-Channel", 0, 0);
			} else {
				// NPN
				lcd_print("NPN     ", 0, 0);
			}
		}
	}

	// Assumes the Gate is in A.
	if (!RECOGNIZED) {
		P1OUT |= AR;   // Base (Gate)       -> HIGH.
		P1OUT |= BR;   // Collector (Drain) -> HIGH.
		P1OUT &= ~_C;  // Emitter (Source)  -> LOW.
		delay_ms(CHG_DELAY);
		
		if ((P1IN & B) == 0) {
			// May be a NPN or a N-Channel MOSFET.
			P1OUT &= ~AR;  // Base (Gate) -> LOW.
			lcd_print("PASS 1", 1, 0);
			delay_ms(CHG_DELAY);
			
			if ((P1IN & B) == B) {
				lcd_print("PASS 2", 1, 0);
				// It's a NPN or a N-Channel MOSFET.
				RECOGNIZED = TRUE;
				
				// Now check if it's a BJT or a MOSFET.
				P1OUT |= AR;  // Base (Gate) -> HIGH.
				delay_ms(CHG_DELAY);
				
				if ((P1IN & A) == A) {
					// N-Channel MOSFET.
					lcd_print("N-Channel", 0, 0);
				} else {
					// NPN
					lcd_print("NPN     ", 0, 0);
				}
			}
		}
	}

	// Assumes the Gate is in C.
	if (!RECOGNIZED) {
		P1OUT |= AR;   // Collector (Drain) -> HIGH.
		P1OUT &= ~BR;  // Emitter (Source)  -> LOW.
		P1OUT |= _C;   // Base (Gate)       -> HIGH.
		delay_ms(CHG_DELAY);
		
		if ((P1IN & A) == 0) {
			// May be a NPN or a N-Channel MOSFET.
			P1OUT &= ~_C;  // Base (Gate) -> LOW.
			lcd_print("PASS 1", 1, 0);
			delay_ms(CHG_DELAY);
			
			if ((P1IN & A) == A) {
				lcd_print("PASS 2", 1, 0);
				// It's a NPN or a N-Channel MOSFET.
				RECOGNIZED = TRUE;
				
				// Now check if it's a BJT or a MOSFET.
				P1OUT |= _C;  // Base (Gate) -> HIGH.
				delay_ms(CHG_DELAY);
				
				if ((P1IN & CR) == CR) {
					// N-Channel MOSFET.
					lcd_print("N-Channel", 0, 0);
				} else {
					// NPN
					lcd_print("NPN     ", 0, 0);
				}
			}
		}
	}
}

/**
 *  Checks if the device being tested is a PNP or P-Channel MOSFET.
 */
void try_pnp_pchannel() {
	// Assumes the Gate is in B.
	P1OUT &= ~AR;  // Collector (Drain) -> LOW.
	P1OUT |= BR;   // Base (Gate)       -> HIGH.
	P1OUT |= _C;   // Emitter (Source)  -> HIGH.
	delay_ms(CHG_DELAY);

	if ((P1IN & A) == 0) {
		// May be a PNP or a P-Channel MOSFET.
		P1OUT &= ~BR;  // Base (Gate) -> LOW.
		lcd_print("PASS 1", 1, 0);
		delay_ms(CHG_DELAY);

		if ((P1IN & A) == A) {
			lcd_print("PASS 2", 1, 0);
			// It's a PNP or a P-Channel MOSFET.
			RECOGNIZED = TRUE;

			// Now check if it's a BJT or a MOSFET.
			P1OUT &= ~BR;  // Base (Gate) -> LOW.
			delay_ms(CHG_DELAY);

			if ((P1IN & B) == 0) {
				// P-Channel MOSFET.
				lcd_print("P-Channel", 0, 0);
			} else {
				// PNP
				lcd_print("PNP     ", 0, 0);
			}
		}
	}

	// Assumes the Gate is in A.
	if (!RECOGNIZED) {
		P1OUT |= AR;   // Base (Gate)       -> HIGH.
		P1OUT &= ~BR;  // Collector (Drain) -> LOW.
		P1OUT |= _C;   // Emitter (Source)  -> HIGH.
		delay_ms(CHG_DELAY);
		
		if ((P1IN & B) == 0) {
			// May be a PNP or a P-Channel MOSFET.
			P1OUT &= ~AR;  // Base (Gate) -> LOW.
			lcd_print("PASS 1", 1, 0);
			delay_ms(CHG_DELAY);
			
			if ((P1IN & B) == B) {
				lcd_print("PASS 2", 1, 0);
				// It's a PNP or a P-Channel MOSFET.
				RECOGNIZED = TRUE;
				
				// Now check if it's a BJT or a MOSFET.
				P1OUT &= ~AR;  // Base (Gate) -> LOW.
				delay_ms(CHG_DELAY);
				
				if ((P1IN & A) == 0) {
					// P-Channel MOSFET.
					lcd_print("P-Channel", 0, 0);
				} else {
					// PNP
					lcd_print("PNP     ", 0, 0);
				}
			}
		}
	}

	// Assumes the Gate is in C.
	if (!RECOGNIZED) {
		P1OUT &= ~AR;  // Collector (Drain) -> LOW.
		P1OUT |= BR;   // Emitter (Source)  -> HIGH.
		P1OUT |= _C;   // Base (Gate)       -> HIGH.
		delay_ms(CHG_DELAY);
		
		if ((P1IN & A) == 0) {
			// May be a PNP or a P-Channel MOSFET.
			P1OUT &= ~_C;  // Base (Gate) -> LOW.
			lcd_print("PASS 1", 1, 0);
			delay_ms(CHG_DELAY);
			
			if ((P1IN & A) == A) {
				lcd_print("PASS 2", 1, 0);
				// It's a PNP or a P-Channel MOSFET.
				RECOGNIZED = TRUE;
				
				// Now check if it's a BJT or a MOSFET.
				P1OUT &= ~_C;  // Base (Gate) -> LOW.
				delay_ms(CHG_DELAY);
				
				if ((P1IN & CR) == 0) {
					// P-Channel MOSFET.
					lcd_print("P-Channel", 0, 0);
				} else {
					// PNP
					lcd_print("PNP     ", 0, 0);
				}
			}
		}
	}
}

/**
 *  Checks if the device being tested is a Diode.
 */
void try_diode() {
	// Assume the Anode is in A and the Cathode in C.
	P1OUT |= AR;   // Anode   -> HIGH.
	P1OUT &= ~_C;  // Cathode -> LOW.
	delay_ms(CHG_DELAY);

	if ((P1IN & A) == 0) {
		// May be a Diode. Invert the polarization.
		P1OUT &= ~AR;  // Anode   -> LOW.
		P1OUT |= _C;   // Cathode -> HIGH.
		lcd_print("PASS 1", 1, 0);
		delay_ms(CHG_DELAY);

		if ((P1IN & A) == 0) {
			lcd_print("PASS 2", 1, 0);
			RECOGNIZED = TRUE;

			lcd_print("Diode   ", 0, 0);
			lcd_print("A=1 C=3 ", 1, 0);
		}
	}

	// Assume the Cathode is in A and the Anode in C.
	if (!RECOGNIZED) {
		P1OUT &= ~AR;  // Cathode -> LOW.
		P1OUT |= _C;   // Anode   -> HIGH.
		delay_ms(CHG_DELAY);
		
		if ((P1IN & A) == A) {
			// May be a Diode. Invert the polarization.
			P1OUT &= ~AR;  // Cathode -> LOW.
			P1OUT |= _C;   // Anode   -> HIGH.
			lcd_print("PASS 1", 1, 0);
			delay_ms(CHG_DELAY);
			
			if ((P1IN & A) == A) {
				lcd_print("PASS 2", 1, 0);
				RECOGNIZED = TRUE;
				
				lcd_print("Diode   ", 0, 0);
				lcd_print("C=1 A=3 ", 1, 0);
			}
		}
	}
}

/**
 *	Setup the interrupts stuff.
 */
/*void setup_interrupts() {
	P1DIR &= ~(BT_INT + RE_A_INT);
	P2DIR &= ~RE_B_INT;
	P2SEL &= ~RE_B_INT;  // Turn XOUT into P2.7

	P1IES &= ~(BT_INT + RE_A_INT);  // Set the interrupt to be from LOW to HIGH.
	P1IFG &= ~(BT_INT + RE_A_INT);  // P1.3 and P1.7 IFG cleared
	P1IE |= (BT_INT + RE_A_INT);    // Set P1.3 and P1.7 as interrupt.
}*/

/**
 *	Setup the PWM stuff.
 */
/*void setup_pwm() {
	P2DIR |= PWM_PIN;
	P2SEL |= PWM_PIN;      // Set P2.2 to TA1.1

	TA1CCR0  = 100 - 1;    // PWM period.
	TA1CCTL1 = OUTMOD_7;   // CCR1 Reset/Set.
	TA1CCR1  = 0;          // CCR1 PWM duty cycle.
	TA1CTL   = TASSEL_2 + MC_1;
}*/


//
//	Interrupts
//

/**
 *	Interrupt service routine for P1.
 */
/*#pragma vector = PORT1_VECTOR
__interrupt void P1_ISR() {
	switch(P1IFG & (BT_INT + RE_A_INT)) {
		case BT_INT:
			// Handle the input array interrupt.
			P1IFG &= ~BT_INT;  // P1.3 IFG cleared.
			
			// Loops through the buttons.
			for (unsigned int i = 0; i < 4; i++) {
				unsigned char tmp[] = { 0b10000000, 0b01000000, 0b00100000, 0b00010000 };
				tmp[i] |= (shift_default_on & 0b00001111);

				// Check if the button was pressed.
				shift_out(tmp[i]);
				if ((P1IN & BT_INT) == BT_INT) {
					handle_bt_press(i);
				}
			}
			
			unsigned char lst = shift_default_on;
			//lst |= leds;
			shift_out(lst);
			
			while ((P1IN & BT_INT) == BT_INT) {}
			break;
		case RE_A_INT:
			// Handle the rotary encoder A interrupt.
			P1IFG &= ~RE_A_INT;  // P1.7 IFG cleared.
			handle_re_rotation();
		default:
			P1IFG = 0;
			break;
	}

	P1IFG = 0;
}*/
