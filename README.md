# LXDMXEthernet_library
Library for Arduino with Ethernet Shield implements Art-Net and sACN with example DMX output using USART

LXDMXEthernet encapsulates functionality for sending and receiving DMX over ethernet.
   It is a virtual class with concrete subclasses LXArtNet and LXSACN which specifically
   implement the Artistic Licence Art-Net and PLASA sACN 1.31 protocols.
   
   Note:  LXDMXEthernet_library requires
          Ethernet library (with the original Arduino Ethernet Shield or Ethernet Shield v2)
          #include the appropriate files in your sketch:
          Ethernet.h & EthernetUdp.h -OR- Ethernet2.h & EthernetUdp2.h
          
          For multicast, EthernetUdp2.h and EthernetUdp2.cpp in the Ethernet library
          must be modified to add the beginMulticast method.
          See the code at the bottom of LXDMXEthernet.h
          (EthernetUdp.h in IDE 1.6.8 includes beginMulticast)
          
Included examples of the library's use:

         Art-Net or sACN controlling Adafruit NeoPixels
         
         DMX output using the AVR's USART and an external line driver chip
         
         DMX output using Mkr1000's SERCOM and an external line driver chip
         
         Receive 2 universes using external packet buffers
