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

#if !defined(VIEWBACK_TIME_DOUBLE)
typedef unsigned long long vb_uint64;
#endif

typedef unsigned char vb_bool;

/*
	Viewback - A tool for designing video games written by Jorge Rodriguez
	Some code unscrupulously stolen from Webby https://github.com/deplinenoise/webby
	Other code unscrupulously stolen from http://code.google.com/p/protobuf-embedded-c/

	Most functions return either void or vb_bool. In the case of void, no errors are
	possible. In the case of vb_bool, 0 means an error occurred and 1 means no error
	occurred. You should check the return value of each call.

	Many const char* strings you pass in will be stored by Viewback, so try not
	to do nasty stuff like vb_something(std::string("example").c_str());

	The memory required by Viewback can be reduced by specifying certain preprocessor
	flags. Don't forget to specify them for both viewback.c and all places where
	viewback.h is included, it's best to put them in your project files.
	VR_NO_RANGE - Remove the ability to specify a channel's range, saves 8 bytes per channel.

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
		How many different channels of data you would like to send to the
		monitor.
	*/
	size_t num_data_channels;

	/*
		Data channels can be placed into groups. These groups can be turned on
		and off in the client as a means to organize the data. This is the
		maximum number of groups possible.
	*/
	size_t num_data_groups;

	/*
		You can add a data channel to a group a maximum of this number of
		times. E.G. if you have two groups, and group 1 has channels A B and C,
		and group 2 has C D E and F, that's 7 total group members. In other
		words it should be at least the number of times that you want to
		call vb_data_add_channel_to_group().
	*/
	size_t num_data_group_members;

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

typedef unsigned short vb_channel_handle_t;
typedef unsigned short vb_group_handle_t;

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
	Returns 1 if the memory provided was sufficient and 0 otherwise.
*/
vb_bool vb_config_install(vb_config_t* config, void* memory, size_t memory_size);

/*
	Viewback will no longer try to reference the memory passed in by
	vb_config_install once this function is called. Only call it after
	vb_server_shutdown is called and vb_server_is_running returns false,
	never while a server is running.
*/
void vb_config_release();

/*
	Register a channel of data. You should provide an address to a handle and
	store that handle somewhere. 'handle' can be NULL. 'name' will not be
	copied elsewhere, so make sure it is persistent memory.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_add_channel(const char* name, vb_data_type_t type, /*out*/ vb_channel_handle_t* handle);

/*
	Register a group of data. You should provide an address to a handle and store
	that handle somewhere. 'handle' can be NULL. 'name' will not be
	copied elsewhere, so make sure it is persistent memory.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_add_group(const char* name, /*out*/ vb_group_handle_t* handle);

/*
	Add the specified channel of data to the specified group.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_add_channel_to_group(vb_group_handle_t group, vb_channel_handle_t channel);

/*
	Register a label for integers. When the specified data has the specified value
	the monitor will give it the specified label. Think enumerations. Example:
	vb_data_label(vb_player_state, 0, "Dead");
	vb_data_label(vb_player_state, 1, "Alive");
	vb_data_label(vb_player_state, 2, "Hungry");
	vb_data_label(vb_player_state, 3, "Ephemeral");
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_add_label(vb_channel_handle_t handle, int value, const char* label);

/*
	Just retrieves the data registered in vb_data_label(). "label" output is only
	valid if the function returns true.
*/
vb_bool vb_data_get_label(vb_channel_handle_t handle, int value, /*out*/ const char** label);

#ifndef VB_NO_RANGE
/*
	If you set this, the monitor will fix the range to the specified values.
	Otherwise the chart will automatically fit the window. For vector data,
	only the max is used.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_set_range(vb_channel_handle_t handle, float range_min, float range_max);
#endif

/*
	After registering all of your data, call this to start up the server.
*/
vb_bool vb_server_create();

/*
	Call every frame. It should take only minimal processing and never does
	memory allocations.
*/
#ifdef VIEWBACK_TIME_DOUBLE
void vb_server_update(double current_time_seconds);
#else
void vb_server_update(vb_uint64 current_time_milliseconds);
#endif

/*
	Closes all sockets. After shutdown you can add more channels or reset the
	config (which removes all channels) if you like. After calling this
	Viewback no longer uses the memory you passed it, so you can free it if
	you won't be using Viewback anymore.
*/
void vb_server_shutdown();

/*
	Returns nonzero if the server is active.
*/
vb_bool vb_server_is_active();

/*
	These methods send data to the monitor. If you use a handle that was
	registered as an int but you try to send it as a float, it will fail.
	These functions use blocking send() and may block if the send buffer
	is full.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_send_int(vb_channel_handle_t handle, int value);
vb_bool vb_data_send_float(vb_channel_handle_t handle, float value);
vb_bool vb_data_send_vector(vb_channel_handle_t handle, float x, float y, float z);

/*
	These methods also send data to the monitor, but will look up the handle
	for you using a linear search.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_send_int_s(const char* channel, int value);
vb_bool vb_data_send_float_s(const char* channel, float value);
vb_bool vb_data_send_vector_s(const char* channel, float x, float y, float z);

/*
	Any text that goes to your console can also be piped into Viewback for
	display in the monitor.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_console_append(const char* text);

/*
	Set the status text. Unlike the console, the status text doesn't append,
	it just shows whatever is in the status. Good for fps and current assets
	loaded and that sort of thing.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_status_set(const char* text);

#ifdef __cplusplus
}
#endif
