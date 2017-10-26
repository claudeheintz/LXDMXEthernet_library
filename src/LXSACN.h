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
#include <inttypes.h>
#include "LXDMXEthernet.h"

#define SACN_PORT 0x15C0
#define SACN_BUFFER_MAX 638
#define SACN_PRIORITY_OFFSET 108
#define SACN_ADDRESS_OFFSET 125
#define SACN_CID_LENGTH 16
#define SLOTS_AND_START_CODE 513

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
/*!
 * @brief enables double buffering of received DMX data, merging from two sources
 * @discussion enableHTP allocates three 512 byte data buffers A, B, and Merged.
                         when sACN DMX is received, the data is copied into A or b
                         based on the IP address of the sender.  The highest level
                         for each slot is written to the merged HTP buffer.
                         Read the data from the HTP buffer using getHTPSlot(n).
                         enableHTP() is not available on an ATmega168, ATmega328, or
                         ATmega328P due to RAM size.
 */
   void    enableHTP();

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
 * @brief get level data from slot/address/channel when merge/double buffering is enabled
 * @discussion You must call enableHTP() once after the constructor before using getHTPSlot()
 * @param slot 1 to 512
 * @return level for slot (0-255)
 */  
   uint8_t  getHTPSlot      ( int slot );
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
 * @param eUDP UDP* object to be used for getting UDP packet
 * @return 1 if packet contains dmx
 */    
   uint8_t  readDMXPacket  ( UDP* eUDP );
   
 /*!
 * @brief read contents of packet from _packet_buffer
 * @discussion _packet_buffer should already contain packet payload when this is called
 * @param eUDP UDP*
 * @param packetSize size of received packet
 * @return 1 if packet contains dmx
 */      
   virtual uint8_t readDMXPacketContents (UDP* eUDP, int packetSize );
   
 /*!
 * @brief process packet, reading it into _packet_buffer
 * @param eUDP UDP*
 * @return number of dmx slots read or 0 if not dmx/invalid
 */
   uint16_t readSACNPacket ( UDP* eUDP );
 /*!
 * @brief send sACN E1.31 packet for dmx output from network
 * @param eUDP UDP* object to be used for sending UDP packet
 * @param to_ip target address
 */  
   void     sendDMX        ( UDP* eUDP, IPAddress to_ip );


void clearDMXOutput ( void );
   
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
/// cid of first sender of an E 1.31 DMX packet (subsequent senders ignored unless using htp)
  	uint8_t _dmx_sender_id[16];
  	
/*!
* @brief indicates dmx data is double buffered
* @discussion  In order to support HTPmerge, 3 buffers to hold DMX data must be allocated
               this is done by calling enableHTP()
*/
	uint8_t   _using_htp;

/// buffer that holds dmx data received from first source
  	uint8_t*  _dmx_buffer_a;
/// buffer that holds dmx data received from other source
  	uint8_t*  _dmx_buffer_b;
/// composite HTP buffer
  	uint8_t*  _dmx_buffer_c;
/// number of slots/address/channels
  	uint16_t  _dmx_slots_a;
/// number of slots/address/channels
  	uint16_t  _dmx_slots_b;
/// cid of second sender of an E 1.31 DMX packet
  	uint8_t _dmx_sender_id_b[16];
/// priority of primary sender (higher than or equal to secondary)	
  	uint8_t   _priority_a;
/// priority of second sender (only equal to primary, less is ignored, higher becomes primary)	  	
  	uint8_t   _priority_b;
/// timestamp of last packet received from primary sender 	
  	long      _last_packet_a;
/// timestamp of last packet received from second sender 
  	long      _last_packet_b;

/*!
* @brief checks the buffer for the sACN header and root layer size
*/  	
  	uint16_t  parse_root_layer    ( int size );
/*!
* @brief checks the buffer for the sACN header and root layer size
*/  
  	uint16_t  parse_framing_layer ( uint16_t size );	
/*!
* @brief dmp layer is where DMX data is located
*/  
  	uint16_t  parse_dmp_layer     ( uint16_t size );
/*!
* @brief utility for checking integrity of ACN packet
*/
  	uint8_t   checkFlagsAndLength ( uint8_t* flb, uint16_t size );
/*!
* @brief utility for matching CID contained in packet
*/
  	uint8_t   checkCID            ( uint8_t* cid );
/*!
* @brief copies CID from packet to array (if empty)
*/
  	void      copyCIDifEmpty      ( uint8_t* cid );
  	
/*!
* @brief initialize data structures
*/
   void  initialize  ( uint8_t* b );
   
 /*!
 * @brief clear "b" dmx buffer and sender CID
 */ 
   void clearDMXSourceB( void );
   
};

#endif // ifndef LXSACNDMX_H
