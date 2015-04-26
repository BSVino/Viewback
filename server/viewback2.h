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

typedef unsigned long long vb_uint64;
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

	None of Viewback is thread safe, you must handle synchronization yourself.

	Refer to the readme for more information.
*/

/*
	Sample code follows. If you don't care about memory allocations then
	an easier API is available from viewback_util2.h


	vb2_config_t config;
	vb_config_initialize(&config);

	// Here you specify how many of each feature you will use.
	config.num_data_channels = 2;

	size_t memory_size = vb_config_get_memory_required(&config);
	m_memory = malloc(memory_size);

	if (!vb_config_install(&config, m_memory, memory_size))
		return 0;

	// Since 2 channels were specified in the config, you can call
	// vb_data_add_channel() at most twice before it will return
	// an error.
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

/* What kind of memory allocation is this? */
typedef enum
{
	VB_AT_MAIN,     /* The main memory block that VB uses */
	VB_AT_AUTOFREE, /* A table of items that will be automatically freed on shutdown */
	VB_AT_AF_ITEM,  /* An item that will be automatically freed on shutdown */
} vb_alloc_type_t;

typedef void*(*vb_alloc)(size_t memory_size, vb_alloc_type_t type);
typedef void(*vb_free)(void* memory, vb_alloc_type_t type);
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
		Profiles are how your data will be organized in the monitor. A profile
		is a collection of channels and controls and an arrangement of panels.
		There is a hard maximum of 64 profiles. You must have at least one.
	*/
	size_t num_data_profiles;

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
		This port will be used for TCP connections. 0 means use an automatically
		assigned default port. If the specified port is not available, the next
		5 ports will be tried until an open port is found. The multicast signal
		broadcasts the TCP port so there's no reason to set this unless your
		network is special.
	*/
	unsigned short tcp_port;

#ifndef VIEWBACK_NO_CONFIG
	/*
		Viewback reads and writes configuration options and persistent data to
		a file. You can specify which file is to be used with this option.
		Otherwise leave it NULL and a file named "viewback.cfg" will be created
		in the current directory.
		NOTE: Viewback doesn't make a copy so don't use memory that will be freed.
	*/
	const char* config_file;
#endif

	// These are used by Viewback to allocate and free memory. Both must be
	// present or neither will be used. If these are not specified then Viewback
	// will use system malloc and free. These will be ignored if the user
	// passes memory to Viewback with vb_config_install().
	vb_alloc alloc_callback;
	vb_free free_callback;
} vb2_config_t;

typedef unsigned short vb_channel_handle_t;
typedef unsigned short vb_profile_handle_t;

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
void vb_config_initialize(vb2_config_t* config);

/*
	Pass in your config and Viewback will tell you how much memory it requires.
*/
size_t vb_config_get_memory_required(vb2_config_t* config);

/*
	Pass in a config and a memory buffer of the size returned by
	vb_config_get_memory_required() or larger.

	If memory is non-NULL, Viewback will use that memory and never try to
	resize its allocation. In this case, no channels, controls, etc can be created
	while Viewback is running.

	If memory is NULL, Viewback will initialize its own memory automatically.
	The alloc_callback and free_callback functions will be used if they are
	non-NULL, otherwise malloc and free will be used.

	Returns 1 if the memory provided was sufficient and 0 otherwise.
*/
vb_bool vb_config_install(vb2_config_t* config, void* memory, size_t memory_size);

/*
	Viewback will no longer try to reference the memory passed in by
	vb_config_install once this function is called. Only call it after
	vb_server_shutdown is called and vb_server_is_running returns false,
	never while a server is running.
	If Viewback initialized its memory automatically, it will be freed here.
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
	Register a monitor profile. You should provide an address to a handle and
	store that handle somewhere. 'handle' can be NULL. 'name' will not be
	copied elsewhere, so make sure it is persistent memory.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_data_add_profile(const char* name, /*out*/ vb_profile_handle_t* handle);

