/* LXArtNet.cpp
   Copyright 2015-2015 by Claude Heintz Design
   see LXDMXEthernet.h for license
   
   LXArtNet partially implements the Art-Net Ethernet Communication Standard.
   http://www.artisticlicence.com
   
   LXArtNet supports capturing a single universe of DMX data
   from Art-Net packets read from UDP.
   LXArtNet will automatically respond to ArtPoll packets
   by sending a unicast reply directly to the poll.
   
   LXArtNet does not support merge and will only accept ArtDMX output packets 
   from the first IP address that it receives a packet from.
   This can be reset by sending an ArtAddress cancel merge command.
      
	Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
*/

#include "LXArtNet.h"

LXArtNet::LXArtNet ( IPAddress address )
{
	initialize(0);
	
    _my_address = address;
    _broadcast_address = INADDR_NONE;
    _dmx_sender = INADDR_NONE;
}

LXArtNet::LXArtNet ( IPAddress address, IPAddress subnet_mask )
{
	initialize(0);
	
    _my_address = address;
    uint32_t a = (uint32_t) address;
    uint32_t s = (uint32_t) subnet_mask;
    _broadcast_address = IPAddress(a | ~s);
    _dmx_sender = INADDR_NONE;
}

LXArtNet::LXArtNet ( IPAddress address, IPAddress subnet_mask, uint8_t* buffer )
{
	initialize(buffer);
    _my_address = address;
    uint32_t a = (uint32_t) address;
    uint32_t s = (uint32_t) subnet_mask;
    _broadcast_address = IPAddress(a | ~s);
    _dmx_sender = INADDR_NONE;
}

LXArtNet::~LXArtNet ( void )
{
    if ( _owns_buffer ) {		// if we created this buffer, then free the memory
		free(_packet_buffer);
	}
}

void  LXArtNet::initialize  ( uint8_t* b ) {
	if ( b == 0 ) {
		// create buffer
		_packet_buffer = (uint8_t*) malloc(ARTNET_BUFFER_MAX);
		_owns_buffer = 1;
	} else {
		// external buffer
		_packet_buffer = b;
		_owns_buffer = 0;
	}
	
	//zero buffer including _dmx_data[0] which is start code
    for (int n=0; n<ARTNET_BUFFER_MAX; n++) {
    	_packet_buffer[n] = 0;
    }
    
    _dmx_slots = 0;
    _universe = 0;
    _net = 0;
    _sequence = 1;
    initializePollReply();
}


uint8_t  LXArtNet::universe ( void ) {
	return _universe;
}

void LXArtNet::setUniverse ( uint8_t u ) {
	_universe = u;
}

void LXArtNet::setSubnetUniverse ( uint8_t s, uint8_t u ) {
   _universe = ((s & 0x0f) << 4) | ( u & 0x0f);
}

void LXArtNet::setUniverseAddress ( uint8_t u ) {
	if ( u != 0x7f ) {
	   if ( u & 0x80 ) {
	     _universe = (_universe & 0xf0) | (u & 0x07);
	   }
	}
}

void LXArtNet::setSubnetAddress ( uint8_t u ) {
	if ( u != 0x7f ) {
	   if ( u & 0x80 ) {
	     _universe = (_universe & 0x0f) | ((u & 0x07) << 4);
	   }
	}
}

void LXArtNet::setNetAddress ( uint8_t s ) {
	if ( s & 0x80 ) {
	  _net = (s & 0x7F);
	}
}

int  LXArtNet::numberOfSlots ( void ) {
	return _dmx_slots;
}

void LXArtNet::setNumberOfSlots ( int n ) {
	_dmx_slots = n;
}

uint8_t LXArtNet::getSlot ( int slot ) {
	return _packet_buffer[ARTNET_ADDRESS_OFFSET+slot];
}

void LXArtNet::setSlot ( int slot, uint8_t value ) {
	_packet_buffer[ARTNET_ADDRESS_OFFSET+slot] = value;
}

uint8_t* LXArtNet::dmxData( void ) {
	return &_packet_buffer[ARTNET_ADDRESS_OFFSET+1];
}

uint8_t* LXArtNet::replyData( void ) {
	return &_reply_buffer[0];
}

uint8_t LXArtNet::readDMXPacket ( EthernetUDP eUDP ) {
   uint16_t opcode = readArtNetPacket(eUDP);
   if ( opcode == ARTNET_ART_DMX ) {
   	return RESULT_DMX_RECEIVED;
   }
   return RESULT_NONE;
}

