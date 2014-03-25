/*
Copyright (c) 2014, Jorge Rodriguez, bs.vino@gmail.com

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software must display the following acknowledgement:
   This product includes software developed by Jorge Rodriguez.
4. Neither the name of the Jorge Rodriguez nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY JORGE RODRIGUEZ ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JORGE RODRIGUEZ BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	Viewback - A tool for designing video games written by Jorge Rodriguez
	Some code unscrupulously stolen from Webby https://github.com/deplinenoise/webby

	All functions return either void or int. In the case of void, no errors are
	possible. In the case of int, 0 means an error occurred and 1 means no error
	occurred. You should check the return value of each call.

	Many const char* strings you pass in will be stored by Viewback, so try not
	to do nasty stuff like vb_something(std::string("example").c_str());

	Refer to the readme for more information.
*/

typedef void(*vb_debug_output_callback)(const char* text);
typedef void(*vb_command_callback)(const char* text);

typedef struct {

	/*
		Must be a valid IP group eg "239.127.251.37" If this is NULL the
		default will be used. You should probably leave it NULL unless your
		network is special. See: http://en.wikipedia.org/wiki/Multicast_address
	*/
	const char* multicast_group;

	/*
		This port will be used for UDP multicast and for the TCP connections.
	*/
	unsigned short port;

	/*
		How many different types of data you would like to send to the monitor.
	*/
	size_t num_data_registrations;

	/*
		Some data can carry labels for when the data is a certain value.
		EG 0 means off, 1 - loading, 2 - initializing, 3 - running,
		4 - shutting down. This is the max number of labels for all data.
	*/
	size_t num_data_labels;

	/*
		How many viewback monitors will be able to connect. Should be at
		least 1, but more monitors are cheap so more doesn't hurt.
	*/
	unsigned char max_connections;

	/*
		This function will be called by Viewback any time a command comes in
		from the client to run. It should be passed into the game's console.
	*/
	vb_command_callback command_callback;

	/*
		This function is for the purposes of debugging Viewback. It will be
		called whenever Viewback has debug output. You'll usually want it off
		unless you're developing Viewback or having problems setting it up.
	*/
	vb_debug_output_callback debug_output_callback;
} vb_config_t;

typedef unsigned int vb_data_handle_t;

/* If you change this, update it in viewback.proto as well. */
typedef enum
{
	VB_DATATYPE_NONE = 0,
	VB_DATATYPE_INT = 1,
	VB_DATATYPE_FLOAT = 2,
	VB_DATATYPE_VECTOR = 3,
} vb_data_type_t;

/*
	A good idea (but not required) to pass your config in here first to make
	sure you get the default values.
*/
void vb_config_initialize(vb_config_t* config);

/*
	Pass in your config and Viewback will tell you how much memory it requires.
*/
size_t vb_config_get_memory_required(vb_config_t* config);

/*
	Pass in a config and a memory buffer of the size returned by
	vb_config_get_memory_required() or larger.
*/
int vb_config_install(vb_config_t* config, void* memory, size_t memory_size);

/*
	Register a type of data. You should provide an address to a handle and store
	that handle somewhere.
*/
int vb_data_register(const char* name, vb_data_type_t type, /*out*/ vb_data_handle_t* handle);

/*
	Register a label for integers. When the specified data has the specified value
	the monitor will give it the specified label. Think enumerations. Example:
	vb_data_label(vb_player_state, 0, "Dead");
	vb_data_label(vb_player_state, 1, "Alive");
	vb_data_label(vb_player_state, 2, "Hungry");
	vb_data_label(vb_player_state, 3, "Ephemeral");
*/
int vb_data_label(vb_data_handle_t handle, int value, const char* label);

/*
	Just retrieves the data registered in vb_data_label(). "label" output is only
	valid if the function return true.
*/
int vb_data_get_label(vb_data_handle_t handle, int value, /*out*/ const char** label);

/*
	If you set this, the monitor will fix the range to the specified values.
	Otherwise the chart will automatically fit the window. For vector data,
	only the max is used.
*/
int vb_data_set_range(vb_data_handle_t handle, float min, float max);

/*
	After registering all of your data, call this to start up the server.
*/
int vb_server_create();

/*
	Call every frame. It should take only minimal processing and never does
	memory allocations.
*/
void vb_server_update(double current_time_seconds);

/*
	Closes all sockets. After shutdown you can add more registrations or reset
	the config (which removes all registrations) if you like. After calling this
	Viewback no longer uses the memory you passed it, so you can free it if
	you won't be using Viewback anymore.
*/
void vb_server_shutdown();

/*
	These methods send data to the monitor. If you use a handle that was
	registered as an int but you try to send it as a float, it will fail.
*/
int vb_data_send_int(vb_data_handle_t handle, int value);
int vb_data_send_float(vb_data_handle_t handle, float value);
int vb_data_send_vector(vb_data_handle_t handle, float x, float y, float z);

/*
	Any text that goes to your console can also be piped into Viewback for
	display in the monitor.
*/
int vb_console_append(const char* text);

/*
	Set the status text. Unlike the console, the status text doesn't append,
	it just shows whatever is in the status. Good for fps and current assets
	loaded and that sort of thing.
*/
int vb_status_set(const char* text);

#ifdef __cplusplus
}
#endif
