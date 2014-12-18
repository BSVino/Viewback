#pragma once

#pragma warning(disable: 4201) /* nonstandard extension used : nameless struct/union */

static_assert(sizeof(unsigned long long int) == 8, "unsigned long long int must be 64 bits.");

#ifdef VIEWBACK_TIME_DOUBLE
typedef double vb__time_t;
#else
typedef vb_uint64 vb__time_t;
#endif


// ============= Viewback stuff =============

#define CHANNEL_FLAG_INITIALIZED (1<<0)
// If you add more than 8, bump the size of vb_data_channel_t::flags

// This isn't really always 1 byte long. It's a bit mask large enough to hold
// all channels, so it may be longer.
typedef unsigned char vb__data_channel_mask_t;
#define VB_CHANNEL_NONE ((vb_channel_handle_t)~0)
#define VB_GROUP_NONE ((vb_group_handle_t)~0)

typedef struct
{
	const char*    name;
	vb_data_type_t type;

#ifndef VB_NO_RANGE
	float          range_min;
	float          range_max;
#endif

	unsigned char  flags; // CHANNEL_FLAG_*

#ifndef VB_NO_COMPRESSION
	// If this is nonzero it means that we threw out some redundant data. Next
	// time we send data to the client we should let it know we threw some out.
	vb__time_t     maintain_time;

	union
	{
		int   last_int;
		float last_float;
		struct
		{
			float last_float_x;
			float last_float_y;
			float last_float_z;
		};
	};
#endif
} vb__data_channel_t;

typedef struct
{
	const char* name;
} vb__data_group_t;

typedef struct
{
	vb_group_handle_t   group;
	vb_channel_handle_t channel;
} vb__data_group_member_t;

typedef struct
{
	vb_channel_handle_t handle;
	int                 value;
	const char*         name;
} vb__data_label_t;

typedef struct
{
	const char*  name;
	vb_control_t type;

	const char*  command;

	union
	{
		vb_control_button_callback       button_callback;
		vb_control_slider_float_callback slider_float_callback;
		vb_control_slider_int_callback   slider_int_callback;
	};

	union
	{
		struct
		{
			float range_min;
			float range_max;
			int   steps;
			float initial_value;
		} slider_float;

		struct
		{
			int range_min;
			int range_max;
			int step_size;
			int initial_value;
		} slider_int;
	};
} vb__data_control_t;

typedef struct
{
	vb__socket_t socket;

	vb__data_channel_mask_t* active_channels;
} vb__connection_t;

typedef struct
{
	vb_config_t config;

	vb__socket_t        multicast_socket;
	struct sockaddr_in  multicast_addr;
	time_t              last_multicast;
	vb__socket_t        tcp_socket;

	vb__data_channel_t* channels;
	size_t              next_channel;

	vb__data_group_t* groups;
	size_t            next_group;

	vb__data_group_member_t* group_members;
	size_t                   next_group_member;

	vb__data_label_t* labels;
	size_t            next_label;

	vb__data_control_t* controls;
	size_t              next_control;

	vb__connection_t* connections;
	char              server_active;

	vb__time_t        current_time;
} vb__t;





// ============= Protobuf stuff =============

// Haha don't do this in a function or it will be quickly freed, it's alloca. :)
#ifdef _DEBUG
#define Packet_alloca(name, length) vb__stack_allocate(char, name, length + 1024)
#else
// Add sizeof(size_t) bytes because we're going to prepend the length of the message.
#define Packet_alloca(name, length) vb__stack_allocate(char, name, length + sizeof(size_t))
#endif

struct vb__Data {
	unsigned long  _handle;
	vb_data_type_t _type; // Won't get sent over the wire, it's needed to tell which data to send.
	unsigned long  _data_int;
	float          _data_float;
	float          _data_float_x;
	float          _data_float_y;
	float          _data_float_z;

#ifdef VIEWBACK_TIME_DOUBLE
	double                 _time_double;
	double                 _maintain_time_double;
#else
	unsigned long long int _time_uint64;
	unsigned long long int _maintain_time_uint64;
#endif
};

struct vb__DataChannel {
	int            _field_name_len;
	const char*    _field_name;
	vb_data_type_t _type;
	unsigned long  _handle;

#ifndef VB_NO_RANGE
	float          _min;
	float          _max;
#endif
};

struct vb__DataGroup {
	const char*    _name;
	int            _name_len;
	unsigned long* _channels;
	int            _channels_repeated_len;
};

struct vb__DataLabel {
	unsigned long  _handle;
	int            _value;
	int            _field_name_len;
	const char*    _field_name;
};

struct vb__DataControl {
	int            _name_len;
	const char*    _name;
	vb_control_t   _type;
	float          _range_min_float;
	float          _range_max_float;
	unsigned int   _num_steps;
	float          _initial_float;
	int            _range_min_int;
	int            _range_max_int;
	unsigned int   _step_size;
	int            _initial_int;
};

struct vb__Packet {
	struct vb__Data*        _data;
	int                     _data_channels_repeated_len;
	struct vb__DataChannel* _data_channels;
	int                     _data_groups_repeated_len;
	struct vb__DataGroup*   _data_groups;
	int                     _data_labels_repeated_len;
	struct vb__DataLabel*   _data_labels;
	int                     _data_controls_repeated_len;
	struct vb__DataControl* _data_controls;

	int            _console_output_len;
	const char*    _console_output;

	int            _status_len;
	const char*    _status;

	int _is_registration;
};

void vb__Packet_initialize(struct vb__Packet* packet);
void vb__Packet_initialize_data(struct vb__Packet* packet, struct vb__Data* data, vb_data_type_t type);
void vb__Packet_initialize_registrations(struct vb__Packet* packet, struct vb__DataChannel* data_channels, size_t channels, struct vb__DataGroup* data_groups, size_t groups, struct vb__DataLabel* data_labels, size_t labels, struct vb__DataControl* data_controls, size_t controls);
size_t vb__Packet_get_message_size(struct vb__Packet *_Packet);
size_t vb__Packet_serialize(struct vb__Packet *_Packet, void *_buffer, size_t length);




