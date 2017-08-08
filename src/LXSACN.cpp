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
	if ( _using_htp ) {
		free(_dmx_buffer_a);
		free(_dmx_buffer_b);
		free(_dmx_buffer_c);
	}
}

void  LXSACN::initialize  ( uint8_t* b ) {
	if ( b == 0 ) {
		// create buffer
		_packet_buffer = (uint8_t*) malloc(SACN_BUFFER_MAX);
		_owns_buffer = 1;
	} else {
		// external buffer.  Size MUST be >=SACN_BUFFER_MAX
		_packet_buffer = b;
		_owns_buffer = 0;
	}
	
    //zero buffer and CID
    memset(_packet_buffer, 0, SACN_BUFFER_MAX);
    memset(_dmx_sender_id, 0, SACN_CID_LENGTH);
    memset(_dmx_sender_id_b, 0, SACN_CID_LENGTH);
    
    _using_htp    = 0;
    _dmx_buffer_a = 0;
    _dmx_buffer_b = 0;
    _dmx_buffer_c = 0;
    
    _dmx_slots = 0;
    _dmx_slots_a = 0;
    _dmx_slots_b = 0;
    _universe = 1;                    // NOTE: unlike Art-Net, sACN universes begin at 1
    _sequence = 1;
}

uint8_t  LXSACN::universe ( void ) {
	return _universe;
}

void LXSACN::setUniverse ( uint8_t u ) {
	_universe = u;
}

void LXSACN::enableHTP() {
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
	// not enough memory on these to allocate these buffers
#else
	if ( ! _using_htp ) {
		_dmx_buffer_a = (uint8_t*) malloc(DMX_UNIVERSE_SIZE);
		_dmx_buffer_b = (uint8_t*) malloc(DMX_UNIVERSE_SIZE);
		_dmx_buffer_c = (uint8_t*) malloc(DMX_UNIVERSE_SIZE);
		for(int i=0; i<DMX_UNIVERSE_SIZE; i++) {
		   _dmx_buffer_a[i] = 0;
		   _dmx_buffer_b[i] = 0;
		   _dmx_buffer_c[i] = 0;
		}
		
		_using_htp = 1;
	}
#endif
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

uint8_t LXSACN::getHTPSlot ( int slot ) {
	return _dmx_buffer_c[slot-1];
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

uint8_t LXSACN::readDMXPacket ( UDP* eUDP ) {
   if ( readSACNPacket(eUDP) ) {
   	if ( startCode() == 0 ) {
   		return RESULT_DMX_RECEIVED;
   	}
   }	
   return 0;
}

uint8_t LXSACN::readDMXPacketContents ( UDP* eUDP, int packetSize ) {
   if ( packetSize > 0 ) {
		if ( parse_root_layer(packetSize) ) {
			if ( startCode() == 0 ) {
				return RESULT_DMX_RECEIVED;
			}
		}
   }
   return RESULT_NONE;
}

uint16_t LXSACN::readSACNPacket ( UDP* eUDP ) {
   uint16_t packetSize = eUDP->parsePacket();
   if ( packetSize ) {
      packetSize = eUDP->read(_packet_buffer, SACN_BUFFER_MAX);
      return parse_root_layer(packetSize);
   }
   return 0;
}

void LXSACN::sendDMX( UDP* eUDP, IPAddress to_ip ) {
   for (int n=0; n<126; n++) {
    	_packet_buffer[n] = 0;		// zero outside layers & start code
    }
    //ACN root layer
   _packet_buffer[0] = 0;
   _packet_buffer[1] = 0x10;
   strcpy((char*)&_packet_buffer[4], "ASC-E1.17");
   uint16_t fplusl = _dmx_slots + 110 + 0x7000;
   _packet_buffer[16] = fplusl >> 8;
   _packet_buffer[17] = fplusl & 0xff;
   _packet_buffer[21] = 0x04;

	//CID (UUID)
	//fd32aedc-7b94-11e7-bb31-be2e44b06b34
   _packet_buffer[22] = 0xfd;
   _packet_buffer[23] = 0x32;
   _packet_buffer[24] = 0xae;
   _packet_buffer[25] = 0xdc;
   _packet_buffer[26] = 0x7b;
   _packet_buffer[27] = 0x94;
   _packet_buffer[28] = 0x11;
   _packet_buffer[29] = 0xe7;
   _packet_buffer[30] = 0xbb;
   _packet_buffer[31] = 0x31;
   _packet_buffer[32] = 0xbe;
   _packet_buffer[33] = 0x2e;
   _packet_buffer[34] = 0x44;
   _packet_buffer[35] = 0xb0;
   _packet_buffer[36] = 0x6b;
   _packet_buffer[37] = 0x34;
   
   //ACN framing layer
   fplusl = _dmx_slots + 88 + 0x7000;
   _packet_buffer[38] = fplusl >> 8;
   _packet_buffer[39] = fplusl & 0xff;
   _packet_buffer[43] = 0x02;
   strcpy((char*)&_packet_buffer[44], "Arduino");
   _packet_buffer[108] = 100;	//priority
   if ( _sequence == 0 ) {
     _sequence = 1;
   } else {
     _sequence++;
   }
   _packet_buffer[111] = _sequence;
   _packet_buffer[113] = _universe >> 8;
   _packet_buffer[114] = _universe & 0xff;
   
   //ACN DMP layer
   fplusl = _dmx_slots + 11 + 0x7000;
   _packet_buffer[115] = fplusl >> 8;
   _packet_buffer[116] = fplusl & 0xff;
   _packet_buffer[117] = 0x02;
   _packet_buffer[118] = 0xa1;
   _packet_buffer[122] = 0x01;
   fplusl = _dmx_slots + 1;	//plus 1 byte for start code
   _packet_buffer[123] = fplusl >> 8;
   _packet_buffer[124] = fplusl & 0xFF;
   
   //assume dmx data has been set
   // _packet_buffer[125] is start code (0)
   eUDP->beginPacket(to_ip, SACN_PORT);
   eUDP->write(_packet_buffer, _dmx_slots + 126);
   eUDP->endPacket();
}

uint16_t LXSACN::parse_root_layer( int size ) {
  if ( ! _using_htp ) {
   	_dmx_slots = 0;		//read into packet buffer which doubles as DMX now invalid until confirmed
  }
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
        if (( _packet_buffer[112] == 0) && ( _packet_buffer[114] == _universe )) {	// implementation has 255 universe limit
          return parse_dmp_layer( tsize );       					
        }					// [112] options flags non-zero if preview or universe terminated
     }
   }
   return 0;
}

