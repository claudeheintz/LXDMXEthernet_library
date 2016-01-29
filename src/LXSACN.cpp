/* LXSACN.cpp
   Copyright 2015-2015 by Claude Heintz Design
   see LXDMXEthernet.h for license
   
   LXSACN partially implements E1.31,
   Lightweight streaming protocol for transport of DMX512 using ACN
   
   sACN E 1.31 is a public standard published by the PLASA technical standards program
   http://tsp.plasa.org/tsp/documents/published_docs.php
*/

#include "LXSACN.h"

LXSACN::LXSACN ( void )
{
	initialize(0);
}

LXSACN::LXSACN ( uint8_t* buffer )
{
	initialize(buffer);
}

LXSACN::~LXSACN ( void )
{
	if ( _owns_buffer ) {		// if we created this buffer, then free the memory
		free(_packet_buffer);
	}
}

void  LXSACN::initialize  ( uint8_t* b ) {
	if ( b == 0 ) {
		// create buffer
		_packet_buffer = (uint8_t*) malloc(SACN_BUFFER_MAX);
		_owns_buffer = 1;
	} else {
		// external buffer
		_packet_buffer = b;
		_owns_buffer = 0;
	}
	
	//zero buffer including _dmx_data[0] which is start code
    for (int n=0; n<SACN_BUFFER_MAX; n++) {
    	_packet_buffer[n] = 0;
    	if ( n < SACN_CID_LENGTH ) {
    		 _dmx_sender_id[n] = 0;
    	}
    }
    
    _dmx_slots = 0;
    _universe = 1;                    // NOTE: unlike Art-Net, sACN universes begin at 1
    _sequence = 1;
}

uint8_t  LXSACN::universe ( void ) {
	return _universe;
}

void LXSACN::setUniverse ( uint8_t u ) {
	_universe = u;
}

int  LXSACN::numberOfSlots ( void ) {
	return _dmx_slots;
}

void LXSACN::setNumberOfSlots ( int n ) {
	_dmx_slots = n;
}

uint8_t LXSACN::getSlot ( int slot ) {
	return _packet_buffer[SACN_ADDRESS_OFFSET+slot];
}

void LXSACN::setSlot ( int slot, uint8_t value ) {
	_packet_buffer[SACN_ADDRESS_OFFSET+slot] = value;
}

uint8_t LXSACN::startCode ( void ) {
	return _packet_buffer[SACN_ADDRESS_OFFSET];
}

void LXSACN::setStartCode ( uint8_t value ) {
	_packet_buffer[SACN_ADDRESS_OFFSET] = value;
}

uint8_t* LXSACN::dmxData( void ) {
	return &_packet_buffer[SACN_ADDRESS_OFFSET];
}

uint8_t LXSACN::readDMXPacket ( EthernetUDP eUDP ) {
   if ( readSACNPacket(eUDP) > 0 ) {
   	return ( startCode() == 0 );
   }	
   return 0;
}

uint8_t LXSACN::readDMXPacketContents ( EthernetUDP eUDP, uint16_t packetSize ) {
	if ( parse_root_layer(packetSize) > 0 ) {
   	if ( startCode() == 0 ) {
   		return RESULT_DMX_RECEIVED;
   	}
   }	
   return RESULT_NONE;
}

uint16_t LXSACN::readSACNPacket ( EthernetUDP eUDP ) {
   _dmx_slots = 0;
   uint16_t packetSize = eUDP.parsePacket();
   if ( packetSize ) {
      packetSize = eUDP.read(_packet_buffer, SACN_BUFFER_MAX);
      _dmx_slots = parse_root_layer(packetSize);
   }
   return _dmx_slots;
}

