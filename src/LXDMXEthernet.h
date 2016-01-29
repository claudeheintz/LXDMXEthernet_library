/* LXDMXEthernet.h
   Copyright 2015-2016 by Claude Heintz Design
   
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

#ifndef LXDMXETHERNET_H
#define LXDMXETHERNET_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include <inttypes.h>

#define RESULT_NONE 0
#define RESULT_DMX_RECEIVED 1
#define RESULT_PACKET_COMPLETE 2

/*!   
@class LXDMXEthernet
@abstract
   LXDMXEthernet encapsulates functionality for sending and receiving DMX over ethernet.
   It is a virtual class with concrete subclasses LXArtNet and LXSACN which specifically
   implement the Artistic Licence Art-Net and PLASA sACN 1.31 protocols.
   
   Note:  LXDMXEthernet_library requires
          Ethernet library used with the original Arduino Ethernet Shield
          For the Ethernet v2 shield, use LXDMXEthernet2_library along with Ethernet2 Library
          
          For multicast, EthernetUDP.h and EthernetUD2.cpp in the Ethernet library
          must be modified to add the beginMulticast method.
          See the code at the bottom of LXDMXEthernet.h
*/

class LXDMXEthernet {

  public:
/*!
* @brief UDP port used by protocol
*/
   virtual uint16_t dmxPort      ( void );

/*!
* @brief universe for sending and receiving dmx
* @discussion First universe is zero for Art-Net and one for sACN E1.31.
* @return universe 0/1-255
*/
   virtual uint8_t universe      ( void );
/*!
* @brief set universe for sending and receiving
* @discussion First universe is zero for Art-Net and one for sACN E1.31.
* @param u universe 0/1-255
*/
   virtual void    setUniverse   ( uint8_t u );
 
 /*!
 * @brief number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @return number of slots/addresses/channels
 */  
   virtual int  numberOfSlots    ( void );
 /*!
 * @brief set number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @param slots 1 to 512
 */  
   virtual void setNumberOfSlots ( int n );
 /*!
 * @brief get level data from slot/address/channel
 * @param slot 1 to 512
 * @return level for slot (0-255)
 */  
   virtual uint8_t  getSlot      ( int slot );
 /*!
 * @brief set level data (0-255) for slot/address/channel
 * @param slot 1 to 512
 * @param level 0 to 255
 */  
   virtual void     setSlot      ( int slot, uint8_t value );
 /*!
 * @brief direct pointer to dmx buffer uint8_t[]
 * @return uint8_t* to dmx data buffer
 */  
   virtual uint8_t* dmxData      ( void );

 /*!
 * @brief read UDP packet
 * @return 1 if packet contains dmx
 */   
   virtual uint8_t readDMXPacket ( EthernetUDP eUDP );
   
 /*!
 * @brief read contents of packet from _packet_buffer
 * @discussion _packet_buffer should already contain packet payload when this is called
 * @param eUDP EthernetUDP
 * @param packetSize size of received packet
 * @return 1 if packet contains dmx
 */      
   virtual uint8_t readDMXPacketContents (EthernetUDP eUDP, uint16_t packetSize );
   
/*!
 * @brief send the contents of the _packet_buffer to the address to_ip
 */
   virtual void    sendDMX       ( EthernetUDP eUDP, IPAddress to_ip );
};

#endif // ifndef LXDMXETHERNET_H


/*
@code
******************  multicast methods added to Ethernet2 library *******************

see  https://github.com/aallan/Arduino/blob/3811729f82ef05f3ae43341022e7b65a92d333a2/libraries/Ethernet/EthernetUdp.cpp

1) Locate the Ethernet library in the IDE
   Mac OS X Control-click Arduino.app and select "Show Package Contents" from the popup menu
            Navigate to Arduino.app/Contents/Java/libraries/Ethernet
   
2) Duplicate the Ethernet library in your sketchbook folder (~/Documents/Arduino/libraries)
   Add the required method as follows:

3)  In EthernetUDPh add the beginMulticast method declaration:
...
  EthernetUDP();  // Constructor
  virtual uint8_t begin(uint16_t);	// initialize, start listening on specified port. Returns 1 if successful, 0 if there are no sockets available to use
  uint8_t beginMulticast(IPAddress ip, uint16_t port);  //############## added to standard library
...

4)  In EthernetUDP.cpp add the method body:

uint8_t EthernetUDP::beginMulticast(IPAddress ip, uint16_t port) {
    if (_sock != MAX_SOCK_NUM)
        return 0;
    
    for (int i = 0; i < MAX_SOCK_NUM; i++) {
        uint8_t s = w5500.readSnSR(i);
        if (s == SnSR::CLOSED || s == SnSR::FIN_WAIT) {
            _sock = i;
            break;
        }
    }
    
    if (_sock == MAX_SOCK_NUM)
        return 0;
    
    
    // Calculate MAC address from Multicast IP Address
    byte mac[] = {  0x01, 0x00, 0x5E, 0x00, 0x00, 0x00 };
    
    mac[3] = ip[1] & 0x7F;
    mac[4] = ip[2];
    mac[5] = ip[3];
    
    w5500.writeSnDIPR(_sock, rawIPAddress(ip));   //239.255.0.1
    w5500.writeSnDPORT(_sock, port);
    w5500.writeSnDHAR(_sock,mac);
    
    _remaining = 0;
    
    socket(_sock, SnMR::UDP, port, SnMR::MULTI);
    
    return 1;
}

**************************************************************************************
*/
