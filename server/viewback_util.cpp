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

#include "viewback_util.h"

#include <vector>
#include <string.h>
#include <stdlib.h>

#include "viewback_shared.h"
#include "viewback_internal.h"

using namespace std;

class CLabel
{
public:
	const char* name;
	int         value;
};

class CChannel
{
public:
	CChannel()
	{
#ifndef VB_NO_RANGE
		range_min = 0;
		range_max = 0;
#endif
	}

public:
	const char*    name;
	vb_data_type_t type;

#ifndef VB_NO_RANGE
	float range_min;
	float range_max;
#endif

	vector<CLabel> labels;
};

class CGroup
{
public:
	const char* name;

	vector<vb_channel_handle_t> channels;
};

static vector<CChannel> g_channels;
static vector<CGroup> g_groups;
static void* g_memory = NULL;

vb_channel_handle_t vb_util_find_channel(const char* name)
{
	for (size_t i = 0; i < g_channels.size(); i++)
	{
		if (strcmp(g_channels[i].name, name) == 0)
			return (vb_channel_handle_t)i;
	}

	return VB_CHANNEL_NONE;
}

vb_group_handle_t vb_util_find_group(const char* name)
{
	for (size_t i = 0; i < g_groups.size(); i++)
	{
		if (strcmp(g_groups[i].name, name) == 0)
			return (vb_group_handle_t)i;
	}

	return VB_GROUP_NONE;
}

void vb_util_initialize()
{
	vb_config_release();

	g_channels.clear();
	g_groups.clear();

	free(g_memory);
	g_memory = NULL;
}

void vb_util_add_channel(const char* name, vb_data_type_t type, /*out*/ vb_channel_handle_t* handle)
{
	CChannel c;
	c.name = name;
	c.type = type;

	g_channels.push_back(c);

	*handle = (vb_channel_handle_t)g_channels.size()-1;
}

void vb_util_add_group(const char* name, /*out*/ vb_group_handle_t* handle)
{
	CGroup g;
	g.name = name;

	g_groups.push_back(g);

	*handle = (vb_group_handle_t)g_groups.size()-1;
}

void vb_util_add_channel_to_group(vb_group_handle_t group, vb_channel_handle_t channel)
{
	g_groups[group].channels.push_back(channel);
}

int vb_util_add_channel_to_group_s(const char* group, const char* channel)
{
	vb_group_handle_t group_handle = vb_util_find_group(group);

	if (group_handle == VB_GROUP_NONE)
		return 0;

	vb_channel_handle_t channel_handle = vb_util_find_channel(channel);

	if (channel_handle == VB_CHANNEL_NONE)
		return 0;

	vb_util_add_channel_to_group(group_handle, channel_handle);

	return 1;
}

void vb_util_add_label(vb_channel_handle_t handle, int value, const char* label)
{
	CLabel l;
	l.name = label;
	l.value = value;

	g_channels[handle].labels.push_back(l);
}

int vb_util_add_label_s(const char* channel, int value, const char* label)
{
	vb_channel_handle_t handle = vb_util_find_channel(channel);

	if (handle == VB_CHANNEL_NONE)
		return 0;

	vb_util_add_label(handle, value, label);

	return 1;
}

#ifndef VB_NO_RANGE
void vb_util_set_range(vb_channel_handle_t handle, float range_min, float range_max)
{
	g_channels[handle].range_min = range_min;
	g_channels[handle].range_max = range_max;
}

int vb_util_set_range_s(const char* channel, float range_min, float range_max)
{
	vb_channel_handle_t handle = vb_util_find_channel(channel);

	if (handle == VB_CHANNEL_NONE)
		return 0;

	vb_util_set_range(handle, range_min, range_max);

	return 1;
}
#endif


// RAII class to free a vector's memory
template<typename T>
class CVectorEmancipator
{
public:
	CVectorEmancipator(vector<T>& v)
		: vec(v)
	{
	}

	~CVectorEmancipator()
	{
		vector<T> v;
		swap(vec, v);
	}

private:
	CVectorEmancipator operator=(CVectorEmancipator&) { VBUnimplemented(); }

	vector<T>& vec;
};

int vb_util_server_create(unsigned char max_connections, vb_debug_output_callback output, vb_command_callback command, const char* multicast_group, unsigned short port)
{
	vb_config_release();

	free(g_memory);
	g_memory = NULL;

	// Vector memory will free at the end of this function.
	CVectorEmancipator<CChannel> c(g_channels);
	CVectorEmancipator<CGroup> g(g_groups);

	vb_config_t config;

	vb_config_initialize(&config);

	config.max_connections = max_connections;
	config.multicast_group = multicast_group;
	config.port = port;
	config.debug_output_callback = output;
	config.command_callback = command;

	config.num_data_channels = g_channels.size();
	config.num_data_groups = g_groups.size();

	for (size_t i = 0; i < g_channels.size(); i++)
		config.num_data_labels += g_channels[i].labels.size();

	for (size_t i = 0; i < g_groups.size(); i++)
		config.num_data_group_members += g_groups[i].channels.size();

	size_t memory_size = vb_config_get_memory_required(&config);
	g_memory = malloc(memory_size);

	if (!vb_config_install(&config, g_memory, memory_size))
		return 0;

	for (size_t i = 0; i < g_channels.size(); i++)
	{
		auto& channel = g_channels[i];
		if (!vb_data_add_channel(channel.name, channel.type, nullptr))
			return 0;

#ifndef VB_NO_RANGE
		if (channel.range_min || channel.range_max)
		{
			if (!vb_data_set_range((vb_channel_handle_t)i, channel.range_min, channel.range_max))
				return 0;
		}
#endif

		for (size_t j = 0; j < channel.labels.size(); j++)
		{
			auto& label = channel.labels[j];
			if (!vb_data_add_label((vb_channel_handle_t)i, label.value, label.name))
				return 0;
		}
	}

	for (size_t i = 0; i < g_groups.size(); i++)
	{
		auto& group = g_groups[i];
		if (!vb_data_add_group(group.name, nullptr))
			return 0;

		for (size_t j = 0; j < group.channels.size(); j++)
		{
			if (!vb_data_add_channel_to_group((vb_group_handle_t)i, (vb_channel_handle_t)group.channels[j]))
				return 0;
		}
	}

	if (!vb_server_create())
		return 0;

	return 1;
}