uint16_t LXSACN::parse_dmp_layer( uint16_t size ) {
  uint16_t tsize = size - 77;
  if ( checkFlagsAndLength(&_packet_buffer[115], tsize) ) {  // dmp pdu length
    if ( _packet_buffer[117] == 0x02 ) {                     // Set Property
      if ( _packet_buffer[118] == 0xa1 ) {                   // address and data format
        if ( _using_htp ) {
           uint16_t slots = _packet_buffer[124];      // if same sender, good dmx!
			  slots += _packet_buffer[123] << 8;
           copyCIDifEmpty(_dmx_sender_id);
           if ( checkCID(_dmx_sender_id) ) {
             _dmx_slots_a  = slots;
				 if ( _dmx_slots_a > _dmx_slots_b ) {
					_dmx_slots = _dmx_slots_a;
				 } else {
					_dmx_slots = _dmx_slots_b;
				 }
				 int di;
				 int dc = _dmx_slots;
				 int dt = 125 + 1;
				 for (di=0; di<dc; di++) {
					 _dmx_buffer_a[di] = _packet_buffer[dt+di];
					if ( _dmx_buffer_a[di] > _dmx_buffer_b[di] ) {
						_dmx_buffer_c[di] = _dmx_buffer_a[di];
					} else {
						_dmx_buffer_c[di] = _dmx_buffer_b[di];
					}
				 }			//for
				 return 1;
           } else {  // not matching first CID
              copyCIDifEmpty(_dmx_sender_id_b);
              if ( checkCID(_dmx_sender_id_b) ) {
                 _dmx_slots_b  = slots;
					  if ( _dmx_slots_a > _dmx_slots_b ) {
						 _dmx_slots = _dmx_slots_a;
					  } else {
						 _dmx_slots = _dmx_slots_b;
					  }

					  int di;
					  int dc = _dmx_slots;
					  int dt = 125 + 1;
					  for (di=0; di<dc; di++) {
						 _dmx_buffer_b[di] = _packet_buffer[dt+di];
						 if ( _dmx_buffer_a[di] > _dmx_buffer_b[di] ) {
							_dmx_buffer_c[di] = _dmx_buffer_a[di];
						 } else {
							_dmx_buffer_c[di] = _dmx_buffer_b[di];
						 }
					  }	//for
					  return 1;
				  }	//matched second CID
           }		//not first CID
        } else {	//not _using_htp
			  copyCIDifEmpty(_dmx_sender_id);
			  if ( checkCID(_dmx_sender_id) ) {
			    uint16_t slots = _packet_buffer[124];      // if same sender, good dmx!
			    slots += _packet_buffer[123] << 8;
			    _dmx_slots = slots;
			    return 1;
			  }
        }
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

uint8_t LXSACN::checkCID(uint8_t* cid) {
  for(int k=0; k<SACN_CID_LENGTH; k++) {
	 if ( cid[k] != _packet_buffer[k+22] ) {
		return 0;
	 } 
  }
  return 1;
}

void LXSACN::copyCIDifEmpty(uint8_t* cid) {
  if ( cid[0] == 0  ) {			
    for(int k=0; k<SACN_CID_LENGTH; k++) {
      cid[k] = _packet_buffer[k+22];
    }
  }
}
