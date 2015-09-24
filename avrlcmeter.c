#ifndef F_CPU
#warning "F_CPU not defined"
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>
#include <avr/pgmspace.h>
#include "avrlcmeter.h"
#include "mylcd.h"
#include "version.h"

volatile unsigned char freq_24 = 0;
volatile unsigned int freq_count_16 = 0;
volatile unsigned int timer2_div25 = 0;
volatile unsigned long int running;

unsigned long int F1;
unsigned long int F2;
unsigned long int Ftest;

double Ls;
double Cs;
double Ct;
double Lt;

#define relay_on() cbi(PORTB,REEDRELAY);
#define relay_off() sbi(PORTB,REEDRELAY);

ISR(TIMER2_COMPA_vect)
{
	freq_count_16 = TCNT1;	// get the count contained in the counter, NOW
	if (timer2_div25 == 24) {	// because of div by 25, this routine gets called at 5Hz
		TCNT1 = 0;
		running = freq_count_16 + (65536 * freq_24);

		// reset things AFTER sending to display!
		timer2_div25 = 0;
		freq_24 = 0;

	}

	timer2_div25++;

}

// this timer will overflow twice or so
ISR(TIMER1_OVF_vect)
{
	freq_24++;
}

int main(void)
{
	// Temporary string for double to string conversion
	char tmpstring[16];

	// set up the 16 bit timer as an external frequency counter:
	TCCR1B |= (1 << CS10) | (1 << CS11) | (1 << CS12);	// External clock, rising edge
	TIMSK1 |= (1 << TOIE1);	// Enable overflow interrupt, it will overflow a few times in counting frequency

	// set up the 8 bit timer as a timebase for the frequency counter:
	TCCR2A |= (1 << WGM21);
	TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);	// CTC mode, 1024-bit prescale
	OCR2A = 125;		// something we can factor into .2 second delays (at 16MHz) with some arithmetic
	TIMSK2 |= (1 << OCIE2A);

	sei();			// enable global interrupts

	/* Setup hardware       */
	DDRB = 0xFF;

	/* enable internal pull up for the input switches     */
	PORTB |= _BV(LCSWITCH);
	PORTB |= _BV(ZEROSWITCH);

        cbi(DDRB, 0); // pushbutton zeroing
        sbi(PORTB, 0); // pullup on zero pushbutton

	/* Set REEDLRELAY as output     */
	sbi(DDRB, REEDRELAY);

        /* Switch it off, just in case  */
	relay_off();

	/* initialize display, cursor off */
	lcd_init(LCD_DISP_ON);

	/* clear display and home cursor */
	lcd_clrscr();

	/* put string to display */
	lcd_puts_P("AVR L/C Meter");
	lcd_gotoxy(0, 1);
	lcd_puts(Version);

	/* Switch to C position in order to start the calibration process       */

	if (bit_is_clear(PINB, LCSWITCH)) {
		lcd_gotoxy(0, 1);
		lcd_puts_P("switch to C/CAL");
	}

	/* Wait till the user switch to the C position  */
	while (bit_is_clear(PINB, LCSWITCH)) {
	};

	/* Calibration  */
	_delay_ms(1000);
	F1 = running;
	relay_on();
	_delay_ms(1000);	// Wait 1 seconds to stabilize
	F2 = running;		// get test frequency
	relay_off();

	lcd_clrscr();

	/* Calculate the residance Cs/Ls stray Inductance and Capacitance       */
	/* Cs = (F2^2 / (F1^2-F2^2))*C1value)     */
	Cs = square(F2 * 5) / ((square(F1 * 5) - square(F2 * 5))) * .000000001;
	Ls = 1 / (4 * square(M_PI) * square(F1 * 5) * Cs);

	while (1) {
		Ftest = running;

		if (bit_is_clear(PINB, LCSWITCH)) {
			if (running < 3) {
				lcd_gotoxy(0, 0);
				lcd_puts_P("Not an inductor ");
			} else {
				/* Inductor mode      */
				lcd_gotoxy(0, 1);
				dtostrf(Ftest, 5, 0, tmpstring);
				lcd_puts(tmpstring);
				lcd_puts("Hz");
				Lt = (square(F1 * 5) / (square(running * 5)) -
				      1) * Ls;
				lcd_gotoxy(0, 0);
				lcd_puts("Lx: ");

				if (Lt > .0001) {
					dtostrf(Lt * 1000, 6, 3, tmpstring);
					lcd_puts(tmpstring);
					lcd_puts("mH");
				}

				else if (Lt > .0000001) {
					dtostrf(Lt * 1000000, 6, 3, tmpstring);
					lcd_puts(tmpstring);
					lcd_puts("uH");
				} else {
					dtostrf(Lt * 1000000000, 6, 3, tmpstring);
					lcd_puts(tmpstring);
					lcd_puts("nH");
				}
			}
		_delay_ms(1000);
		lcd_clrscr();
		}

		if (bit_is_set(PINB, LCSWITCH)) {
			/* Capacitor mode     */
			lcd_gotoxy(0, 1);
			dtostrf(Ftest, 5, 0, tmpstring);
			lcd_puts(tmpstring);
			lcd_puts("Hz");

			Ct = (square(F1 * 5) / (square(running * 5)) - 1) * Cs;
			lcd_gotoxy(0, 0);
			lcd_puts("Cx: ");

			if (Ct > .0001) {
				dtostrf(Ct * 1000, 6, 3, tmpstring);
				lcd_puts(tmpstring);
				lcd_puts("mF");
			} else if (Ct > .0000001) {
				dtostrf(Ct * 1000000, 6, 3, tmpstring);
				lcd_puts(tmpstring);
				lcd_puts("uF");
			} else if (Ct > .0000000001) {
				dtostrf(Ct * 1000000000, 6, 3, tmpstring);
				lcd_puts(tmpstring);
				lcd_puts("nF");
			} else if (Ct <=0) {
                                lcd_puts_P("      ");
                                lcd_puts("pF");
                        } else {
/* Need to prevent this warning warning: integer constant is too large for ‘long’ type  */
				dtostrf(Ct * 1000000000000, 6, 0, tmpstring);
				lcd_puts(tmpstring);
				lcd_puts("pF");
			}
		//_delay_ms(1000);
		//lcd_clrscr();

                if(bit_is_clear(PINB, ZEROSWITCH)) {
                        lcd_gotoxy(0,0);
                        lcd_puts_P("zeroed                \r");
                        F1 = running;
                }
                while(bit_is_clear(PINB, ZEROSWITCH)) {
                        // do nothing till the user lets go of the zero button
                }

		_delay_ms(1000);
		lcd_clrscr();

		}
	}

	return 0;
}
