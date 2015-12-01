/* LXArduinoDMX.h
   Copyright 2015 by Claude Heintz Design
   All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of LXDMXEthernet nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LXArduinoDMX_H
#define LXArduinoDMX_H

#include <Arduino.h>
#include <inttypes.h>

#define DMX_MIN_SLOTS 24
#define DMX_MAX_SLOTS 512

typedef void (*LXRecvCallback)(int);

class LXArduinoDMXOutput {

  public:
	LXArduinoDMXOutput ( void );
	/*!
    * @param pin used to control driver chip's DE (data enable) line, HIGH for output
    * @param slots number of slots aka channels or addresses (~24-512)
   */
	LXArduinoDMXOutput ( uint8_t pin, uint16_t slots  );
    ~LXArduinoDMXOutput ( void );
    
   /*!
    * @brief starts interrupt that continuously sends DMX output
    * @discussion Sets up baud rate, bits and parity, 
    *             sets globals accessed in ISR, 
    *             enables transmission and tx interrupt.
   */
   void start( void );
   /*!
    * disables transmission and tx interrupt
   */
	void stop( void );
	
	/*!
	 * @brief Sets the number of slots (aka addresses or channels) sent per DMX frame.
	 * @discussion defaults to 512 or DMX_MAX_SLOTS and should be no less DMX_MIN_SLOTS slots.  
	 *             The DMX standard specifies min break to break time no less than 1024 usecs.  
	 *             At 44 usecs per slot ~= 24
	 * @param slot the highest slot number (~24 to 512)
	*/
	void setMaxSlots (int slot);
	/*!
	 * @brief Sets the output value of a slot
	 * @param slot number of the slot/address/channel (1-512)
	 * @param value level (0-255)
	*/
   void setSlot (int slot, uint8_t value);
   /*!
    * @brief provides direct access to data array
    * @return pointer to dmx array
   */
   uint8_t* dmxData(void);
    
  private:
   /*!
    * @brief true when ISR is enabled
   */
  	uint8_t  _interrupt_status;
  	/*!
    * @brief Array of dmx data including start code
   */
  	uint8_t  _dmxData[DMX_MAX_SLOTS+1];
};

class LXArduinoDMXInput {

  public:
	LXArduinoDMXInput ( void );
	/*!
	 * @param pin used to control driver chip's DE (data enable) line, LOW for input
	*/
	LXArduinoDMXInput ( uint8_t pin );
   ~LXArduinoDMXInput ( void );
   
   /*!
    * @brief starts interrupt that continuously reads DMX data
    * @discussion sets up baud rate, bits and parity, 
    *             sets globals accessed in ISR, 
    *             enables transmission and tx interrupt
   */
   void start( void );
   /*!
    * @brief disables receive and rx interrupt
   */
	void stop( void );

   /*!
    * @brief reads the value of a slot/address/channel
    * @discussion NOTE: Data is not double buffered.  
    *                   So a complete single frame is not guaranteed.  
    *                   The ISR continuously reads the next frame into the buffer
    * @return level (0-255)
   */
   uint8_t getSlot (int slot);
   /*!
    * @brief provides direct access to data array
    * @return pointer to dmx array
   */
   uint8_t* dmxData(void);
   
   /*!
    * @brief Function called when DMX frame has been read
    * @discussion Sets a pointer to a function that is called
    *             on the break after a DMX frame has been received.  
    *             Whatever happens in this function should be quick!  
    *             Best used to set a flag that is polled outside of ISR for available data.
   */
   void setDataReceivedCallback(LXRecvCallback callback);
    
  private:
   /*!
    * @brief True when ISR is enabled
   */
  	uint8_t  _interrupt_status;
   /*!
    * @brief Array of dmx data including start code
   */
  	uint8_t  _dmxData[DMX_MAX_SLOTS+1];
};

#endif // ifndef LXArduinoDMX_H