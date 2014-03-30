#pragma once

static_assert(sizeof(unsigned long long int) == 8, "unsigned long long int must be 64 bits.");



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

#ifdef VIEWBACK_TIME_DOUBLE
	double                 _time_double;
#else
	unsigned long long int _time_uint64;
#endif
};

struct DataChannel {
	int            _field_name_len;
	const char*    _field_name;
	vb_data_type_t _type;
	unsigned long  _handle;
	float          _min;
	float          _max;
};

struct DataGroup {
	const char*    _name;
	int            _name_len;
	unsigned long* _channels;
	int            _channels_repeated_len;
};

struct DataLabel {
	unsigned long  _handle;
	int            _value;
	int            _field_name_len;
	const char*    _field_name;
};

struct Packet {
	struct Data*        _data;
	int                 _data_channels_repeated_len;
	struct DataChannel* _data_channels;
	int                 _data_groups_repeated_len;
	struct DataGroup*   _data_groups;
	int                 _data_labels_repeated_len;
	struct DataLabel*   _data_labels;

	int            _console_output_len;
	const char*    _console_output;

	int            _status_len;
	const char*    _status;
};

void Packet_initialize(struct Packet* packet);
void Packet_initialize_data(struct Packet* packet, struct Data* data, vb_data_type_t type);
void Packet_initialize_registrations(struct Packet* packet, struct DataChannel* data_channels, size_t channels, struct DataGroup* data_groups, size_t groups, struct DataLabel* data_labels, size_t labels);
size_t Packet_get_message_size(struct Packet *_Packet);
size_t Packet_serialize(struct Packet *_Packet, void *_buffer, size_t length);




// ============= Viewback stuff =============

typedef struct
{
	const char*    name;
	vb_data_type_t type;
	float          range_min;
	float          range_max;
} vb_data_channel_t;

typedef struct
{
	const char* name;
} vb_data_group_t;

typedef struct
{
	vb_group_handle_t   group;
	vb_channel_handle_t channel;
} vb_data_group_member_t;

typedef struct
{
	vb_channel_handle_t handle;
	int                 value;
	const char*         name;
} vb_data_label_t;

typedef struct
{
	vb_socket_t socket;
} vb_connection_t;

typedef struct
{
	vb_config_t config;

	vb_socket_t        multicast_socket;
	struct sockaddr_in multicast_addr;
	time_t             last_multicast;
	vb_socket_t        tcp_socket;

	vb_data_channel_t* channels;
	size_t             next_channel;

	vb_data_group_t* groups;
	size_t           next_group;

	vb_data_group_member_t* group_members;
	size_t                  next_group_member;

	vb_data_label_t* labels;
	size_t           next_label;

	vb_connection_t* connections;
	bool             server_active;

#ifdef VIEWBACK_TIME_DOUBLE
	double    current_time;
#else
	vb_uint64 current_time_ms;
#endif
} vb_t;
