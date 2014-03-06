#include <viewback.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <memory>
#include <time.h>

int main()
{
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
		return 1;
#endif

	vb_config_t config;
	vb_config_initialize(&config);
	config.num_data_registrations = 4;
	config.num_data_labels = 4;

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

	vb_data_handle_t vb_keydown, vb_player, vb_health, vb_mousepos;

	if (!vb_data_register("Key down", VB_DATATYPE_INT, &vb_keydown) ||
		!vb_data_register("Player", VB_DATATYPE_INT, &vb_player) ||
		!vb_data_register("Health", VB_DATATYPE_FLOAT, &vb_health) ||
		!vb_data_register("Mouse", VB_DATATYPE_VECTOR, &vb_mousepos))
	{
		printf("Couldn't register data events\n");
		return 1;
	}

	if (!vb_data_label(vb_player, 0, "Dead") ||
		!vb_data_label(vb_player, 1, "Alive") ||
		!vb_data_label(vb_player, 2, "Transient") ||
		!vb_data_label(vb_player, 3, "Philosophical"))
	{
		printf("Couldn't register labels\n");
		return 1;
	}

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

	while (true)
	{
		vb_server_update();

		time_t current_time;
		time(&current_time);

		// Update once per second.
		if (current_time == initial_time)
			continue;

		// TODO: See what happens when it updates viewback as fast as it can instead of only once per second.

		initial_time = current_time;

		health += (float)(rand()%100)/50-1;

		mouse_x += ((float)(rand()%100)/50-1)*10;
		mouse_y += ((float)(rand()%100)/50-1)*10;

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

		if (!success)
			printf("Could not send data\n");
	}

	vb_server_shutdown();

#ifdef _WIN32
	WSACleanup();
#endif
}
