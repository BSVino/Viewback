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
	Sample code follows.


	vb_util_initialize();

	vb_channel_handle_t channel;
	vb_util_add_channel("Test", VB_DATATYPE_INT, &channel);

	vb_util_server_create();

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
	Install the config. This function frees any memory allocated by the above functions.
	Returns 0 on failure, 1 on success.
*/
vb_bool vb_util_server_create(unsigned char max_connections, vb_debug_output_callback output, vb_command_callback command, const char* multicast_group, unsigned short port);

#ifdef __cplusplus
}
#endif
