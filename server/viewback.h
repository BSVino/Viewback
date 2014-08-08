/*
Copyright (c) 2014, Jorge Rodriguez, bs.vino@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
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

	This is the code for the viewback server, which will typically be integrated into
	your game's client, if your game is multiplayer. It looks like this:

	Viewback client <----------> | Viewback server |
	(eg The monitor app)         | Game client     | <------> Game server

	Most functions return either void or vb_bool. In the case of void, no errors are
	possible. In the case of vb_bool, 0 means an error occurred and 1 means no error
	occurred. You should check the return value of each call.

	Many const char* strings you pass in will be stored by Viewback, so try not
	to do nasty stuff like vb_something(std::string("example").c_str());

	The memory required by Viewback can be reduced by specifying certain preprocessor
	flags. Don't forget to specify them for both viewback.c and all places where
	viewback.h is included, it's best to put them in your project files.
	VR_NO_RANGE - Remove the ability to specify a channel's range, saves 8 bytes per channel.
	VR_NO_COMPRESSION - Remove delta compression, saves 20 bytes per channel.

	On Windows you must call WSAStartup before using Viewback.

	Refer to the readme for more information.
*/

/*
	Sample code follows. If you don't care about memory allocations then
	an easier API is available from viewback_util.h


	vb_config_t config;
	vb_config_initialize(&config);

	config.num_data_channels = 2;

	size_t memory_size = vb_config_get_memory_required(&config);
	m_memory = malloc(memory_size);

	if (!vb_config_install(&config, m_memory, memory_size))
		return 0;

	vb_channel_handle_t channel;
	if (!vb_data_add_channel("Test", VB_DATATYPE_INT, &channel))
		return 0;

	vb_server_create();

	while (game_running())
	{
		vb_server_update(game_time);

		if (!vb_data_send_int(channel, rand()))
			printf("Error sending to Viewback channel\n");
	}

	vb_server_shutdown();
*/

typedef void(*vb_debug_output_callback)(const char* text);
typedef void(*vb_command_callback)(const char* text);
typedef void(*vb_control_button_callback)();
typedef void(*vb_control_slider_float_callback)(float value);
typedef void(*vb_control_slider_int_callback)(int value);


typedef struct {
	/*
		This is advertised over UDP multicast and will be seen when clients
		scan for servers. If there are multiple servers on the network,
		it's a good idea to distinguish each server by player name,
		eg "Double Action: cliffyb" and "Double Action: Gaben"
		NOTE: Viewback doesn't make a copy so don't use memory that will be freed.
	*/
	const char* server_name;

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
		A list of controls that can be used to modify in-game values in real time.
		This is the max number of controls that will be available.
	*/
	size_t num_data_controls;

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

	/*
		Must be a valid IP group eg "239.127.251.37" If this is NULL the
		default will be used. See: http://en.wikipedia.org/wiki/Multicast_address
		If you change this then Viewback clients won't be able to connect
		automatically and will have to connect manually. You should probably
		leave it 0 unless your network is special.
	*/
	const char* multicast_group;

	/*
		This port will be used for UDP multicast connections. 0 means use the
		default port. If you change this then Viewback clients won't be able
		to connect automatically and will have to connect manually. You should
		probably leave it 0 unless your network is special.
	*/
	unsigned short multicast_port;

	/*
		This port will be used for TCP connections. 0 means use an automatically
		assigned default port. The multicast signal broadcasts the TCP port so
		there's no reason to set this unless your network is special.
	*/
	unsigned short tcp_port;

} vb_config_t;

typedef unsigned short vb_channel_handle_t;
typedef unsigned short vb_group_handle_t;

/* If you change this, update it in data.proto as well. */
typedef enum
{
	VB_DATATYPE_NONE   = 0,
	VB_DATATYPE_INT    = 1,
	VB_DATATYPE_FLOAT  = 2,
	VB_DATATYPE_VECTOR = 3,
} vb_data_type_t;

/* If you change this, update it in data.proto as well. */
typedef enum
{
	VB_CONTROL_NONE         = 0,
	VB_CONTROL_BUTTON       = 1,
	VB_CONTROL_SLIDER_FLOAT = 2,
	VB_CONTROL_SLIDER_INT   = 3,
	VB_CONTROL_MAX          = 4,
} vb_control_t;

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
	Register a control, a more convenient way to send commands to the game.
	Float sliders are given a min and max range (the highest and lowest values
	on the slider) and a "steps" parameter, which specifies how many total values
	are selectable on the slider. For example, if steps is 3 then the three
	selectable values are range_min, range_max, and (range_min + range_max)/2.
	If steps is 0 then the range is treated as continuous and any value in
	[range_min, range_max] can be chosen.
	Int sliders are given a min and max range as well, but the step_size
	specifies the difference between each selectable value. For example if
	step_size is 1 then your values are 1, 2, 3, 4 and if it is 2 then your
	values are 2, 4, 6, 8.
	When a user fiddles with the control in the monitor, the callback is
	triggered.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_add_control_button(const char* name, vb_control_button_callback callback);
vb_bool vb_data_add_control_slider_float(const char* name, float range_min, float range_max, int steps, vb_control_slider_float_callback callback);
vb_bool vb_data_add_control_slider_int(const char* name, int range_min, int range_max, int step_size, vb_control_slider_int_callback callback);

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
