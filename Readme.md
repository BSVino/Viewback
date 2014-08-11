Viewback - A Video Game Design Tool
===================================

Viewback is a tool to help game designers conduct usability play tests. It
forwards the debug information from your game to your wireless device, where
you can see it and the player can't. Now you can diagnose problems with the
game while your playtester enjoys her experience. You can see changes to
internal game state and send commands to the game in real time.

[Watch this video to see how it works.](https://www.youtube.com/watch?v=vzF4IUAhqgI)

Downloads
---------

* [The Viewback Monitor for Windows](http://vinoisnotouzo.com/viewback/ViewbackMonitor.zip) - Includes a test "game". Run the game and the monitor at the same time and the test game should show up in the monitor.
* [The Viewback Monitor for Android](http://vinoisnotouzo.com/viewback/ViewbackMonitor.apk) - Requires Android 4.3. If you would like to see it on Android < 4.3, bug me about it and I'll move it higher in my priority list.

Integration
-----------

The Viewback server is written in C and can be easily integrated into any game
engine. It uses a permissive MIT license, avoids blocking networking calls, and
uses only a few hundred bytes of memory. The server compiles on any C compiler
with no dependencies, and the monitor is available for Windows and Android,
with OSX and iOS coming soon.

Since Viewback is written in C it can be used in just about any game engine
environment. Any engine written in C or C++ (Id Tech engines, Unreal, Source)
can use Viewback with no extra effort. Users of engines written in Java or C#
(Unity, Minecraft) can either write language bindings or implement the
[Viewback network protocol](https://github.com/BSVino/Viewback/blob/master/NetworkProtocol.md) on their own.

This repository contains the header and source code for the Viewback server,
which will typically be integrated into your game's client, if your game is
multiplayer. It looks like this:

    Viewback client <----------> | Viewback server |
    (eg The monitor app)         | Game client     | <------> Game server

Installation Instructions
-------------------------

First, copy all files in the server directory to a directory inside your
source tree. If you're familiar with git, you can use a git submodule for
this purpose. Then add viewback.cpp and viewback_util.cpp to your project
files. In whichever files you want to use Viewback, add at the top:

	#include "viewback_util.h"

Now you are ready to implement the API.

Sample Code
-----------

This code uses the `vb_util` interface, which handles memory allocations for
you. If you care about how Viewback manages memory, see viewback.h for an
interface that allows you to allocate the memory that Viewback uses.

	#include "viewback_util.h"

	vb_util_initialize(); // This is optional.

	// A "channel" is a stream of data to be sent to the Viewback monitor for display.
	// Here we create an integer channel called "Health".
	vb_channel_handle_t health_channel;
	vb_util_add_channel("Health", VB_DATATYPE_INT, &health_channel);

	// The name you pass in here will be displayed in the server list on the monitor.
	vb_util_server_create("My Amazing Game");

	while (game_running())
	{
		// Call this as many times as you like, but at least once per game frame.
		vb_server_update(game_time);

		// You can send data once per frame, or only when the data changes. It's up to you.
		if (!vb_data_send_int(health_channel, player->GetHealth()))
			printf("Error sending to Viewback channel\n");
	}

	vb_server_shutdown();

Features
--------

### Channels

A channel is a stream of data to be sent to the Viewback monitor for display.
Each channel has a type, currently supported types are integer, float, or vector.
Depending on the type of the vector the data will be displayed in the monitor
in a different panel. Floats and ints will be shown in the time display and
vectors will be shown in the 2D display.

### Groups

A group allows you to organize your channels. You can add a channel to a group
and then activate a group to see all of the channels in that group. For example
an "Animation" group may contain these channels:

* PlayerYaw
* ViewVector
* PlayerVelocity

while the "PlayerMovement" group would contain

* PlayerVelocity
* JumpButton
* OnGround

A channel can belong to multiple groups or no groups.

### Labels

Integer channels are often enumerations - i.e. each value represents a state.
These states often have names and looking at the names is nicer than looking
at the numbers. So for integer channels you can specify that a certain value
has a name, and this name will show up in the monitor instead of the number.
For example for the PlayerState channel you may have these labels:

* 0: Dead
* 1: Respawning
* 2: Alive
* 3: DeathAnimation
* 4: Spectating

Whenever the channel has the value of "2", the monitor will show the "PlayerState"
channel as being "Alive".

### Controls

You can specify controls to modify parameters of your game in real time. These
controls appear in the monitor and when they are manipulated by the user,
the callbacks that you specify will be triggered in the game code. There are
currently two types of controls supported.

#### Buttons

When pressed, a callback function in the game will execute. For example,
a "Pause" button could call this function:

	void viewback_pause_callback()
	{
		Game()->TogglePause();
	}

Other ideas for buttons:

* Take a screenshot without leaning over your playtester.
* Turn cheats on and off.
* Activate the bug report system.
* Reset the level if your playtester got stuck.

#### Sliders

When the slider handle is moved a callback in the game will execute. Sliders
can specify integer or float values. Some ideas for sliders:

* Adjust the difficulty of the game if your playtester is having trouble (or not enough trouble)
* Adjust the number of bots in the game
* Real-time tuning of a design parameter that you've been trying to get right, like the player run speed or jump height.

### Console

If your game has a console, you can forward the console to Viewback. Output
from the console will appear in the Viewback monitor and the user can input
commands into the monitor which will get forwarded to the game.

Don't have a console in your game? No sweat, you can use the Viewback monitor
as your console. Just call the Viewback `vb_console_append()` function with
whatever messages you want to see, and it will show up on the Viewback monitor.

### Status

The status string is like the console but it's always visible. New status
lines replace old lines and they never scroll off the screen. Use it for
things like the framerate, memory used, and how many monsters are currently
spawned.