void LXSACN::sendDMX ( EthernetUDP eUDP, IPAddress to_ip ) {
   for (int n=0; n<126; n++) {
    	_packet_buffer[n] = 0;		// zero outside layers & start code
    }
   _packet_buffer[0] = 0;
   _packet_buffer[1] = 0x10;
   _packet_buffer[4] = 'A';
   _packet_buffer[5] = 'S';
   _packet_buffer[6] = 'C';
   _packet_buffer[7] = '-';
   _packet_buffer[8] = 'E';
   _packet_buffer[9] = '1';
   _packet_buffer[10] = '.';
   _packet_buffer[11] = '1';
   _packet_buffer[12] = '7';
   uint16_t fplusl = _dmx_slots + 110 + 0x7000;
   _packet_buffer[16] = fplusl >> 8;
   _packet_buffer[17] = fplusl & 0xff;
   _packet_buffer[21] = 0x04;
   fplusl = _dmx_slots + 88 + 0x7000;
   _packet_buffer[38] = fplusl >> 8;
   _packet_buffer[39] = fplusl & 0xff;
   _packet_buffer[43] = 0x02;
   _packet_buffer[44] = 'A';
   _packet_buffer[45] = 'r';
   _packet_buffer[46] = 'd';
   _packet_buffer[47] = 'u';
   _packet_buffer[48] = 'i';
   _packet_buffer[49] = 'n';
   _packet_buffer[50] = 'o';
   _packet_buffer[108] = 100;	//priority
   if ( _sequence == 0 ) {
     _sequence = 1;
   } else {
     _sequence++;
   }
   _packet_buffer[111] = _sequence;
   _packet_buffer[113] = _universe >> 8;
   _packet_buffer[114] = _universe & 0xff;
   fplusl = _dmx_slots + 11 + 0x7000;
   _packet_buffer[115] = fplusl >> 8;
   _packet_buffer[116] = fplusl & 0xff;
   _packet_buffer[117] = 0x02;
   _packet_buffer[118] = 0xa1;
   _packet_buffer[122] = 0x01;
   fplusl = _dmx_slots + 1;
   _packet_buffer[123] = fplusl >> 8;
   _packet_buffer[124] = fplusl & 0xFF;
   //assume dmx data has been set
   eUDP.beginPacket(to_ip, SACN_PORT);
   eUDP.write(_packet_buffer, _dmx_slots + 126);
   eUDP.endPacket();
}

uint16_t LXSACN::parse_root_layer( uint16_t size ) {
  if  ( _packet_buffer[1] == 0x10 ) {									//preamble size
    if ( strcmp((const char*)&_packet_buffer[4], "ASC-E1.17") == 0 ) {
      uint16_t tsize = size - 16;
      if ( checkFlagsAndLength(&_packet_buffer[16], tsize) ) { // root pdu length
        if ( _packet_buffer[21] == 0x04 ) {							// vector RLP is 1.31 data
          return parse_framing_layer( tsize );
        }
      }
    }       // ACN packet identifier
  }			// preamble size
  return 0;
}

uint16_t LXSACN::parse_framing_layer( uint16_t size ) {
   uint16_t tsize = size - 22;
   if ( checkFlagsAndLength(&_packet_buffer[38], tsize) ) {     // framing pdu length
     if ( _packet_buffer[43] == 0x02 ) {                        // vector dmp is 1.31
        if ( _packet_buffer[114] == _universe ) {// implementation has 255 universe limit
          return parse_dmp_layer( tsize );       
        }
     }
   }
   return 0;
}

uint16_t LXSACN::parse_dmp_layer( uint16_t size ) {
  uint16_t tsize = size - 77;
  if ( checkFlagsAndLength(&_packet_buffer[115], tsize) ) {  // dmp pdu length
    if ( _packet_buffer[117] == 0x02 ) {                     // Set Property
      if ( _packet_buffer[118] == 0xa1 ) {                   // address and data format
        if ( _dmx_sender_id[0] == 0  ) {			
          for(int k=0; k<SACN_CID_LENGTH; k++) {		 // if _dmx_sender_id is not set
            _dmx_sender_id[k] = _packet_buffer[k+22];  // set it to id of this packet
          }
        }
        for(int k=0; k<SACN_CID_LENGTH; k++) {
          if ( _dmx_sender_id[k] != _packet_buffer[k+22] ) {
            return 0;
          } 
        }
        uint16_t slots = _packet_buffer[124];      // if same sender, good dmx!
        slots += _packet_buffer[123] << 8;
        return slots;
      }
    }
  }
  return 0;
}

//  utility for checking 2 byte:  flags (high nibble == 0x7) && 12 bit length

uint8_t LXSACN::checkFlagsAndLength( uint8_t* flb, uint16_t size ) {
	if ( ( flb[0] & 0xF0 ) == 0x70 ) {
	   uint16_t pdu_length = flb[1];
		pdu_length += ((flb[0] & 0x0f) << 8);
		if ( ( pdu_length != 0 ) && ( size >= pdu_length ) ) {
		   return 1;
		}
	}
	return 0;
} 
