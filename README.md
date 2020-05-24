# CppNetwork
Network programing library for C++. Socket, Server, and Client will be covered.

## Knowledge
### Sockets
**Stream Sockets**
- TCP - The Transmission Control Protocol
- Reliable two-way connected communication streams
- Order of received packet will be order of sent packet
- Error free
- Slower than Datagram Socket

**Datagram Sockets**
- UDP - User Datagram Protocol
- Connectionless, it does not require the connection
- Unreliable, some packets can be lost/dropped
- Fast!

### IP Address
**The Internet Protocol Version 4**
- IPv4
- Consist of 32-bit(4-bytes) (e.g. 127.0.0.1.)
- Loopback: 127.0.0.1.
- All internet website uses IPv4

**The Internet Protocol Version 6**
- IPv6
- Consist of 128-bit (e.g. 0000:0000:0000:0000:0000:0000:0000:ffff)
- Loopback
  - ::1
  - ::1/128
  - ::ffff:127.0.0.1
  
**Subnets**
- netmask
  - Class A: 255.0.0.0
  - Class B: 255.255.0.0
  - Class C: 255.255.255.0
- Put subnet by adding slash after IP address
  - e.g. 127.0.0.1/17
  
  ## Port Number
  16-bit number used by socket to find the connection in the local
  - Service can be differentiated by port number with same IP Address