/*
	Add the specified channel of data to the specified profile.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_profile_add_channel(vb_profile_handle_t profile, vb_channel_handle_t channel);
vb_bool vb_profile_remove_channel(vb_profile_handle_t profile, vb_channel_handle_t channel);

/*
	Add the specified control of data to the specified profile.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_profile_add_control(vb_profile_handle_t profile, const char* control);

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
	If you set this, the monitor will fix the range to the specified channel.
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
	Register controls which execute the specified commands when they are used.
	The command will be sent to the command callback specified in
	vb_data_set_command_callback().

	For the sliders, the command can be specified in two ways. If a "%f" exists
	in your command string, then it will be replaced with the value that the
	user has set the slider to, and then sent to the command callback. Otherwise
	if a "%f" is not present, a space and then the value will be be appended to
	the command before it is sent to the command callback.

	The value-setting functions specified above work just fine for these and it
	is a good idea to set the initial values for yourself.

	Other options are the same as above.

	Examples:

	vb_data_add_control_button_command("Respawn", "player_respawn");

	When the [Respawn] button is pressed, Viewback will send the
	"player_respawn" command to the console callback.

	vb_data_add_control_slider_float_command("Player max speed", 10, 100, 0, "player_speed");

	When the slider is set to 70, this string will be sent to the command line:
	"player_speed 70"

	vb_data_add_control_slider_int_command("Number of enemies", 0, 8, 0, "enemy_count %f");

	When the slider is set to 7, this string will be sent to the command line:
	"enemy_count 7"
*/
vb_bool vb_data_add_control_button_command(const char* name, const char* command);
vb_bool vb_data_add_control_slider_float_command(const char* name, float range_min, float range_max, int steps, const char* command);
vb_bool vb_data_add_control_slider_int_command(const char* name, int range_min, int range_max, int step_size, const char* command);

/*
	Register controls which monitor and update the data in the specified memory
	address. Viewback automatically checks the variable for changes and updates
	all clients.

	A value of NULL for the address is valid. If your data occassionally goes
	away and comes back (say, it is freed and reallocated for some reason) then
	you can use the "update" versions of these procedures to set the address to
	NULL and then reset it to the new value and everything will work just fine.

	Other options are the same as above.

	Examples:

	vb_data_add_control_slider_float_address("Player max speed", 10, 100, 0, &player->speed);
	vb_data_add_control_slider_int_command("Number of enemies", 0, 8, 0, &game->num_enemies);
*/
vb_bool vb_data_add_control_slider_float_address(const char* name, float range_min, float range_max, int steps, float* address);
vb_bool vb_data_add_control_slider_int_address(const char* name, int range_min, int range_max, int step_size, int* address);

// During setup these procedures set the initial value of the control sliders.
// During runtime these procedures update the clients with the new values.
vb_bool vb_data_set_control_slider_float_value(const char* name, float value);
vb_bool vb_data_set_control_slider_int_value(const char* name, int value);

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
	These methods create controls that the user can modify on the monitor. They
	are designed to be called after vb_server_create(). If you want to create
	a control before vb_server_create(), use vb_data_add_control_*.

	WARNING: The name you pass in is a unique identifier for this const. If you
	change the name you are creating a new const.

	These procedures will only work if automatic memory allocation is used - ie
	if NULL is passed as the memory in vb_config_install(). The initial_value
	parameter is used only on the first load, after that the value is ignored
	and read from persistent storage.

	if automatic memory management is used:
		if this is the first time running this procedure:
			return initial_value
		otherwise if the value is in persistent storage:
			return the user's stored value
		otherwise:
			return initial_value
	otherwise:
		return initial_value
*/
int vb_const_int(const char* name, int initial_value);
float vb_const_float(const char* name, float initial_value);

/*
	Any text that goes to your console can also be piped into Viewback for
	display in the monitor. The text is sent immediately so transient storage
	is OK.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_console_append(const char* text);

/*
	Set the status text. Unlike the console, the status text doesn't append,
	it just shows whatever is in the status. Good for fps and current assets
	loaded and that sort of thing. The text is sent immediately so transient
	storage is OK.
	Returns 1 on success, 0 on failure.
*/
vb_bool vb_status_set(const char* text);

/*
	If you had to reload the library you can use these to reset the static
	pointers that Viewback uses internally.
*/
void vb_static_retrieve(void** p1, void** p2);
void vb_static_reset(void* p1, void* p2);


#ifdef __cplusplus
}
#endif
