/* LXArtNet.h
   Copyright 2015-2015 by Claude Heintz Design
   see LXDMXEthernet.h for license

	Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
*/

#ifndef LXARTNET_H
#define LXARTNET_H

#include <Arduino.h>
#include <inttypes.h>
#include "LXDMXEthernet.h"

#define ARTNET_PORT 0x1936
#define ARTNET_BUFFER_MAX 530
#define ARTNET_REPLY_SIZE 240
#define ARTNET_TOD_PKT_SIZE	1228
#define ARTNET_RDM_PKT_SIZE 281
#define ARTNET_ADDRESS_OFFSET 17

#define ARTNET_ART_POLL 0x2000
#define ARTNET_ART_POLL_REPLY 0x2100
#define ARTNET_ART_DMX 0x5000
#define ARTNET_ART_ADDRESS 0x6000
#define ARTNET_ART_TOD_REQUEST	0x8000
#define ARTNET_ART_TOD_CONTROL	0x8200
#define ARTNET_ART_RDM			0x8300
#define ARTNET_NOP 0


typedef void (*ArtNetReceiveCallback)(void);
typedef void (*ArtNetRDMRecvCallback)(uint8_t* pdata);

/*!
@class LXArtNet
@abstract 
   LXArtNet partially implements the Art-Net Ethernet Communication Standard.

	LXArtNet is primarily a node implementation.  It supports output of a single universe
   of DMX data from the network.  It does not support merge and will only accept
   packets from the first IP address from which it receives an ArtDMX packet.
   This can be reset by sending an ArtAddress cancel merge command.
   
   When reading packets, LXArtNet will automatically respond to ArtPoll packets.
   Depending on the constructor used, it will either broadcast the reply or will
   reply directly to the sender of the poll.

   http://www.artisticlicence.com
*/
class LXArtNet : public LXDMXEthernet {

  public:
/*!
* @brief constructor with address used for ArtPollReply
* @param address sent in ArtPollReply
*/   
   LXArtNet  ( IPAddress address );
/*!
* @brief constructor creates broadcast address for Poll Reply
* @param address sent in ArtPollReply
* @param subnet_mask used to set broadcast address
*/  
	LXArtNet  ( IPAddress address, IPAddress subnet_mask );
/*!
* @brief constructor creates instance with external buffer for UDP packet
* @param address sent in ArtPollReply
* @param subnet_mask used to set broadcast address
* @param buffer external buffer for UDP packets
*/ 
	LXArtNet ( IPAddress address, IPAddress subnet_mask, uint8_t* buffer );
	
	
/*!
* @brief destructor for LXArtNet (frees packet buffer if allocated with constructor)
*/ 
   ~LXArtNet ( void );

/*!
* @brief UDP port used by protocol
*/   
   uint16_t dmxPort ( void ) { return ARTNET_PORT; }

/*!
* @brief universe for sending and receiving dmx
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* @return universe 0-255
*/   
   uint8_t universe           ( void );
/*!
* @brief set universe for sending and receiving
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* @param u universe 0-255
*/
   void    setUniverse        ( uint8_t u );
/*!
* @brief set subnet/universe for sending and receiving
* @discussion First universe is zero for Art-Net.  Sets separate nibbles: high/subnet, low/universe.
* @param s subnet 0-16
* @param u universe 0-16
*/
   void    setSubnetUniverse  ( uint8_t s, uint8_t u );
/*!
* @brief set universe for sending and receiving
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* 0x7f is no change, otherwise if high bit is set, low nibble becomes universe (subnet remains the same)
* @param u universe 0-16 + flag 0x80
*/
   void    setUniverseAddress ( uint8_t u );
/*!
* @brief set subnet for sending and receiving
* @discussion First universe is zero for Art-Net.  High nibble is subnet, low nibble is universe.
* 0x7f is no change, otherwise if high bit is set, low nibble becomes subnet (universe remains the same)
* @param s subnet 0-16 + flag 0x80
*/
   void    setSubnetAddress   ( uint8_t s );
/*!
* @brief set net for sending and receiving
* @discussion Net portion of Port-Address, Net+Subnet+Universe (15bits, 7+4+4)
* requires bit 7 to be set to change from ArtAddress packet other 6-0 bits are the net setting
* @param s subnet 0-127 + flag 0x80
*/
   void    setNetAddress   ( uint8_t s );
/*!
 * @brief enables double buffering of received DMX data, merging from two sources
 * @discussion enableHTP allocates three 512 byte data buffers A, B, and Merged.
                         when ArtDMX is received, the data is copied into A or b
                         based on the IP address of the sender.  The highest level
                         for each slot is written to the merged HTP buffer.
                         Read the data from the HTP buffer using getHTPSlot(n).
                         enableHTP() is not available on an ATmega168, ATmega328, or
                         ATmega328P due to RAM size.
 */
   void    enableHTP();

 /*!
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
 * @param value level 0 to 255
 */  
   void     setSlot      ( int slot, uint8_t value );

 /*!
 * @brief direct pointer to dmx buffer uint8_t[]
 * @return uint8_t* to dmx data buffer
 */ 
   uint8_t* dmxData      ( void );
/*!
 * @brief direct pointer to poll reply packet contents
 * @return uint8_t* to poll reply packet contents
 */ 
   uint8_t* replyData      ( void );
   
/*!
 * @brief direct pointer to poll reply packet contents
 * @return uint8_t* to poll reply packet contents
 */ 
   void     setNodeName    ( const char* name );

