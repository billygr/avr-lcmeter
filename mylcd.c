#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "mylcd.h"

#define DDR(x) (*(&x - 1))	/* address of data direction register of port x */

#define lcd_e_delay()   __asm__ __volatile__( "rjmp 1f\n 1:" );
#define lcd_e_toggle()  toggle_e()
#define lcd_e_high()    LCD_E_PORT  |=  _BV(LCD_E_PIN);
#define lcd_e_low()     LCD_E_PORT  &= ~_BV(LCD_E_PIN);
#define lcd_rs_high()   LCD_RS_PORT |=  _BV(LCD_RS_PIN)
#define lcd_rs_low()    LCD_RS_PORT &= ~_BV(LCD_RS_PIN)
#define lcd_rw_high()   LCD_RW_PORT |=  _BV(LCD_RW_PIN)
#define lcd_rw_low()    LCD_RW_PORT &= ~_BV(LCD_RW_PIN)

#if LCD_LINES==1
#define LCD_FUNCTION_DEFAULT    LCD_FUNCTION_4BIT_1LINE
#else
#define LCD_FUNCTION_DEFAULT    LCD_FUNCTION_4BIT_2LINES
#endif

void toggle_e()
{
	lcd_e_high();
	lcd_e_delay();
	lcd_e_low();
}

void lcd_command(unsigned int cmd)
{
	lcd_waitbusy();
	lcd_write(cmd, 0);
}

unsigned int lcd_waitbusy()
{
	_delay_ms(10);
	return 0;
}

void lcd_clrscr(void)
{
	lcd_command(1 << LCD_CLR);
}

void lcd_write(unsigned int data, unsigned int rs)
{
	unsigned char dataBits;

	if (rs) {		// write data        (RS=1, RW=0)
		lcd_rs_high();
	} else {		// write instruction (RS=0, RW=0) 
		lcd_rs_low();
	}

	//lcd_rw_low();

	/* configure data pins as output */
	DDR(LCD_DATA0_PORT) |= 0x0F;

	/* output high nibble first */
	dataBits = LCD_DATA0_PORT & 0xF0;
	LCD_DATA0_PORT = dataBits | ((data >> 4) & 0x0F);
	lcd_e_toggle();

	/* output low nibble */
	LCD_DATA0_PORT = dataBits | (data & 0x0F);
	lcd_e_toggle();

	/* all data pins high (inactive) */
	LCD_DATA0_PORT = dataBits | 0x0F;

}

void lcd_init(unsigned int dispAttr)
{
	DDR(LCD_DATA0_PORT) |= 0x7F;

	_delay_ms(16);		/* wait 16ms or more after power-on       */
	/* initial write to lcd is 8bit */
	LCD_DATA1_PORT |= _BV(LCD_DATA1_PIN);	// _BV(LCD_FUNCTION)>>4;
	LCD_DATA0_PORT |= _BV(LCD_DATA0_PIN);	// _BV(LCD_FUNCTION_8BIT)>>4;
	lcd_e_toggle();
	_delay_ms(5);		/* delay, busy flag can't be checked here */

	/* repeat last command */
	lcd_e_toggle();
	_delay_ms(1);		/* delay, busy flag can't be checked here */

	/* repeat last command a third time */
	lcd_e_toggle();
	_delay_ms(1);		/* delay, busy flag can't be checked here */

	/* now configure for 4bit mode */
	LCD_DATA0_PORT &= ~_BV(LCD_DATA0_PIN);	// LCD_FUNCTION_4BIT_1LINE>>4
	lcd_e_toggle();
	_delay_ms(1);		/* some displays need this additional delay */

	lcd_command(LCD_FUNCTION_DEFAULT);	/* function set: display lines  */

	lcd_command(LCD_DISP_OFF);	/* display off                  */
	lcd_clrscr();		/* display clear                */
	lcd_command(LCD_MODE_DEFAULT);	/* set entry mode               */
	lcd_command(dispAttr);	/* display/cursor control       */

}

void lcd_puts(const char *s)
{
	register char c;

	while ((c = *s++)) {
		lcd_putc(c);
	}

}

void lcd_puts_p(const char *progmem_s)
{
	register char c;

	while ((c = pgm_read_byte(progmem_s++))) {
		lcd_putc(c);
	}

}

void lcd_putc(char c)
{
	lcd_waitbusy();
	lcd_write(c, 1);
}

void lcd_gotoxy(unsigned int x, unsigned int y)
{
#if LCD_LINES==1
	lcd_command((1 << LCD_DDRAM) + LCD_START_LINE1 + x);
#endif
#if LCD_LINES==2
	if (y == 0)
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE1 + x);
	else
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE2 + x);
#endif
#if LCD_LINES==4
	if (y == 0)
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE1 + x);
	else if (y == 1)
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE2 + x);
	else if (y == 2)
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE3 + x);
	else			/* y==3 */
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE4 + x);
#endif

}
