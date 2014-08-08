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

#include "viewback.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	Viewback Util - A collection of functions to help set up Viewback

	While none of the functions in viewback.h will ever allocate memory
	on the heap, if you don't care about that then you can use the functions
	in this file to make life easier for yourself. They're just a more
	convenient wrapper around viewback.h
*/

/*
	Sample code follows. For more complex examples, see tests/game.cpp


	vb_util_initialize();

	vb_channel_handle_t channel;
	vb_util_add_channel("Test", VB_DATATYPE_INT, &channel);

	vb_util_server_create("Test Server");

	while (game_running())
	{
		vb_server_update(game_time);

		if (!vb_data_send_int(channel, rand()))
			printf("Error sending to Viewback channel\n");
	}

	vb_server_shutdown();
*/

/*
	Call this first. If can call this again to clear out a previous config.
*/
void vb_util_initialize();

/*
	Register a channel of data. You should provide an address to a handle and
	store that handle somewhere.
*/
void vb_util_add_channel(const char* name, vb_data_type_t type, /*out*/ vb_channel_handle_t* handle);

/*
	Register a group of data. You should provide an address to a handle and store
	that handle somewhere.
*/
void vb_util_add_group(const char* name, /*out*/ vb_group_handle_t* handle);

/*
	Add the specified channel of data to the specified group.

	The string version performs a linear search for the specified group and
	channel and returns 0 if they couldn't be found, 1 otherwise.
*/
void vb_util_add_channel_to_group(vb_group_handle_t group, vb_channel_handle_t channel);
vb_bool vb_util_add_channel_to_group_s(const char* group, const char* channel);

/*
	Register a label for integers. When the specified data has the specified value
	the monitor will give it the specified label. Think enumerations. Example:
	vb_data_label(vb_player_state, 0, "Dead");
	vb_data_label(vb_player_state, 1, "Alive");
	vb_data_label(vb_player_state, 2, "Hungry");
	vb_data_label(vb_player_state, 3, "Ephemeral");

	The string version performs a linear search for the specified channel and
	returns 0 if it couldn't be found, 1 otherwise.
*/
void vb_util_add_label(vb_channel_handle_t handle, int value, const char* label);
vb_bool vb_util_add_label_s(const char* channel, int value, const char* label);

#ifndef VB_NO_RANGE
/*
	If you set this, the monitor will fix the range to the specified values.
	Otherwise the chart will automatically fit the window. For vector data,
	only the max is used.

	The string version performs a linear search for the specified channel and
	returns 0 if it couldn't be found, 1 otherwise.
*/
void vb_util_set_range(vb_channel_handle_t handle, float range_min, float range_max);
vb_bool vb_util_set_range_s(const char* channel, float range_min, float range_max);
#endif

/*
	Register a control, a more convenient way to send commands to the game.
	For more info see the notes in voteback.h for vb_data_add_control_button().
*/
void vb_util_add_control_button(const char* name, vb_control_button_callback callback);
void vb_util_add_control_slider_float(const char* name, float range_min, float range_max, int steps, vb_control_slider_float_callback callback);
void vb_util_add_control_slider_int(const char* name, int range_min, int range_max, int step_size, vb_control_slider_int_callback callback);

// For info on these advanced commands see the vb_config_t structure in viewback.h
void vb_util_set_max_connections(unsigned char max_connections);
void vb_util_set_output_callback(vb_debug_output_callback output);
void vb_util_set_command_callback(vb_command_callback command);
void vb_util_set_multicast_group(const char* multicast_group);
void vb_util_set_multicast_port(unsigned short multicast_port);
void vb_util_set_tcp_port(unsigned short tcp_port);

/*
	Install the config. This function frees any memory allocated by the above functions.
	server_name will appear in the server list as clients scan for servers, for
	more info see the vb_config_t structure in viewback.h
	Returns 0 on failure, 1 on success.
*/
vb_bool vb_util_server_create(const char* server_name);

#ifdef __cplusplus
}
#endif