 /*!
 * @brief read UDP packet
 * @param eUDP pointer to UDP object to be used for getting UDP packet
 * @return 1 if packet contains dmx
 */    
   uint8_t  readDMXPacket       ( UDP* eUDP );
 /*!
 * @brief read contents of packet from _packet_buffer
 * @discussion _packet_buffer should already contain packet payload when this is called
 * @param wUDP WiFiUDP
 * @param packetSize size of received packet
 * @return 1 if packet contains dmx
 */      
   uint8_t readDMXPacketContents ( UDP* eUDP, int packetSize );
 /*!
 * @brief process packet, reading it into _packet_buffer
 * @param eUDP UDP* (used for Poll Reply if applicable)
 * @return Art-Net opcode of packet
 */
   uint16_t readArtNetPacket    ( UDP* eUDP );
 /*!
 * @brief read contents of packet from _packet_buffer
 * @param wUDP WiFiUDP (used for Poll Reply if applicable)
 * @param packetSize size of received packet
 * @return Art-Net opcode of packet
 */   
   uint16_t readArtNetPacketContents ( UDP* eUDP, int packetSize );
   
 /*!
 * @brief read dmx data from ArtDMX packet
 * @param wUDP WiFiUDP (used for Poll Reply if applicable)
 * @param slots number of slots to read
 * @return opcode (
 */   
   uint16_t readArtDMX ( UDP* eUDP, uint16_t slots, int packetSize );
 /*!
 * @brief send Art-Net ArtDMX packet for dmx output from network
 * @param eUDP UDP* to be used for sending UDP packet
 * @param to_ip target address
 */    
   void     sendDMX             ( UDP* eUDP, IPAddress to_ip );
 /*!
 * @brief send ArtPoll Reply packet for dmx output from network
 * @discussion If broadcast address is defined by passing subnet to constructor, reply is broadcast
 *             Otherwise, reply is unicast to remoteIP belonging to the sender of the poll
 * @param eUDP pointer to UDP object to be used for sending UDP packet
 */  
   void     send_art_poll_reply ( UDP* eUDP );
   
   /*!
 * @brief send ArtTOD packet for dmx output from network
 * @discussion 
 * @param wUDP		pointer to UDP object to be used for sending UDP packet
 * @param todata	pointer to TOD array of 6 byte UIDs
 * @param ucount	number of 6 byte UIDs contained in todata
 */    
   void     send_art_tod ( UDP* wUDP, uint8_t* todata, uint8_t ucount );
   
/*!
 * @brief send ArtRDM packet
 * @discussion 
 * @param wUDP		pointer to UDP object to be used for sending UDP packet
 * @param rdmdata	pointer to rdm packet to be sent
 * @param toa		IPAddress to send UDP ArtRDM packet    
 */ 
   void send_art_rdm ( UDP* wUDP, uint8_t* rdmdata, IPAddress toa );
   
/*!
 * @brief function callback when ArtTODRequest is received
 * @discussion callback pointer is to integer
 *             to distinguish between ARTNET_ART_TOD_REQUEST *0
 *             and ARTNET_ART_TOD_CONTROL *1
*/
   void setArtTodRequestCallback(ArtNetRDMRecvCallback callback);
   
   /*!
	* @brief function callback when ArtRDM is received
	* @discussion callback has pointer to RDM payload
	*/
   void setArtRDMCallback(ArtNetRDMRecvCallback callback);
   
  private:
/*!
* @brief buffer that holds contents of incoming or outgoing packet
* @discussion To minimize memory footprint, the default is no double buffer for dmx data.
*             readArtNetPacket fills the buffer with the payload of the incoming packet.
*             Previous dmx data is invalidated.
*/
  	uint8_t*   _packet_buffer;
  	
/// indicates the _packet_buffer was allocated by the constructor and is private.
	uint8_t   _owns_buffer;

/// array that holds contents of outgoing ArtPollReply packet
	static uint8_t _reply_buffer[ARTNET_REPLY_SIZE];


/// number of slots/address/channels
  	uint16_t  _dmx_slots;
/// high nibble subnet, low nibble universe
  	uint8_t   _universe;
/// 7bit net value of Port-Address, Net + Subnet + Universe
  	uint8_t   _net;
/// sequence number for sending ArtDMX packets
  	uint8_t   _sequence;

/// address included in poll replies 	
  	IPAddress _my_address;
/// if subnet is supplied in constructor, holds address to broadcast poll replies
  	IPAddress _broadcast_address;
/// first sender of an ArtDMX packet (subsequent senders ignored until cancelMerge)
  	IPAddress _dmx_sender;

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
/// second sender of an ArtDMX packet
  	IPAddress _dmx_sender_b;
  	
  	/*!
    * @brief Pointer to art tod request callback
   */
  	ArtNetRDMRecvCallback _art_tod_req_callback;
  	
  	/*!
    * @brief Pointer to art RDM packet received callback function
   */
  	ArtNetRDMRecvCallback _art_rdm_callback;

/*!
* @brief checks packet for "Art-Net" header
* @return opcode if Art-Net packet
*/
  	uint16_t  parse_header        ( void );	
/*!
* @brief utility for parsing ArtAddress packets
* @return opcode in case command changes dmx data
*/
   uint16_t  parse_art_address   ( void );
   
/*!
* @brief utility for parsing ArtTODRequest packets
*/     
   uint16_t parse_art_tod_request( UDP* wUDP );
   uint16_t parse_art_tod_control( UDP* wUDP );
   
/*!
* @brief utility for parsing ArtRDM packets
*/     
   uint16_t parse_art_rdm( UDP* wUDP );
   
/*!
* @brief initialize data structures
*/
   void  initialize  ( uint8_t* b );
   
/*!
* @brief initialize poll reply buffer
*/
   void  initializePollReply  ( void );
   
};

#endif // ifndef LXARTNET_H
