/**************************************************************************/
/*!
    @file     LXUSARTDMX.cpp
    @author   Claude Heintz
    @license  BSD (see LXUSARTDMX.h)
    @copyright 2015-2016 by Claude Heintz

    DMX Driver for Arduino using USART.

    @section  HISTORY

    v1.0   First release
    v1.1 - Consolidated input and output into single class with defined LXSerialDMX instance
*/
/**************************************************************************/
#include "pins_arduino.h"
#include "LXArduinoDMXUSART.h"
#include <inttypes.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>

LXUSARTDMX LXSerialDMX;

// ***************** define registers, flags and interrupts  ****************

// processors with single UART such AT328 processor (Uno)
// should not define USE_UART_1
// Uncomment the following to use UART1:
//#define USE_UART_1

// USE_UART_1 should be defined for Teensy 2.0++, Leonardo, Yun
#if defined(__AVR_AT90USB1286__) || defined (__AVR_ATmega32U4__)
   #define USE_UART_1
#endif


#if !defined(USE_UART_1)    // definitions for UART0
							// for AT328 see datasheet pg 194

    #define LXUCSRA UCSR0A		// USART register A
    #define LXTXC   TXC0			// tx buffer empty
    #define LXUDRE  UDRE0			// data ready
    #define LXFE    FE0				// frame error
    #define LXU2X   U2X0			// double speed

    #define LXUCSRB UCSR0B		// USART register B
    #define LXRXCIE RXCIE0			// rx interrupt enable bit
    #define LXTXCIE TXCIE0			// tx interrupt enable bit
    #define LXRXEN  RXEN0			// rx enable bit
    #define LXTXEN  TXEN0			// tx enable bit

    #define LXUCSRC UCSR0C		// USART register C
    #define LXUSBS0  USBS0			// stop bits
    #define LXUCSZ0 UCSZ00			// length
    #define LXUPM0  UPM00			// parity
    #define LXUCSRRH UBRR0H		// USART baud rate register msb
    #define LXUCSRRL UBRR0L		// USARTbaud rate register msb
    #define LXUDR   UDR0		// USART data register tx/rx

    #define LXUSART_RX_vect  USART_RX_vect		// RX ISR
    #define LXUSART_TX_vect  USART_TX_vect		// TX ISR

    #define BIT_FRAME_ERROR (1<<LXFE)
    #define BIT_2X_SPEED (1<<LXU2X)
    #define FORMAT_8N2 (3<<LXUCSZ0) | (1<<LXUSBS0)
    #define FORMAT_8E1 (3<<LXUCSZ0) | (2<<LXUPM0)
    #define BIT_TX_ENABLE  (1<<LXTXEN)
    #define BIT_TX_ISR_ENABLE (1<<LXTXCIE)
    #define BIT_RX_ENABLE  (1<<LXRXEN)
    #define BIT_RX_ISR_ENABLE (1<<LXRXCIE)
  
// <- end definitions for not defined(USE_UART_1)

