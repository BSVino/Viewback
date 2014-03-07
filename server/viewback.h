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

typedef struct {
	int port;
	int num_data_registrations;
	int num_data_labels;
	int max_connections;
} vb_config_t;

typedef int vb_data_handle_t;

/* If you change this, update it in viewback.proto as well. */
typedef enum
{
	VB_DATATYPE_NONE = 0,
	VB_DATATYPE_INT = 1,
	VB_DATATYPE_FLOAT = 2,
	VB_DATATYPE_VECTOR = 3,
} vb_data_type_t;

typedef void (*vb_command_callback)(const char* text);

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
	the config (which removes all registrations) if you like.
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
	This function will be called by Viewback any time a command comes in from
	the client to run.
*/
int vb_console_register_command_callback(vb_command_callback cmd);

#ifdef __cplusplus
}
#endif
