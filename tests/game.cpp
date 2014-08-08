// This code is in the public domain. No warranty implied, use at your own risk.

#include <viewback_util.h>

#ifdef _WIN32
#include <winsock2.h>
#include <sys/types.h>
#include <sys/timeb.h>

#define snprintf _snprintf
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
#include <vector>
#include <sstream>
#include <cstring>

#pragma warning(disable:4702) // unreachable code. The last part of main() is unreachable, which is okay since this is just a sample.

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

bool g_paused = false;

void pause_callback()
{
	g_paused = !g_paused;

	if (g_paused)
		vb_console_append("Paused.\n");
	else
		vb_console_append("Unpaused.\n");
}

void difficulty_callback(float difficulty)
{
	ostringstream s;
	s << "Difficulty set to: ";
	s << difficulty;
	s << "\n";

	vb_console_append(s.str().c_str());
}

void brightness_callback(float brightness)
{
	ostringstream s;
	s << "Brightness set to: ";
	s << brightness;
	s << "\n";

	vb_console_append(s.str().c_str());
}

void bots_callback(int bots)
{
	ostringstream s;
	s << "Number of bots set to: ";
	s << bots;
	s << "\n";

	vb_console_append(s.str().c_str());
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

	vb_util_initialize();

	unsigned short port = 0;
	const char* multicast_group = NULL;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(args[i], "--port") == 0 && i < argc - 1)
		{
			i++;
			port = (unsigned short)atoi(args[i]);
		}
		else if (strcmp(args[i], "--multicast-group") == 0 && i < argc - 1)
		{
			i++;
			multicast_group = args[i];
		}
	}

	vb_channel_handle_t vb_keydown, vb_player, vb_health, vb_mousepos;

	vb_channel_handle_t vb1, vb2, vb3, vb4, vb5, vb6;

	vb_util_add_channel("Key down", VB_DATATYPE_INT, &vb_keydown);
	vb_util_add_channel("Player", VB_DATATYPE_INT, &vb_player);
	vb_util_add_channel("Health", VB_DATATYPE_FLOAT, &vb_health);
	vb_util_add_channel("Mouse", VB_DATATYPE_VECTOR, &vb_mousepos);
	vb_util_add_channel("Test1", VB_DATATYPE_FLOAT, &vb1);
	vb_util_add_channel("Test2", VB_DATATYPE_FLOAT, &vb2);
	vb_util_add_channel("Test3", VB_DATATYPE_FLOAT, &vb3);
	vb_util_add_channel("Test4", VB_DATATYPE_VECTOR, &vb4);
	vb_util_add_channel("Test5", VB_DATATYPE_VECTOR, &vb5);
	vb_util_add_channel("Test6", VB_DATATYPE_VECTOR, &vb6);

	int iArraySize = 10;

	vector<vb_channel_handle_t> avbArray;
	avbArray.resize(iArraySize);

	vector<string> asNames;
	asNames.resize(iArraySize);

	vb_group_handle_t vb_group_array;
	vb_util_add_group("Array", &vb_group_array);

	for (int i = 0; i < iArraySize; i++)
	{
		char szNumber[2] = "0";
		szNumber[0] += (char)i;
		asNames[i] = string("array") + szNumber;
		vb_util_add_channel(asNames[i].c_str(), VB_DATATYPE_INT, &avbArray[i]);
		vb_util_add_channel_to_group_s("Array", asNames[i].c_str());
	}

	vb_group_handle_t vb_group1, vb_group2, vb_group3;
	
	vb_util_add_group("Group1", &vb_group1);
	vb_util_add_group("Group2", &vb_group2);
	vb_util_add_group("Group3", &vb_group3);

	if (!vb_util_add_channel_to_group_s("Group1", "Key down") ||
		!vb_util_add_channel_to_group_s("Group1", "Player") ||
		!vb_util_add_channel_to_group_s("Group1", "Health") ||
		!vb_util_add_channel_to_group_s("Group2", "Player") ||
		!vb_util_add_channel_to_group_s("Group2", "Health") ||
		!vb_util_add_channel_to_group_s("Group2", "Mouse") ||
		!vb_util_add_channel_to_group_s("Group3", "Key down") ||
		!vb_util_add_channel_to_group_s("Group3", "Mouse"))
	{
		printf("Couldn't set up groups\n");
		return 1;
	}

	if (!vb_util_add_label_s("Player", 0, "Dead") ||
		!vb_util_add_label_s("Player", 1, "Alive") ||
		!vb_util_add_label_s("Player", 2, "Transient") ||
		!vb_util_add_label_s("Player", 3, "Philosophical"))
	{
		printf("Couldn't register labels\n");
		return 1;
	}

#ifndef VB_NO_RANGE
	if (!vb_util_set_range_s("Health", 0, 150))
	{
		printf("Couldn't set range\n");
		return 1;
	}
#endif

	vb_util_add_control_button("Pause", &pause_callback);
	vb_util_add_control_slider_float("Difficulty", 0, 10, 21, &difficulty_callback);
	vb_util_add_control_slider_float("Brightness", 0, 1, 0, &brightness_callback);
	vb_util_add_control_slider_int("Bots", 0, 6, 1, &bots_callback);

	vb_util_set_multicast_group(multicast_group);
	vb_util_set_tcp_port(port);
	vb_util_set_output_callback(&debug_printf);
	vb_util_set_command_callback(&command_callback);

	if (!vb_util_server_create("Viewback Test Server"))
	{
		printf("Couldn't install config\n");
		return 1;
	}

	// Initialize
	for (int i = 0; i < iArraySize; i++)
		vb_data_send_int(avbArray[i], 0);

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

	for (;;)
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

		if (g_paused)
			continue;

		ftime(&last_update);

		if (rand() % 4 == 0)
			health += RemapVal((float)(rand() % 100), 0, 99, -1, 1);

		mouse_x += RemapVal((float)(rand() % 100), 0, 99, -1, 1)*10;
		mouse_y += RemapVal((float)(rand() % 100), 0, 99, -1, 1)*10;

		mouse_x *= 0.99f;
		mouse_y *= 0.99f;

		if (rand() % 40 == 0)
			t1 += RemapVal((float)(rand() % 100), 0, 99, -1, 1);
		if (rand() % 4 == 0)
			t2 += RemapVal((float)(rand() % 100), 0, 99, -1, 1);
		t3 += RemapVal((float)(rand() % 100), 0, 99, -1, 1);

		if (rand() % 40 == 0)
		{
			t4x += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
			t4y += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		}

		if (rand() % 4 == 0)
		{
			t5x += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
			t5y += RemapVal((float)(rand() % 100), 0, 99, -1, 1) * 10;
		}

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
		if (!vb_data_send_int_s("Player", player))
			success = false;

		if (!vb_data_send_float_s("Test1", t1))
			success = false;
		if (!vb_data_send_float(vb2, t2))
			success = false;
		if (!vb_data_send_float(vb3, t3))
			success = false;
		if (!vb_data_send_vector_s("Test4", t4x, t4y, 0))
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

		int array_index = current_time % iArraySize;
		int array_value = rand() % 10;
		printf("Array[%d] = %d\n", array_index, array_value);
		if (!vb_data_send_int(avbArray[array_index], array_value))
			success = false;

		initial_time = current_time;

		if (rand() % 2 == 0)
		{
			const char* output;
			switch (rand() % 5)
			{
			default:
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
	vb_config_release();

#ifdef _WIN32
	WSACleanup();
#endif
}
