#include <viewback.h>

#ifdef _WIN32
#include <winsock2.h>
#include <sys/types.h>
#include <sys/timeb.h>
#endif

#ifdef __linux__
#include <stdio.h>
#include <alloca.h>
#include <stdlib.h>
#include <sys/timeb.h>
#endif

#include <memory>
#include <time.h>

#include <string>
#include <sstream>
#include <cstring>

using namespace std;

void command_callback(const char* text)
{
	string s;
	s += "] ";
	s += text;
	s += "\n";

	printf("From client: %s", s.c_str());

	// Send it back down the wire as console output so that the monitor can see it arrived.
	vb_console_append(s.c_str());
}

void debug_printf(const char* text)
{
	printf("%s", text);
}


float RemapVal(float flInput, float flInLo, float flInHi, float flOutLo, float flOutHi)
{
	return (((flInput - flInLo) / (flInHi - flInLo)) * (flOutHi - flOutLo)) + flOutLo;
}

int main(int argc, const char** args)
{
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
		return 1;
#endif

	vb_config_t config;
	vb_config_initialize(&config);
	config.num_data_channels = 10;
	config.num_data_groups = 3;
	config.num_data_group_members = 8;
	config.num_data_labels = 4;
	config.debug_output_callback = &debug_printf;
	config.command_callback = &command_callback;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(args[i], "--port") == 0 && i < argc - 1)
		{
			i++;
			config.port = atoi(args[i]);
		}
		else if (strcmp(args[i], "--multicast-group") == 0 && i < argc - 1)
		{
			i++;
			config.multicast_group = args[i];
		}
	}

	size_t memory_required = vb_config_get_memory_required(&config);

	// User is responsible for freeing this memory.
	// Here I use stack allocation so that it's automatically
	// freed when the function returns. If you don't
	// understand what that means, don't use alloca(),
	// use some other memory allocation means instead.
	void* memory = alloca(memory_required);

	if (!vb_config_install(&config, memory, memory_required))
	{
		printf("Couldn't initialize Viewback.\n");
		return 1;
	}

	vb_channel_handle_t vb_keydown, vb_player, vb_health, vb_mousepos;

	vb_channel_handle_t vb1, vb2, vb3, vb4, vb5, vb6;

	if (!vb_data_add_channel("Key down", VB_DATATYPE_INT, &vb_keydown) ||
		!vb_data_add_channel("Player", VB_DATATYPE_INT, &vb_player) ||
		!vb_data_add_channel("Health", VB_DATATYPE_FLOAT, &vb_health) ||
		!vb_data_add_channel("Mouse", VB_DATATYPE_VECTOR, &vb_mousepos) ||
		!vb_data_add_channel("Test1", VB_DATATYPE_FLOAT, &vb1) ||
		!vb_data_add_channel("Test2", VB_DATATYPE_FLOAT, &vb2) ||
		!vb_data_add_channel("Test3", VB_DATATYPE_FLOAT, &vb3) ||
		!vb_data_add_channel("Test4", VB_DATATYPE_VECTOR, &vb4) ||
		!vb_data_add_channel("Test5", VB_DATATYPE_VECTOR, &vb5) ||
		!vb_data_add_channel("Test6", VB_DATATYPE_VECTOR, &vb6))
	{
		printf("Couldn't register data channels\n");
		return 1;
	}

	vb_group_handle_t vb_group1, vb_group2, vb_group3;
	if (!vb_data_add_group("Group1", &vb_group1) ||
		!vb_data_add_group("Group2", &vb_group2) ||
		!vb_data_add_group("Group3", &vb_group3))
	{
		printf("Couldn't register data groups\n");
		return 1;
	}

	if (!vb_data_add_channel_to_group(vb_group1, vb_keydown) ||
		!vb_data_add_channel_to_group(vb_group1, vb_player) ||
		!vb_data_add_channel_to_group(vb_group1, vb_health) ||
		!vb_data_add_channel_to_group(vb_group2, vb_player) ||
		!vb_data_add_channel_to_group(vb_group2, vb_health) ||
		!vb_data_add_channel_to_group(vb_group2, vb_mousepos) ||
		!vb_data_add_channel_to_group(vb_group3, vb_keydown) ||
		!vb_data_add_channel_to_group(vb_group3, vb_mousepos))
	{
		printf("Couldn't set up groups\n");
		return 1;
	}

	if (!vb_data_add_label(vb_player, 0, "Dead") ||
		!vb_data_add_label(vb_player, 1, "Alive") ||
		!vb_data_add_label(vb_player, 2, "Transient") ||
		!vb_data_add_label(vb_player, 3, "Philosophical"))
	{
		printf("Couldn't register labels\n");
		return 1;
	}

	// if (!vb_data_set_range(vb_health, 0, 150)) {}

	if (!vb_server_create())
	{
		printf("Couldn't create Viewback server\n");
		return 1;
	}

	time_t initial_time;
	time(&initial_time);

	srand((int)initial_time);

	int key_down = 0;
	int player = 0;
	float health = 100;
	float mouse_x = 0;
	float mouse_y = 0;

	float t1 = 100;
	float t2 = 100;
	float t3 = 100;
	float t4x = 0;
	float t4y = 0;
	float t5x = 0;
	float t5y = 0;
	float t6x = 0;
	float t6y = 0;

	struct timeb initial_time_millis;
	ftime(&initial_time_millis);

	struct timeb last_update;
	last_update.time = 0;
	last_update.millitm = 0;

	// Pretend the game has been running for a year, to locate deep run problems.
	initial_time_millis.time -= 60 * 60 * 24 * 365;

	while (true)
	{
		struct timeb current_time_millis;
		ftime(&current_time_millis);

#ifdef VIEWBACK_TIME_DOUBLE
		double current_time_double = (current_time_millis.time - initial_time_millis.time) + (double)(current_time_millis.millitm - initial_time_millis.millitm) / 1000;
		vb_server_update(current_time_double);
#else
		vb_uint64 current_time_ms = (current_time_millis.time - initial_time_millis.time)*1000 + (current_time_millis.millitm - initial_time_millis.millitm);
		vb_server_update(current_time_ms);
#endif

		double last_update_double = (current_time_millis.time - last_update.time) + (double)(current_time_millis.millitm - last_update.millitm) / 1000;
		if (last_update_double < 0.2)
			continue;

		ftime(&last_update);

		if (rand() % 4 == 0)
			health += RemapVal((float)(rand() % 100), 0, 99, -1, 1);

		mouse_x += RemapVal((float)(rand() % 100), 0, 99, -1, 1)*10;
		mouse_y += RemapVal((float)(rand() % 100), 0, 99, -1, 1)*10;

		mouse_x *= 0.99f;
		mouse_y *= 0.99f;

		if (rand() % 4 == 0)
			t1 += RemapVal((float)(rand() % 100), 0, 99, -1, 1);
		if (rand() % 4 == 0)
			t2 += RemapVal((float)(rand() % 100), 0, 99, -1, 1);
		if (rand() % 4 == 0)
			t3 += RemapVal((float)(rand() % 100), 0, 99, -1, 1);

		t4x += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		t4y += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		t5x += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		t5y += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		t6x += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		t6y += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;

		t4x *= 0.99f;
		t4y *= 0.99f;
		t5x *= 0.99f;
		t5y *= 0.99f;
		t6x *= 0.99f;
		t6y *= 0.99f;

		key_down = rand() % 2;
		player = rand() % 4;

		printf("Key down: %d\n", key_down);
		printf("Health: %f\n", health);
		printf("Mouse: <%f, %f>\n", mouse_x, mouse_y);

		const char* label;
		if (vb_data_get_label(vb_player, player, &label))
			printf("Player is %s\n", label);
		else
			printf("Player state: %d\n", player);

		bool success = true;
		if (!vb_data_send_int(vb_keydown, key_down))
			success = false;
		if (!vb_data_send_float(vb_health, health))
			success = false;
		if (!vb_data_send_vector(vb_mousepos, mouse_x, mouse_y, 0))
			success = false;
		if (!vb_data_send_int(vb_player, player))
			success = false;

		if (!vb_data_send_float(vb1, t1))
			success = false;
		if (!vb_data_send_float(vb2, t2))
			success = false;
		if (!vb_data_send_float(vb3, t3))
			success = false;
		if (!vb_data_send_vector(vb4, t4x, t4y, 0))
			success = false;
		if (!vb_data_send_vector(vb5, t5x, t5y, 0))
			success = false;
		if (!vb_data_send_vector(vb6, t6x, t6y, 0))
			success = false;

		time_t current_time;
		time(&current_time);

		// Send console output once per second.
		if (current_time == initial_time)
			continue;

		initial_time = current_time;

		if (rand() % 2 == 0)
		{
			const char* output;
			switch (rand() % 5)
			{
			case 0:
				output = ("Loading.\n");
				break;

			case 1:
				output = ("Reticulating splines.\n");
				break;

			case 2:
				output = ("Player \"GabeN\" connected (127.103.92.1)\n");
				break;

			case 3:
				output = ("Player \"CliffyB\" connected (127.63.210.1)\n");
				break;

			case 4:
				output = ("Player \"gooseman\" connected (127.45.150.1)\n");
				break;
			}

			printf("Sending console output: %s", output);
			if (!vb_console_append(output))
				printf("Couldn't append console output.\n");
		}

		if (!success)
			printf("Could not send data\n");

		ostringstream s;
		s << "FPS: ";
		s << (float)(rand() % 1000)/100 + 60;

		vb_status_set(s.str().c_str());
	}

	vb_server_shutdown();

#ifdef _WIN32
	WSACleanup();
#endif
}