uint8_t LXArtNet::readDMXPacketContents ( EthernetUDP eUDP, uint16_t packetSize ) {
	uint16_t opcode = readArtNetPacketContents(eUDP, packetSize);
   if ( opcode == ARTNET_ART_DMX ) {
   	return RESULT_DMX_RECEIVED;
   }
   if ( opcode == ARTNET_ART_POLL ) {
   	return RESULT_PACKET_COMPLETE;
   }
   return RESULT_NONE;
}

/*
  attempts to read a packet from the supplied EthernetUDP object
  returns opcode
  sends ArtPollReply with IPAddress if packet is ArtPoll
  replies directly to sender unless reply_ip != INADDR_NONE allowing specification of broadcast
  only returns ARTNET_ART_DMX if packet contained dmx data for this universe
  Packet size checks that packet is >= expected size to allow zero termination or padding
*/
uint16_t LXArtNet::readArtNetPacket ( EthernetUDP eUDP ) {
	uint16_t opcode = ARTNET_NOP;
	uint16_t packetSize = eUDP.parsePacket();
	if ( packetSize ) {
		packetSize = eUDP.read(_packet_buffer, ARTNET_BUFFER_MAX);
		opcode = readArtNetPacketContents(eUDP, packetSize);
	}
	return opcode;
}

uint16_t LXArtNet::readArtNetPacketContents ( EthernetUDP eUDP, uint16_t packetSize ) {
   uint16_t opcode = ARTNET_NOP;
	
	_dmx_slots = 0;
	/* Buffer now may not contain dmx data for desired universe.
		After reading the packet into the buffer, check to make sure
		that it is an Art-Net packet and retrieve the opcode that
		tells what kind of message it is.                            */
	opcode = parse_header();
	switch ( opcode ) {
		case ARTNET_ART_DMX:
			// ignore protocol version [10] hi byte [11] lo byte sequence[12], physical[13]
			if (( _packet_buffer[14] == _universe ) && ( _packet_buffer[15] == _net )) {
				packetSize -= 18;
				uint16_t slots = _packet_buffer[17];
				slots += _packet_buffer[16] << 8;
				if ( packetSize >= slots ) {
					if ( (uint32_t)_dmx_sender == 0 ) {		//if first sender, remember address
						_dmx_sender = eUDP.remoteIP();
					}
					if ( _dmx_sender == eUDP.remoteIP() ) {
						_dmx_slots = slots;
					}	// matched sender
				}		// matched size
			}			// matched universe
			if ( _dmx_slots == 0 ) {	//only set >0 if all of above matched
				opcode = ARTNET_NOP;
			}
			break;
		case ARTNET_ART_ADDRESS:
			if (( packetSize >= 107 ) && ( _packet_buffer[11] >= 14 )) {  //protocol version [10] hi byte [11] lo byte
				opcode = parse_art_address();
				send_art_poll_reply( eUDP );
			}
			break;
		case ARTNET_ART_POLL:
			if (( packetSize >= 14 ) && ( _packet_buffer[11] >= 14 )) {
				send_art_poll_reply( eUDP );
			}
			break;
	}
   return opcode;
}

void LXArtNet::sendDMX ( EthernetUDP eUDP, IPAddress to_ip ) {
   _packet_buffer[0] = 'A';
   _packet_buffer[1] = 'r';
   _packet_buffer[2] = 't';
   _packet_buffer[3] = '-';
   _packet_buffer[4] = 'N';
   _packet_buffer[5] = 'e';
   _packet_buffer[6] = 't';
   _packet_buffer[7] = 0;
   _packet_buffer[8] = 0;        //op code lo-hi
   _packet_buffer[9] = 0x50;
   _packet_buffer[10] = 0;
   _packet_buffer[11] = 14;
   if ( _sequence == 0 ) {
     _sequence = 1;
   } else {
     _sequence++;
   }
   _packet_buffer[12] = _sequence;
   _packet_buffer[13] = 0;
   _packet_buffer[14] = _universe;
   _packet_buffer[15] = _net;
   _packet_buffer[16] = _dmx_slots >> 8;
   _packet_buffer[17] = _dmx_slots & 0xFF;
   //assume dmx data has been set
  
   eUDP.beginPacket(to_ip, ARTNET_PORT);
   eUDP.write(_packet_buffer, 18+_dmx_slots);
   eUDP.endPacket();
}

/*
  sends ArtDMX packet to EthernetUDP object's remoteIP if to_ip is not specified
  ( remoteIP is set when parsePacket() is called )
  includes my_ip as address of this node
*/
void LXArtNet::send_art_poll_reply( EthernetUDP eUDP ) {
	_reply_buffer[190] = _universe;
  
  IPAddress a = _broadcast_address;
  if ( a == INADDR_NONE ) {
    a = eUDP.remoteIP();   // reply directly if no broadcast address is supplied
  }
  eUDP.beginPacket(a, ARTNET_PORT);
  eUDP.write(_reply_buffer, ARTNET_REPLY_SIZE);
  eUDP.endPacket();
}

