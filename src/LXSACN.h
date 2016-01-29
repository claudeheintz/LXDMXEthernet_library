/* LXSACN.h
   Copyright 2015 by Claude Heintz Design
   Copyright 2015-2015 by Claude Heintz Design
   see LXDMXEthernet.h for license
   
   sACN E 1.31 is a public standard published by the PLASA technical standards program
   http://tsp.plasa.org/tsp/documents/published_docs.php
*/

#ifndef LXSACNDMX_H
#define LXSACNDMX_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include <inttypes.h>
#include "LXDMXEthernet.h"

#define SACN_PORT 0x15C0
#define SACN_BUFFER_MAX 638
#define SACN_ADDRESS_OFFSET 125
#define SACN_CID_LENGTH 16

/*!
* @class LXSACN
* @abstract
           LXSACN partially implements E1.31, lightweight streaming protocol for transport of DMX512 using ACN
           http://tsp.plasa.org/tsp/documents/published_docs.php
           
           	LXSACN is primarily a node implementation.  It supports output of a single universe
           	of DMX data from the network.  It does not support merge and will only accept
           	packets from the first source from which it receives an E1.31 DMX packet.
*/
class LXSACN : public LXDMXEthernet {

  public:
/*!
* @brief constructor for LXSACN
*/  
	LXSACN  ( void );
/*!
* @brief constructor for LXSACN with external buffer fro UDP packets
* @param external packet buffer
*/  
	LXSACN ( uint8_t* buffer );
/*!
* @brief destructor for LXSACN  (frees packet buffer if allocated with constructor)
*/  	
   ~LXSACN ( void );

/*!
* @brief UDP port used by protocol
*/   
   uint16_t dmxPort ( void ) { return SACN_PORT; }

/*!
* @brief universe for sending and receiving dmx
* @discussion First universe is one for sACN E1.31.  Range is 1-255 in this implementation, 1-32767 in full E1.31.
* @return universe 1-255
*/ 
   uint8_t universe      ( void );
/*!
* @brief set universe for sending and receiving
* @discussion First universe one for sACN E1.31.  Range is 1-255 in this implementation, 1-32767 in full E1.31.
* @param u universe 1-255
*/
   void    setUniverse   ( uint8_t u );

 /*
 * @brief number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @return number of slots/addresses/channels
 */    
   int  numberOfSlots    ( void );
 /*!
 * @brief set number of slots (aka addresses or channels)
 * @discussion Should be minimum of ~24 depending on actual output speed.  Max of 512.
 * @param n number of slots 1 to 512
 */  
   void setNumberOfSlots ( int n );
 /*!
 * @brief get level data from slot/address/channel
 * @param slot 1 to 512
 * @return level for slot (0-255)
 */  
   uint8_t  getSlot      ( int slot );
 /*!
 * @brief set level data (0-255) for slot/address/channel
 * @param slot 1 to 512
 * @param level 0 to 255
 */  
   void     setSlot      ( int slot, uint8_t value );
/*!
* @brief dmx start code (set to zero for standard dmx)
* @return dmx start code
*/
   uint8_t  startCode    ( void );
/*!
* @brief sets dmx start code for outgoing packets
* @param value start code (set to zero for standard dmx)
*/
   void     setStartCode ( uint8_t value );
 /*!
 * @brief direct pointer to dmx buffer uint8_t[]
 * @return uint8_t* to dmx data buffer
 */
   uint8_t* dmxData      ( void );

 /*!
 * @brief read UDP packet
 * @param eUDP EthernetUDP object to be used for getting UDP packet
 * @return 1 if packet contains dmx
 */    
   uint8_t  readDMXPacket  ( EthernetUDP eUDP );
   
 /*!
 * @brief read contents of packet from _packet_buffer
 * @discussion _packet_buffer should already contain packet payload when this is called
 * @param eUDP EthernetUDP
 * @param packetSize size of received packet
 * @return 1 if packet contains dmx
 */      
   virtual uint8_t readDMXPacketContents (EthernetUDP eUDP, uint16_t packetSize );
   
 /*!
 * @brief process packet, reading it into _packet_buffer
 * @param eUDP EthernetUDP
 * @return number of dmx slots read or 0 if not dmx/invalid
 */
   uint16_t readSACNPacket ( EthernetUDP eUDP );
 /*!
 * @brief send sACN E1.31 packet for dmx output from network
 * @param eUDP EthernetUDP object to be used for sending UDP packet
 * @param to_ip target address
 */  
   void     sendDMX        ( EthernetUDP eUDP, IPAddress to_ip );

   
  private:
/*!
* @brief buffer that holds contents of incoming or outgoing packet
* @discussion There is no double buffer for dmx data.
*             readSACNPacket fills the buffer with the payload of the incoming packet.
*             Previous dmx data is invalidated.
*/
  	uint8_t*   _packet_buffer;
/*!
* @brief indicates was created by constructor
*/
	uint8_t   _owns_buffer;
/// number of slots/address/channels
  	int       _dmx_slots;
/// universe 1-255 in this implementation
  	uint16_t  _universe;
/// sequence number for sending sACN DMX packets
  	uint8_t   _sequence;
/// cid of first sender of an E 1.31 DMX packet (subsequent senders ignored)
  	uint8_t _dmx_sender_id[16];

/*!
* @brief checks the buffer for the sACN header and root layer size
*/  	
  	uint16_t  parse_root_layer    ( uint16_t size );
/*!
* @brief checks the buffer for the sACN header and root layer size
*/  
  	uint16_t  parse_framing_layer ( uint16_t size );	
  	uint16_t  parse_dmp_layer     ( uint16_t size );
  	uint8_t   checkFlagsAndLength ( uint8_t* flb, uint16_t size );
  	
/*!
* @brief initialize data structures
*/
   void  initialize  ( uint8_t* b );
};

#endif // ifndef LXSACNDMX_H