#elif defined(USE_UART_1)   // definitions for UART1

    #define LXUCSRA UCSR1A		// USART register A
    #define LXTXC   TXC1			// tx buffer empty
    #define LXUDRE  UDRE1			// data ready
    #define LXFE    FE1				// frame error
    #define LXU2X   U2X1            // double speed

    #define LXUCSRB UCSR1B		// USART register B
    #define LXRXCIE RXCIE1			// rx interrupt enable bit
    #define LXTXCIE TXCIE1			// tx interrupt enable bit
    #define LXRXEN  RXEN1			// rx enable bit
    #define LXTXEN  TXEN1			// tx enable bit

    #define LXUCSRC UCSR1C		// USART register C
    #define LXUSBS0  USBS1			// stop bits
    #define LXUCSZ0 UCSZ10			// length
    #define LXUPM0  UPM10			// parity
    #define LXUCSRRH UBRR1H		// USART baud rate register msb
    #define LXUCSRRL UBRR1L		// USARTbaud rate register msb
    #define LXUDR   UDR1		// USART data register tx/rx

    #define LXUSART_RX_vect  USART1_RX_vect		// RX ISR
    #define LXUSART_TX_vect  USART1_TX_vect		// TX ISR

    #define BIT_FRAME_ERROR (1<<LXFE)
    #define BIT_2X_SPEED (1<<LXU2X)
    #define FORMAT_8N2 (3<<LXUCSZ0) | (1<<LXUSBS0)
    #define FORMAT_8E1 (3<<LXUCSZ0) | (2<<LXUPM0)
    #define BIT_TX_ENABLE  (1<<LXTXEN)
    #define BIT_TX_ISR_ENABLE (1<<LXTXCIE)
    #define BIT_RX_ENABLE  (1<<LXRXEN)
    #define BIT_RX_ISR_ENABLE (1<<LXRXCIE)


#endif // <- end definitions for defined(USE_UART_1)

	//***** baud rate defines
    #define F_CLK 				16000000UL
    #define DMX_DATA_BAUD		250000
    #define DMX_BREAK_BAUD 	 	99900
    //99900

    //***** states indicate current position in DMX stream
    #define DMX_STATE_BREAK 0
    #define DMX_STATE_START 1
    #define DMX_STATE_DATA 2
    #define DMX_STATE_IDLE 3

	//***** status is if interrupts are enabled and IO is active
    #define ISR_DISABLED 			0
    #define ISR_OUTPUT_ENABLED 	1
    #define ISR_INPUT_ENABLED 	2

// **************************** global data (can be accessed in ISR)  ***************

uint8_t*  _shared_dmx_data;
uint8_t   _shared_dmx_state;
uint16_t  _shared_dmx_slot;
uint16_t  _shared_max_slots = DMX_MIN_SLOTS;
LXRecvCallback _shared_receive_callback = NULL;


//************************************************************************************
// ************************  LXUSARTDMXOutput member functions  ********************

LXUSARTDMX::LXUSARTDMX ( void ) {
	_direction_pin = DIRECTION_PIN_NOT_USED;	//optional
	_shared_max_slots = DMX_MAX_SLOTS;
	_interrupt_status = ISR_DISABLED;
	
	//zero buffer including _dmxData[0] which is start code
    for (int n=0; n<DMX_MAX_SLOTS+1; n++) {
    	_dmxData[n] = 0;
    }
}


LXUSARTDMX::~LXUSARTDMX ( void ) {
    stop();
    _shared_dmx_data = NULL;
    _shared_receive_callback = NULL;
}

//  ***** start *****
//  sets up baud rate, bits and parity
//  sets globals accessed in ISR
//  enables transmission and tx interrupt

void LXUSARTDMX::startOutput ( void ) {
	if ( _direction_pin != DIRECTION_PIN_NOT_USED ) {
		digitalWrite(_direction_pin, HIGH);
	}
	if ( _interrupt_status == ISR_INPUT_ENABLED ) {
		stop();
	}
	if ( _interrupt_status == ISR_DISABLED ) {	//prevent messing up sequence if already started...
		LXUCSRRH = (unsigned char)(((F_CLK + DMX_DATA_BAUD * 8L) / (DMX_DATA_BAUD * 16L) - 1)>>8);
		LXUCSRRL = (unsigned char) ((F_CLK + DMX_DATA_BAUD * 8L) / (DMX_DATA_BAUD * 16L) - 1);
		LXUCSRA &= ~BIT_2X_SPEED;

		LXUDR = 0x0;     			//USART send register  
		_shared_dmx_data = dmxData();                 
		_shared_dmx_state = DMX_STATE_BREAK;

		LXUCSRC = FORMAT_8N2; 					//set length && stopbits (no parity)
		LXUCSRB |= BIT_TX_ENABLE | BIT_TX_ISR_ENABLE;  //enable tx and tx interrupt
		_interrupt_status = ISR_OUTPUT_ENABLED;
	}
}

