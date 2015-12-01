# LXDMXEthernet_library
Library for Arduino with Ethernet Shield implements Art-Net and sACN with example DMX output using USART

LXDMXEthernet encapsulates functionality for sending and receiving DMX over ethernet.
   It is a virtual class with concrete subclasses LXArtNet and LXSACN which specifically
   implement the Artistic Licence Art-Net and PLASA sACN 1.31 protocols.
   
   Note:  LXDMXEthernet_library requires
          Ethernet library used with the original Arduino Ethernet Shield
          For the Ethernet v2 shield, use LXDMXEthernet2_library along with Ethernet2 Library
          
          For multicast, EthernetUDP.h and EthernetUD.cpp in the Ethernet library
          must be modified to add the beginMulticast method.
          See the code at the bottom of LXDMXEthernet.h
          
Included examples of the library's use:

         Art-Net or sACN controlling Adafruit NeoPixels
         
         DMX output using the AVR's USART and an external line driver chip
