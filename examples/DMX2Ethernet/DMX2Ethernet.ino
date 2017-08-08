/**************************************************************************/
/*!
    @file     DMX2Ethernet.ino
    @author   Claude Heintz
    @license  BSD (see LXDMXEthernet.h)
    @copyright 2015-2017 by Claude Heintz

    Example using LXDMXEthernet_Library for input from DMX to Art-Net or E1.31 sACN
    
    Art-Net(TM) Designed by and Copyright Artistic Licence Holdings Ltd.
    sACN E 1.31 is a public standard published by the PLASA technical standards program

	See LXDMXEthernet.h or http://lx.claudeheintzdesign.com/opensource.html for license.
    
    @section  HISTORY

    v1.00 - First release
    
    
//*********************** includes ***********************/

#include "LXArduinoDMXUSART.h"

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

#define ETHERNET_SHIELD_V2 1

#if defined ( ETHERNET_SHIELD_V2 )
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <utility/w5500.h>
#else
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <utility/w5100.h>
#endif

#include <IPAddress.h>


#include <LXDMXEthernet.h>
#include <LXArtNet.h>
#include <LXSACN.h>

//*********************** defines ***********************

/*
   Enter a MAC address and IP address for your controller below.
   The MAC address can be random as is the one shown, but should
   not match any other MAC address on your network.
   
   If BROADCAST_IP is not defined, ArtPollReply will be sent directly to server
   rather than being broadcast.  (If a server' socket is bound to a specific network
   interface ip address, it will not receive broadcast packets.)
*/

#define USE_DHCP 1
#define USE_SACN 1
// comment out next line for unicast sACN
//#define USE_MULTICAST 1

#define MAC_ADDRESS 0x90, 0xA2, 0xDA, 0x10, 0x6C, 0xA8
#define IP_ADDRESS 192,168,1,20
#define GATEWAY_IP 192,168,1,1
#define SUBNET_MASK 255,255,255,0
#define BROADCAST_IP 192,168,1,255
// this is the IP address to which output packets are sent, unless multicast is used
#define TARGET_IP 192,168,1,255

// this sketch flashes an indicator led:
#define LED_PIN 3

// the driver direction is controlled by:
#define RXTX_PIN 2

//the Ethernet Shield has an SD card that also communicates by SPI
//set its select pin to output to be safe:
#define SDSELECT_PIN 4

//*********************** globals ***********************

//network addresses
byte mac[] = {  MAC_ADDRESS };
IPAddress ip(IP_ADDRESS);
IPAddress gateway(GATEWAY_IP);
IPAddress subnet_mask(SUBNET_MASK);

#if defined( BROADCAST_IP )
IPAddress broadcast_ip( BROADCAST_IP);
#else
IPAddress broadcast_ip = INADDR_NONE;
#endif

// buffer
unsigned char packetBuffer[ARTNET_BUFFER_MAX];

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP eUDP;

// LXDMXEthernet instance ( created in setup so its possible to get IP if DHCP is used )
LXDMXEthernet* interface;

// sACN uses multicast, Art-Net uses broadcast. Both can be set to unicast (use_multicast = 0)
#if defined ( USE_MULTICAST )
uint8_t use_multicast = USE_SACN;
#else
uint8_t use_multicast = 0;
#endif


uint8_t led_state = 0;

void blinkLED() {
  if ( led_state ) {
    digitalWrite(LED_PIN, HIGH);
    led_state = 0;
  } else {
    digitalWrite(LED_PIN, LOW);
    led_state = 1;
  }
}

int got_dmx = 0;
void gotDMXCallback(int slots);
IPAddress send_address;

void setup() {
  pinMode(LED_PIN, OUTPUT);  //status LED
  blinkLED();
  #if defined(SDSELECT_PIN)
    pinMode(SDSELECT_PIN, OUTPUT);
  #endif

  if ( USE_DHCP ) {                                          // Initialize Ethernet
    Ethernet.begin(mac);                                     // DHCP
  } else {
  	Ethernet.begin(mac, ip, gateway, gateway, subnet_mask);   // Static
  }

  blinkLED();
  
  if ( USE_SACN ) {                       // Initialize Interface (defaults to first universe)
    interface = new LXSACN();
    //interface->setUniverse(1);	         // for different universe, change this line and the multicast address below
    if ( use_multicast ) {
      send_address = IPAddress(239,255,0,1);
    } else {
      send_address = IPAddress(TARGET_IP);
    }
  } else {
    interface = new LXArtNet(Ethernet.localIP(), Ethernet.subnetMask());
    #if defined( BROADCAST_IP )
       send_address = broadcast_ip;
    #else
      send_address = IPAddress(TARGET_IP);
    #endif
    
    //((LXArtNet*)interface)->setSubnetUniverse(0, 0);  //for different subnet/universe, change this line
  }
  
  LXSerialDMX.setDataReceivedCallback(&gotDMXCallback);
  LXSerialDMX.startInput();

  if ( use_multicast ) {
    //eUDP.beginMulticast(send_address,  interface->dmxPort() - 1);
    eUDP.begin(interface->dmxPort());
  } else {
    eUDP.begin(interface->dmxPort());
  }

  interface->setNumberOfSlots(512);

  blinkLED();
}


// ***************** input callback function *************

void gotDMXCallback(int slots) {
  got_dmx = slots;
}

/************************************************************************

  The main loop fades the levels of addresses 7 and 8 to full
  
*************************************************************************/

void loop() {
  if ( got_dmx ) {
    //interface->setNumberOfSlots(got_dmx);
    for(int i=1; i<=got_dmx; i++) {
      interface->setSlot(i, LXSerialDMX.getSlot(i));
    }
    blinkLED();
    
    interface->sendDMX(&eUDP, send_address);

    got_dmx = 0;
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}