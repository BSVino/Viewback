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
	Call this first.
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
	Don't forget to check the return value of the string version.
*/
void vb_util_add_channel_to_group(vb_group_handle_t group, vb_channel_handle_t channel);
int vb_util_add_channel_to_group_s(const char* group, const char* channel);

/*
	Register a label for integers. When the specified data has the specified value
	the monitor will give it the specified label. Think enumerations. Example:
	vb_data_label(vb_player_state, 0, "Dead");
	vb_data_label(vb_player_state, 1, "Alive");
	vb_data_label(vb_player_state, 2, "Hungry");
	vb_data_label(vb_player_state, 3, "Ephemeral");

	Don't forget to check the return value of the string version.
*/
void vb_util_add_label(vb_channel_handle_t handle, int value, const char* label);
int vb_util_add_label_s(const char* channel, int value, const char* label);

/*
	If you set this, the monitor will fix the range to the specified values.
	Otherwise the chart will automatically fit the window. For vector data,
	only the max is used. Don't forget to check the return value of the string
	version.
*/
void vb_util_set_range(vb_channel_handle_t handle, float range_min, float range_max);
int vb_util_set_range_s(const char* channel, float range_min, float range_max);

/*
	Install the config. This function frees any memory allocated by the above functions.
*/
int vb_util_server_create(unsigned char max_connections, vb_debug_output_callback output, vb_command_callback command, const char* multicast_group, unsigned short port);

#ifdef __cplusplus
}
#endif
