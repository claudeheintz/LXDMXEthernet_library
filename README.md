# LXDMXEthernet_library
Library for Arduino with Ethernet Shield implements Art-Net and sACN with example DMX output using USART

LXDMXEthernet encapsulates functionality for sending and receiving DMX over ethernet.
   It is a virtual class with concrete subclasses LXArtNet and LXSACN which specifically
   implement the Artistic Licence Art-Net and PLASA sACN 1.31 protocols.
   
   Note:  LXDMXEthernet_library requires
          Ethernet library used with the original Arduino Ethernet Shield
          For the Ethernet v2 shield, use the branch that has includes for Ethernet2.h and EthernetUdp2.h
          
          For multicast, EthernetUDP.h and EthernetUDP.cpp in the Ethernet library
          must be modified to add the beginMulticast method.
          See the code at the bottom of LXDMXEthernet.h
          
Included examples of the library's use:

         Art-Net or sACN controlling Adafruit NeoPixels
         
         DMX output using the AVR's USART and an external line driver chip







LXSeries open source projects are free to download and have BSD style licenses so their code can be reused by anyone.
If you would like to support LXSeries open source projects and free applications, you can make a donation:
   https://www.claudeheintzdesign.com/lx/lxseries_opensource_donate.html
