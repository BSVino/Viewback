The Viewback Network Protocol
=============================

Network communication in Viewback happens in two stages.

Multicast Discovery
-------------------

The Viewback server pumps out a signal once per second using UDP multicast to announce its presence on the network on multicast group 239.127.251.37:51072. (All prime numbers! Except the port, I guess.) The format is:

	Bytes 0 & 1: "VB"              // First two bytes are ASCII "VB", a unique identifier to reduce multicast collisions.
	Byte 2:      0x1               // This is a version identifier. Currently version 1 is the only version. You should ignore newer versions.
	Bytes 3 & 4: 27015             // This is an unsigned short in network byte order indicating the port that the Viewback server is running on.
	Bytes 5->:   "Viewback Server" // A null terminated string that is the name of this Viewback server.

TCP Messages
------------

Once the client connects to the Viewback server, it can send commands to the server using simple strings and receive replies as Google Protobuf messages. When the client connects, all channels begin deactivated, and the client must request that a channel be activated using the commands below. Before sending any data the server will automatically send a registration packet. See the data section for details on what the registration packet contains.

### Commands

Command messages are usually plaintext ascii encoded null terminated strings.

`registrations`

Request a registration packet from the server. The client shouldn't have to send this message ever since the server sends it automatically, but it's still here for legacy reasons.

`console: [command]`

Anything following the first space (byte index 9+) represents a console command incoming from the client that should be executed in the game console. Example: `console: sv_cheats 1`

`activate: [channel#]`

Anything following the first space (byte index 10+) represents an ascii encoded channel number that should be activated.

`deactivate: [channel#]`

Anything following the first space (byte index 12+) represents an ascii encoded channel number that should be deactivated.

`group: [group#]`

Anything following the first space (byte index 7+) represents an ascii encoded group number whose channels should be activated. All groups not in that channel should be deactivated.

`control: [control#] [options]`

A control was modified on the client, the server should call the specified control callback. The control number is ascii encoded at byte index 9 until the next space. Buttons have no options, sliders have the value that is to be set (integer or float) following a space. Example: `control: 2 3.14` means to set control index 2 (which should be a float slider) to value 3.14.

### Data

All packets sent from the Viewback server to the Viewback client are Google Protobuf messages, prepended with a four-byte network order unsigned integer representing the length of the protobuffer message. The .proto file can be found in the `protobuf` directory in this repository.

Some of the packet information will be registration information (`data_channels`, `data_groups`, `data_labels`, `data_controls`) and these should be sent by themselves and not packaged with any data or console messages.





