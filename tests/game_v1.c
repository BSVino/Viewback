// This code is in the public domain. No warranty implied, use at your own risk.

#include <viewback.h>

#ifdef _WIN32
#include <winsock2.h>
#include <sys/types.h>
#include <sys/timeb.h>

#define snprintf _snprintf
#endif

int main()
{
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
		return 1;
#endif

	vb_config_t config;
	vb_config_initialize(&config);

	// Here you specify how many of each feature you will use.
	config.num_data_channels = 2;
	config.num_data_groups = 1;
	config.num_data_group_members = 1;

	size_t memory_size = vb_config_get_memory_required(&config);

	if (!vb_config_install(&config, 0, 0))
		return 0;

	// Since 2 channels were specified in the config, you can call
	// vb_data_add_channel() at most twice before it will return
	// an error.
	vb_channel_handle_t channel;
	if (!vb_data_add_channel("Test", VB_DATATYPE_INT, &channel))
		return 0;

	vb_group_handle_t group;
	vb_data_add_group("TestGroup", &group);
	vb_data_add_channel_to_group(group, channel);

	vb_server_create();

	for (int i = 0; i < 1000; i++)
	{
		vb_server_update(i);

		if (!vb_data_send_int(channel, rand()))
			printf("Error sending to Viewback channel\n");
	}

	vb_server_shutdown();
}
