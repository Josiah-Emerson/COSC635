# Overview
The PP1_UDPFileSender program sends the specified file specified by the filename to the IPv4 destination address 
and port number. The PP1_UDPFileReceiver program receives the file sent by the sender program. The receiver is able 
to detect any packets out of order, including when the [END] packet is received prior to a [DATA] packet. Once the [START]
PDU is received, the receiver will timeout after 5 seconds of inactivity. In the event that the receiver times out before 
receiving all packets, the program will notify the user that some or all of their data is missing as well as the amount missing. 
The program will keep the received data written in the file with the .part extension (i.e. filename.part) in a timeout situation. 
As an example, if 5 [DATA] packets #1-5 are expected for the file foo.ext, but packet #4 is not received before the timeout period,
then the file foo.part contains the data from the packets: [1, 2, 3, 5]. Lastly, the program is able to distinguish the case when 
it has received all [DATA] packets, but has time out without receiving the [END] packet. For this case, it will still store it in foo.part,
but it will notify the user that it has received all the data of the file.

usage:
```
build/bin/PP1_UDPFileSender [-i <IPv4 Address>] [-p <portno>] [-f <filename>]
```
Receiver: 
usage: 
```
build/bin/PP1_UDPFileReceiver <portno>
```

To compile:
```
cmake -B build
cmake --build build
```

executables will be placed in the build/bin directory
