/*
 * main.c
 *
 *  Created on: Jun 16, 2019
 *      Author: Aaron N. Rudolph
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#define ENABLE_PIN PC0
#define DATA_PIN PC1

#define BAUD 31250
#define BAUD_UBRR F_CPU/16/BAUD-1

uint8_t rx_buf[3]; // Holds 1 MIDI packet
uint8_t rx_cnt = 0; // RX byte count


/*
 * MIDI-Specific
 */
void noteOn(uint8_t pitch)
{
	// Zero out PORTD[7..2], shift in pitch
	PORTD = (PORTD & 0b00000001) | (pitch << 1);
	// Set DATA pin
	PORTC |= (1 << DATA_PIN);
	// Set ENABLE pin (Active low)
	PORTC &= ~(1 << ENABLE_PIN);

	// Reset ENABLE pin
	PORTC |= (1 << ENABLE_PIN);
	// Reset DATA pin
	PORTC &= ~(1 << DATA_PIN);

}

void noteOff(uint8_t pitch)
{
	// Zero out PORTD[7..2], shift in pitch
	PORTD = (PORTD & 0b00000001) | (pitch << 1);

	// Set ENABLE pin (Active low)
	PORTC &= ~(1 << ENABLE_PIN);

	// Reset ENABLE pin
	PORTC |= (1 << ENABLE_PIN);

}

/*
 * UART-Specific
 */

void uart_init()
{
	// 31,250 Baud
	UBRR0 = BAUD_UBRR;

	// RX only, Interrupt enabled
	UCSR0B = (1 << RXEN0) | (1 << RXCIE0);
	/* Set frame format: 8data, 1stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

ISR(USART_RX_vect)
{
	uint8_t inByte = UDR0;
	if(inByte & 0x80)
	{
		rx_cnt = 0;
	}
	rx_buf[rx_cnt] = inByte;
	rx_cnt ++;


	if(rx_cnt > 2)
	{
		uint8_t pitch = rx_buf[1];
		switch(rx_buf[0] >> 4)
		{
		case 0x9:
			if(rx_buf[2] > 0)
			{
				noteOn(pitch);
			}
			else
			{
				noteOff(pitch);
			}
			break;
		case 0x8:
			noteOff(pitch);
			break;
		}
	}
}

/*
 * Initialization
 */

int main(void)
{
	// Set pins
	DDRC |= (1 << ENABLE_PIN) | (1 << DATA_PIN);
	// Enable Interrupts
	sei();
	// Set all to output except RX pin
	DDRD = 0b11111110;
	// Init UART
	uart_init();

	while(1);
}