//  ***** start *****
//  sets up baud rate, bits and parity
//  sets globals accessed in ISR
//  enables transmission and tx interrupt

void LXUSARTDMX::startInput ( void ) {
	if ( _direction_pin != DIRECTION_PIN_NOT_USED ) {
		digitalWrite(_direction_pin, LOW);
	}
	if ( _interrupt_status == ISR_OUTPUT_ENABLED ) {
		stop();
	}
	if ( _interrupt_status == ISR_DISABLED ) {	//prevent messing up sequence if already started...
		LXUCSRRH = (unsigned char)(((F_CLK + DMX_DATA_BAUD * 8L) / (DMX_DATA_BAUD * 16L) - 1)>>8);
		LXUCSRRL = (unsigned char) ((F_CLK + DMX_DATA_BAUD * 8L) / (DMX_DATA_BAUD * 16L) - 1);
		LXUCSRA &= ~BIT_2X_SPEED;

		_shared_dmx_data = dmxData();                 
		_shared_dmx_state = DMX_STATE_IDLE;
		_shared_dmx_slot = 0;
	
		LXUCSRC = FORMAT_8N2; 					//set length && stopbits (no parity)
		LXUCSRB |= BIT_RX_ENABLE | BIT_RX_ISR_ENABLE;  //enable tx and tx interrupt
		_interrupt_status = ISR_INPUT_ENABLED;
	}
}

//  ***** stop *****
//  disables interrupts

void LXUSARTDMX::stop ( void ) { 
	if ( _interrupt_status == ISR_OUTPUT_ENABLED ) {
		LXUCSRB &= ~BIT_TX_ISR_ENABLE;  							//disable tx interrupt
		LXUCSRB &= ~BIT_TX_ENABLE;     							//disable tx enable
	} else if ( _interrupt_status == ISR_INPUT_ENABLED ) {
		LXUCSRB &= ~BIT_RX_ISR_ENABLE;  							//disable rx interrupt
		LXUCSRB &= ~BIT_RX_ENABLE;     							//disable rx enable	
	}
	_interrupt_status = ISR_DISABLED;
}

void LXUSARTDMX::setDirectionPin( uint8_t pin ) {
	_direction_pin = pin;
	pinMode(_direction_pin, OUTPUT);
}

//  ***** setMaxSlots *****
//  sets the number of slots sent per DMX frame
//  defaults to 512 or DMX_MAX_SLOTS
//  should be no less DMX_MIN_SLOTS slots
//  the DMX standard specifies min break to break time no less than 1024 usecs
//  at 44 usecs per slot ~= 24

void LXUSARTDMX::setMaxSlots (int slots) {
	_shared_max_slots = max(slots, DMX_MIN_SLOTS);
}

//  ***** getSlot *****
//  reads the value of a slot
//  see buffering note for ISR below 

uint8_t LXUSARTDMX::getSlot (int slot) {
	return _dmxData[slot];
}

//  ***** setSlot *****
//  sets the output value of a slot

void LXUSARTDMX::setSlot (int slot, uint8_t value) {
	_dmxData[slot] = value;
}

//  ***** dmxData *****
//  pointer to data buffer

uint8_t* LXUSARTDMX::dmxData(void) {
	return &_dmxData[0];
}

//  ***** setDataReceivedCallback *****
//  sets pointer to function that is called
//  on the break after a frame has been received
//  whatever happens in this function should be quick
//  ie set a flag so that processing of the received data happens
//  outside of the ISR.


void LXUSARTDMX::setDataReceivedCallback(LXRecvCallback callback) {
	_shared_receive_callback = callback;
}


