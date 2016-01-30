#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUDP.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <LXDMXEthernet.h>
#include <LXArtNet.h>
#include <LXSACN.h>

//*********************** defines ***********************

#define PIN 6
#define NUM_LEDS 12

//  Make choices here about protocol ( set to 1 to activate option )

#define USE_DHCP 0
#define USE_SACN 0

//  Uncomment to use multicast, which requires extended Ethernet library
//  see note in LXDMXEthernet.h file about method added to library

//#define USE_MULTICAST 1

//*********************** globals ***********************

// network addresses
// IP address is defined assuming that the Arduino is connected to
// a Mac without a router and a self assigned ip
byte mac[] = { 0x00, 0x08, 0xDC, 0x4C, 0x29, 0x7E }; //00:08:DC Wiznet
IPAddress ip(169,254,100,100);
IPAddress gateway(169,254,1,1);
IPAddress subnet_mask(255,255,0,0);

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP eUDP;

// Adafruit_NeoPixel instance to drive the NeoPixels
Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// LXDMXEthernet instance ( created in setup so its possible to get IP if DHCP is used )
LXDMXEthernet* interface;

// sACN uses multicast, Art-Net uses broadcast. Both can be set to unicast (use_multicast = 0)
uint8_t use_multicast = USE_SACN;


//*********************** setup ***********************
void setup() {

  if ( USE_DHCP ) {                                          // Initialize Ethernet
    Ethernet.begin(mac);                                     // DHCP
  } else {
  	Ethernet.begin(mac, ip, gateway, gateway, subnet_mask);   // Static
  }
  
  if ( USE_SACN ) {                       // Initialize Interface (defaults to first universe)
    interface = new LXSACN();
    //interface->setUniverse(1);	         // for different universe, change this line and the multicast address below
  } else {
    interface = new LXArtNet(Ethernet.localIP(), Ethernet.subnetMask());
    use_multicast = 0;
    //((LXArtNet*)interface)->setSubnetUniverse(0, 0);  //for different subnet/universe, change this line
  }

  if ( use_multicast ) {                  // Start listening for UDP on port
    eUDP.beginMulticast(IPAddress(239,255,0,1), interface->dmxPort());
  } else {
    eUDP.begin(interface->dmxPort());
  }
    
  ring.begin();                   // Initialize NeoPixel driver
  ring.show();
}

//*********************** main loop *******************
void loop() {
  uint16_t r,g,b;
  uint8_t i;
  
  // read a packet and if the packet is dmx, write its data to the pixels
  if ( interface->readDMXPacket(eUDP) == RESULT_DMX_RECEIVED ) {
    for (int p=0; p<NUM_LEDS; p++) {
      // for each NeoPixel find the slot number (each takes 3 slots for RGB)
      i = 3*p;
      r = interface->getSlot(i+1);  // Red
      g = interface->getSlot(i+2);  // Green
      b = interface->getSlot(i+3);  // Blue
      // gamma correct
      r = (r*r)/255;    
      g = (g*g)/255;
      b = (b*b)/255;
      // write to NeoPixel Driver
      ring.setPixelColor(p, r, g, b);
    }
    // send to NeoPixel Ring
    ring.show();
  }
}
