/**************************************************************************/
/*!
    @file     2Universes.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXEthernet.h)
    @copyright 2015-2016 by Claude Heintz All Rights Reserved

    Example using LXDMXEthernet_Library for output of Art-Net or E1.31 sACN from
    EEthernet connection to DMX
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  2 Universes may excede the memory available on ATmega328 boards
           such as Arduino Uno.
           
    @section  HISTORY

    v1.00 - First release
*/
/**************************************************************************/

#include <SPI.h>

/******** Important note about Ethernet library ********
   There are various ethernet shields that use differnt Wiznet chips w5100, w5200, w5500
   It is necessary to use an Ethernet library that supports the correct chip for your shield
   Perhaps the best library is the one by Paul Stoffregen which supports all three chips:
   
   https://github.com/PaulStoffregen/Ethernet

   The Paul Stoffregen version is much faster than the built-in Ethernet library and is
   neccessary if the shield receives more than a single universe of packets.
   
   The Ethernet Shield v2 uses a w5500 chip and will not work with the built-in Ethernet library
   The library manager does have an Ethernet2 library which supports the w5500.  To use this,
   uncomment the line to define ETHERNET_SHIELD_V2
*/
#if defined ( ETHERNET_SHIELD_V2 )
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#else
#include <Ethernet.h>
#include <EthernetUdp.h>
#endif

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <LXDMXEthernet.h>
#include <LXArtNet.h>
#include <LXSACN.h>

//*********************** defines ***********************

#define PIN 6
#define NUM_LEDS 14

//  Make choices here about protocol ( set to 1 to activate option )

#define USE_DHCP 0
#define USE_SACN 0

// dmx protocol interface for parsing packets (created in setup)
LXDMXEthernet* interface;
LXDMXEthernet* interfaceUniverse2;

// buffer large enough to contain incoming packet
uint8_t packetBuffer[SACN_BUFFER_MAX];

//*********************** globals ***********************

// network addresses
// IP address is defined assuming that the Arduino is connected to
// a Mac without a router and a self assigned ip
byte mac[] = { 0x00, 0x08, 0xDC, 0x4C, 0x29, 0x7E }; //00:08:DC Wiznet
IPAddress ip(169,254,100,100);
IPAddress gateway(169,254,1,1);
IPAddress subnet_mask(255,255,0,0);

// Adafruit_NeoPixel instance to drive the NeoPixels
Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP eUDP;

// sACN uses multicast, Art-Net uses broadcast. Both can be set to unicast (use_multicast = 0)
uint8_t use_multicast = 0;		//only single multicast address is supported in this example, will not work for 2 universes


//*********************** setup ***********************

void setup() {
  if ( USE_DHCP ) {                                          // Initialize Ethernet
    Ethernet.begin(mac);                                     // DHCP
  } else {
  		Ethernet.begin(mac, ip, gateway, gateway, subnet_mask);   // Static
  }
  
  if ( USE_SACN ) {                       // Initialize Interface (defaults to first universe)
    interface = new LXSACN(&packetBuffer[0]);
    interfaceUniverse2 = new LXSACN(&packetBuffer[0]);
    interfaceUniverse2->setUniverse(2);	         // for different universe, change this line and the multicast address below
  } else {
    interface = new LXArtNet(Ethernet.localIP(), Ethernet.subnetMask(), &packetBuffer[0]);
    interfaceUniverse2 = new LXArtNet(Ethernet.localIP(), Ethernet.subnetMask(), &packetBuffer[0]);
    ((LXArtNet*)interfaceUniverse2)->setSubnetUniverse(0, 1);  //for different subnet/universe, change this line
    use_multicast = 0;
  }

  if ( use_multicast ) {                  // Start listening for UDP on port
    eUDP.beginMulticast(IPAddress(239,255,0,1), interface->dmxPort());
  } else {
    eUDP.begin(interface->dmxPort());
  }

  //announce presence via Art-Net Poll Reply (only advertises one universe)
  if ( ! USE_DHCP ) {
     ((LXArtNet*)interface)->send_art_poll_reply(&eUDP);
  }
	pinMode(3, OUTPUT);
	pinMode(5, OUTPUT);
	
	ring.begin();                   // Initialize NeoPixel driver
  ring.show();
}

/************************************************************************

  The main loop checks for and reads packets from  UDP socket
  connection.  readDMXPacketContents() returns RESULT_DMX_RECEIVED when a DMX packet is received.
  If the first universe does not match, try the second

*************************************************************************/

void loop() {
  uint16_t packetSize = eUDP.parsePacket();
  if ( packetSize ) {
  	  packetSize = eUDP.read(packetBuffer, SACN_BUFFER_MAX);
	  uint8_t read_result = interface->readDMXPacketContents(&eUDP, packetSize);
	  uint8_t read_result2 = 0;

	  if ( read_result == RESULT_DMX_RECEIVED ) {
	     // edge case test universe 1, slot 1
	     analogWrite(3,interface->getSlot(1));
       ring.setPixelColor(1, interface->getSlot(1), interface->getSlot(2), interface->getSlot(3));
	     ring.show();
	  } else if ( read_result == RESULT_NONE ) {				// if not good dmx first universe (or art poll), try 2nd
	     read_result2 = interfaceUniverse2->readDMXPacketContents(&eUDP, packetSize);
	     if ( read_result2 == RESULT_DMX_RECEIVED ) {
	     		// edge case test 2nd universe, slot 512
		  		analogWrite(5,interfaceUniverse2->getSlot(512));
          ring.setPixelColor(8, interface->getSlot(510), interface->getSlot(511), interface->getSlot(512));
	        ring.show();
	     }
	  }
	}
}
