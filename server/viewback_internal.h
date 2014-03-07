#pragma once




// ============= Protobuf stuff =============

// Haha don't do this in a function or it will be quickly freed, it's alloca. :)
#ifdef _DEBUG
#define Packet_alloca(length) alloca(length + 1024)
#else
// Add sizeof(size_t) bytes because we're going to prepend the length of the message.
#define Packet_alloca(length) alloca(length + sizeof(size_t))
#endif

struct Data {
	unsigned long  _handle;
	vb_data_type_t _type; // Won't get sent over the wire, it's needed to tell which data to send.
	unsigned long  _data_int;
	float          _data_float;
	float          _data_float_x;
	float          _data_float_y;
	float          _data_float_z;
};

struct DataRegistration {
	int            _field_name_len;
	const char*    _field_name;
	vb_data_type_t _type;
	unsigned long  _handle;
};

struct DataLabel {
	unsigned long  _handle;
	int            _value;
	int            _field_name_len;
	const char*    _field_name;
};

#define MAX_REPEATED_LENGTH 100

struct Packet {
	struct Data*             _data;
	int                      _data_registrations_repeated_len;
	struct DataRegistration* _data_registrations;
	int                      _data_labels_repeated_len;
	struct DataLabel*        _data_labels;
	int            _console_output_len;
	const char*    _console_output;
};

void Packet_initialize(struct Packet* packet);
void Packet_initialize_data(struct Packet* packet, struct Data* data, vb_data_type_t type);
void Packet_initialize_registrations(struct Packet* packet, struct DataRegistration* data_reg, size_t registrations, struct DataLabel* data_labels, size_t labels);
size_t Packet_get_message_size(struct Packet *_Packet);
size_t Packet_serialize(struct Packet *_Packet, void *_buffer, size_t length);




// ============= Viewback stuff =============

typedef struct
{
	const char*    name;
	vb_data_type_t type;
} vb_data_registration_t;

typedef struct
{
	vb_data_handle_t handle;
	int              value;
	const char*      name;
} vb_data_label_t;

typedef struct
{
	vb_socket_t socket;
} vb_connection_t;

typedef struct
{
	vb_config_t config;

	vb_socket_t             multicast_socket;
	struct sockaddr_in      multicast_addr;
	time_t                  last_multicast;

	vb_socket_t             tcp_socket;

	vb_data_registration_t* registrations;
	int                     next_registration;

	vb_data_label_t* labels;
	int              next_label;

	vb_connection_t*        connections;

	bool     server_active;

	vb_command_callback command_callback;
} vb_t;