uint16_t LXArtNet::parse_header( void ) {
  if ( strcmp((const char*)_packet_buffer, "Art-Net") == 0 ) {
    return _packet_buffer[9] * 256 + _packet_buffer[8];  //opcode lo byte first
  }
  return ARTNET_NOP;
}

/*
  reads an ARTNET_ART_ADDRESS packet
  can set output universe
  can cancel merge which resets address of dmx sender
     (after first ArtDmx packet, only packets from the same sender are accepted
     until a cancel merge command is received)
*/
uint16_t LXArtNet::parse_art_address( void ) {
   setNetAddress(_packet_buffer[12]);
	//[14] to [31] short name <= 18 bytes
	//[32] to [95] long name  <= 64 bytes
	//[96][97][98][99]                  input universe   ch 1 to 4
	//[100][101][102][103]               output universe   ch 1 to 4
	setUniverseAddress(_packet_buffer[100]);
	//[102][103][104][105]                      subnet   ch 1 to 4
	setSubnetAddress(_packet_buffer[104]);
	//[105]                                   reserved
	uint8_t command = _packet_buffer[106]; // command
	switch ( command ) {
	   case 0x01:	//cancel merge: resets ip address used to identify dmx sender
	   	_dmx_sender = (uint32_t)0;
	   	break;
	   case 0x90:	//clear buffer
	   	_dmx_sender = (uint32_t)0;
	   	for(int j=18; j<ARTNET_BUFFER_MAX; j++) {
	   	   _packet_buffer[j] = 0;
	   	}
	   	_dmx_slots = 512;
	   	return ARTNET_ART_DMX;	// return ARTNET_ART_DMX so function calling readPacket
	   	   						   // knows there has been a change in levels
	   	break;
	}
	return ARTNET_ART_ADDRESS;
}

void  LXArtNet::initializePollReply  ( void ) {
	int i;
  for ( i = 0; i < ARTNET_REPLY_SIZE; i++ ) {
    _reply_buffer[i] = 0;
  }
  _reply_buffer[0] = 'A';
  _reply_buffer[1] = 'r';
  _reply_buffer[2] = 't';
  _reply_buffer[3] = '-';
  _reply_buffer[4] = 'N';
  _reply_buffer[5] = 'e';
  _reply_buffer[6] = 't';
  _reply_buffer[7] = 0;
  _reply_buffer[8] = 0;        // op code lo-hi
  _reply_buffer[9] = 0x21;
  _reply_buffer[10] = ((uint32_t)_my_address) & 0xff;      //ip address
  _reply_buffer[11] = ((uint32_t)_my_address) >> 8;
  _reply_buffer[12] = ((uint32_t)_my_address) >> 16;
  _reply_buffer[13] = ((uint32_t)_my_address) >>24;
  _reply_buffer[14] = 0x36;    // port lo first always 0x1936
  _reply_buffer[15] = 0x19;
  _reply_buffer[16] = 0;       // firmware hi-lo
  _reply_buffer[17] = 0;
  _reply_buffer[18] = 0;       // subnet hi-lo
  _reply_buffer[19] = 0;
  _reply_buffer[20] = 0;       // oem hi-lo
  _reply_buffer[21] = 0;
  _reply_buffer[22] = 0;       // ubea
  _reply_buffer[23] = 0;       // status
  _reply_buffer[24] = 0x50;    //     Mfg Code
  _reply_buffer[25] = 0x12;    //     seems DMX workshop reads these bytes backwards
  _reply_buffer[26] = 'A';     // short name
  _reply_buffer[27] = 'r';
  _reply_buffer[28] = 'd';
  _reply_buffer[29] = 'u';
  _reply_buffer[30] = 'i';
  _reply_buffer[31] = 'n';
  _reply_buffer[32] = 'o';
  _reply_buffer[33] = 'D';
  _reply_buffer[34] = 'M';
  _reply_buffer[35] = 'X';
  _reply_buffer[36] =  0;
  _reply_buffer[44] = 'A';     // long name
  _reply_buffer[45] = 'r';
  _reply_buffer[46] = 'd';
  _reply_buffer[47] = 'u';
  _reply_buffer[48] = 'i';
  _reply_buffer[49] = 'n';
  _reply_buffer[50] = 'o';
  _reply_buffer[51] = 'D';
  _reply_buffer[52] = 'M';
  _reply_buffer[53] = 'X';
  _reply_buffer[54] =  0;
  _reply_buffer[173] = 1;    // number of ports
  _reply_buffer[174] = 128;  // can output from network
  _reply_buffer[182] = 128;  //  good output... change if error
  _reply_buffer[190] = _universe;
}
