#include "fake_hw.h"

volatile uint8_t ADMUX;
volatile uint8_t ACSR;
volatile uint8_t ADCW;
volatile uint8_t ADC;
volatile uint8_t ADCL;
volatile uint8_t ADCH;
volatile uint8_t ADCSR;
volatile uint8_t ADCSRA;

volatile uint8_t DDRA;
volatile uint8_t DDRB;
volatile uint8_t DDRC;
volatile uint8_t DDRD;
volatile uint8_t DDRF;
volatile uint8_t DDRE;
volatile uint8_t DDRG;

volatile uint8_t EICRA;
volatile uint8_t EICRB;
volatile uint8_t EIFR;
volatile uint8_t EIMSK;
volatile uint8_t ETIFR;
volatile uint8_t ETIMSK;

volatile uint8_t TIFR;
volatile uint8_t TIMSK;

volatile uint8_t ASSR;
volatile uint8_t TCCR0;
volatile uint8_t TCCR2;

volatile uint8_t TCCR1A;
volatile uint8_t TCCR1B;
volatile uint8_t TCCR1C;
volatile uint8_t TCCR3A;
volatile uint8_t TCCR3B;
volatile uint8_t TCCR3C;

volatile uint8_t OCR0;
volatile uint8_t OCR2;
volatile uint8_t OCR1A;
volatile uint8_t OCR1AL;
volatile uint8_t OCR1AH;
volatile uint8_t OCR1B;
volatile uint8_t OCR1BL;
volatile uint8_t OCR1BH;
volatile uint8_t OCR1C;
volatile uint8_t OCR1CL;
volatile uint8_t OCR1CH;
volatile uint8_t OCR3A;
volatile uint8_t OCR3AL;
volatile uint8_t OCR3AH;
volatile uint8_t OCR3B;
volatile uint8_t OCR3BL;
volatile uint8_t OCR3BH;
volatile uint8_t OCR3C;
volatile uint8_t OCR3CL;
volatile uint8_t OCR3CH;

volatile uint8_t TCNT0;
volatile uint8_t TCNT1;
volatile uint8_t TCNT1L;
volatile uint8_t TCNT1H;
volatile uint8_t TCNT2;
volatile uint8_t TCNT3;
volatile uint8_t TCNT3L;
volatile uint8_t TCNT3H;

volatile uint8_t ICR1;
volatile uint8_t ICR1L;
volatile uint8_t ICR1H;
volatile uint8_t ICR3;
volatile uint8_t ICR3L;
volatile uint8_t ICR3H;

volatile uint8_t PINA;
volatile uint8_t PINB;
volatile uint8_t PINC;
volatile uint8_t PIND;
volatile uint8_t PINE;
volatile uint8_t PINF;
volatile uint8_t PING;

volatile uint8_t PORTA;
volatile uint8_t PORTB;
volatile uint8_t PORTC;
volatile uint8_t PORTD;
volatile uint8_t PORTE;
volatile uint8_t PORTF;
volatile uint8_t PORTG;

volatile uint8_t SPCR;
volatile uint8_t SPDR;
volatile uint8_t SPSR;

volatile uint8_t UBRR0H;
volatile uint8_t UBRR0L;
volatile uint8_t UBRRH;
volatile uint8_t UBRRL;
volatile uint8_t UBRR1L;
volatile uint8_t UBRR1H;
volatile uint8_t UCSR0A;
volatile uint8_t UCSR0B;
volatile uint8_t UCSR0C;
volatile uint8_t UCSRA;
volatile uint8_t UCSRB;
volatile uint8_t UCSRC;
volatile uint8_t UCSR1A;
volatile uint8_t UCSR1B;
volatile uint8_t UCSR1C;
volatile uint8_t UDR0;
volatile uint8_t UDR;
volatile uint8_t UDR1;

volatile uint8_t SFIOR;
volatile uint8_t WDTCR;
volatile uint8_t OCDR;
volatile uint8_t MCUSR;
volatile uint8_t MCUCSR;
volatile uint8_t MCUCR;
volatile uint8_t RAMPZ;
volatile uint8_t XDIV;
volatile uint8_t SPMCR;
volatile uint8_t SPMCSR;
volatile uint8_t XMCRB;
volatile uint8_t XMCRA;
volatile uint8_t OSCCAL;

volatile uint8_t TWBR;
volatile uint8_t TWSR;
volatile uint8_t TWAR;
volatile uint8_t TWDR;
volatile uint8_t TWCR;
