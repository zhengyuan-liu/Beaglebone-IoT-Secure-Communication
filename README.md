# Beaglebone-IoT-Secure-Communication
A embedded system (Beaglebone) and Internet of Things project about secure communication. 

Beaglebone with a temperature sensor accepts commands from a network server, and sends reports back to the server, in both unencrypted (TCP) and encrypted (TLS) channels.

The embedded OS of Beaglebone is Debian GNU/Linux 8. MRAA AIO/GPIO is used to get IO from sensors.

## Source Code Description:
### Unencrypted Communication with a Logging Server
lab4c_tcp:
* builds and runs on Beaglebone.
* accepts the following parameters:
  * --id=9-digit-number
  * --host=name or address
  * --log=filename
  * port number
* accepts commands and generates reports from/to a network connection to a server.
  1. open a TCP connection to the server at the specified address and port
  2. immediately send (and log) an ID terminated with a newline: ID=ID-number 
This new report enables the server to keep track of which devices it has received reports from.
  3. send (and log) newline terminated temperature reports over the connection
  4. process (and log) newline-terminated commands received over the connection
  5. The last command sent by the server will be an OFF.

The temperature sensor has been connected to Analog input 0.

### Authenticated TLS Session Encryption
lab4c_tls:
* builds and runs on Beaglebone
* operates by:
  1. opening a TLS connection to the server at the specified address and port
  2. sending (and logging) ID followed by a newline
  3. sending (and logging) temperature reports over the connection
  4. processing (and logging) commands received over the connection