//************************************************************************************
// ************************ TX ISR (transmit interrupt service routine)  *************
//
// this routine is called when USART transmission is complete
// what this does is to send the next byte
// when that byte is done being sent, the ISR is called again
// and the cycle repeats...
// until _shared_max_slots worth of bytes have been sent on succesive triggers of the ISR
// and then on the next ISR...
// the break/mark after break is sent at a different speed
// and then on the next ISR...
// the start code is sent
// and then on the next ISR...
// the next data byte is sent
// and the cycle repeats...


ISR (LXUSART_TX_vect) {
	switch ( _shared_dmx_state ) {
	   
		case DMX_STATE_BREAK:
			// set the slower baud rate and send the break
			LXUCSRRH = (unsigned char)(((F_CLK + DMX_BREAK_BAUD * 8L) / (DMX_BREAK_BAUD * 16L) - 1)>>8);
			LXUCSRRL = (unsigned char) ((F_CLK + DMX_BREAK_BAUD * 8L) / (DMX_BREAK_BAUD * 16L) - 1);
			LXUCSRA &= ~BIT_2X_SPEED;
			LXUCSRC = FORMAT_8E1;
			_shared_dmx_state = DMX_STATE_START;
			LXUDR = 0x0;
			break;		// <- DMX_STATE_BREAK
			
		case DMX_STATE_START:
			// set the baud to full speed and send the start code
			LXUCSRRH = (unsigned char)(((F_CLK + DMX_DATA_BAUD * 8L) / (DMX_DATA_BAUD * 16L) - 1)>>8);
			LXUCSRRL = (unsigned char) ((F_CLK + DMX_DATA_BAUD * 8L) / (DMX_DATA_BAUD * 16L) - 1);
			LXUCSRA &= ~BIT_2X_SPEED;
			LXUCSRC = FORMAT_8N2;
			_shared_dmx_slot = 0;	
			LXUDR = _shared_dmx_data[_shared_dmx_slot++];	//send next slot (start code)
			_shared_dmx_state = DMX_STATE_DATA;
			break;		// <- DMX_STATE_START
		
		case DMX_STATE_DATA:
			// send the next data byte until the end is reached
			LXUDR = _shared_dmx_data[_shared_dmx_slot++];	//send next slot
			if ( _shared_dmx_slot > _shared_max_slots ) {
				_shared_dmx_state = DMX_STATE_BREAK;
			}
			break;		// <- DMX_STATE_DATA
	}
}

//***********************************************************************************
// ************************ RX ISR (receive interrupt service routine)  *************
//
// this routine is called when USART receives data
// wait for break:  if have previously read data call callback function
// then on next receive:  check start code
// then on next receive:  read data until done (in which case idle)
//
//  NOTE: data is not double buffered
//  so a complete single frame is not guaranteed
//  the ISR will continue to read the next frame into the buffer

ISR (LXUSART_RX_vect) {
	uint8_t status_register = LXUCSRA;
  	uint8_t incoming_byte = LXUDR;
	
	if ( status_register & BIT_FRAME_ERROR ) {
		_shared_dmx_state = DMX_STATE_BREAK;
		if ( _shared_dmx_slot > 0 ) {
			if ( _shared_receive_callback != NULL ) {
				_shared_receive_callback(_shared_dmx_slot);
			}
		}
		_shared_dmx_slot = 0;
		return;
	}
	
	switch ( _shared_dmx_state ) {
	
		case DMX_STATE_BREAK:
			if ( incoming_byte == 0 ) {						//start code == zero (DMX)
				_shared_dmx_state = DMX_STATE_DATA;
				_shared_dmx_slot = 1;
			} else {
				_shared_dmx_state = DMX_STATE_IDLE;
			}
			break;
			
		case DMX_STATE_DATA:
			_shared_dmx_data[_shared_dmx_slot++] = incoming_byte;
			if ( _shared_dmx_slot > DMX_MAX_SLOTS ) {
				_shared_dmx_state = DMX_STATE_IDLE;			// go to idle, wait for next break
			}
			break;
	}
}