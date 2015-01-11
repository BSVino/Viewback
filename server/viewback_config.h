#ifndef VIEWBACK_NO_CONFIG

#include <stdio.h>

#define VB__PARSE_REQUIRE(x, error) \
do { \
	int r = (x); \
	if (!r) { \
		VBPrintf("PARSE ERROR: Required a " error ".\n"); \
		return r; \
	} \
} while (0) \

#define VB__PARSE_EAT(x) \
do { \
	int r = vb__configfile_parse_eat(x); \
	if (!r) { VBPrintf("PARSE ERROR: Expected " #x ".\n"); return 0; }\
} while (0) \

typedef enum
{
	VB__TOKEN_UNKNOWN = -1,
	VB__TOKEN_EOF     = 0,
	VB__TOKEN_EOL,
	VB__TOKEN_TEXT,
	VB__TOKEN_CONTROLS,
	VB__TOKEN_VALUE,
	VB__TOKEN_OPEN_CURLY,
	VB__TOKEN_CLOSE_CURLY,
} vb__token;

static const char* vb__p;
static const char* vb__p_end;

static vb__token   vb__token_type;
static const char* vb__token_string;
static size_t      vb__token_length;

// Increase the size of vb__configfile_data_t::action if you add more than 8 of these.
#define CONFIGFILE_ACTION_LOAD_VB (1<<0) // Load the data into the VB pointer. VB will not be accessed if this flag is not set.

typedef struct
{
	vb_config_t* config;

	char action;

	size_t num_controls;
	size_t num_const_controls; // num_const_controls <= num_controls

} vb__configfile_data_t;

static vb__configfile_data_t* vb__configfile;

const char* vb__configfile_name()
{
	if (vb__configfile->config->config_file)
		return vb__configfile->config->config_file;

	return "viewback.cfg";
}

int vb__configfile_lex_text(char p)
{
	// These characters are delimiters, not considered text.
	if (p == ':' || p == '{' || p == '}')
		return 0;

	// Any other printable ASCII character is text.
	return (p >= ' ');
}

static vb__token vb__configfile_lex_next()
{
	if (vb__p == vb__p_end)
		return vb__token_type = VB__TOKEN_EOF;

	while ((*vb__p == ' ' || *vb__p == '\r' || *vb__p == '\t') && vb__p <= vb__p_end)
		vb__p++;

	if (*vb__p == '\n')
	{
		vb__token_type = VB__TOKEN_EOL;
		while (*vb__p == '\n' && vb__p <= vb__p_end)
			vb__p++;

		vb__token_length = vb__p - vb__token_string;

		return vb__token_type;
	}

	if (vb__p == vb__p_end)
		return vb__token_type = VB__TOKEN_EOF;

	vb__token_string = vb__p;

	if (vb__configfile_lex_text(*vb__p))
	{
		vb__token_type = VB__TOKEN_TEXT;
		while (vb__configfile_lex_text(*vb__p))
			vb__p++;

		vb__token_length = vb__p - vb__token_string;

		if (vb__strncmp(vb__token_string, "controls", vb__token_length, 8) == 0)
			return vb__token_type = VB__TOKEN_CONTROLS;

		return vb__token_type;
	}
	else if (*vb__p == ':')
		return vb__p++, vb__token_type = VB__TOKEN_VALUE;
	else if (*vb__p == '{')
		return vb__p++, vb__token_type = VB__TOKEN_OPEN_CURLY;
	else if (*vb__p == '}')
		return vb__p++, vb__token_type = VB__TOKEN_CLOSE_CURLY;

	return vb__token_type = VB__TOKEN_UNKNOWN;
}

static int vb__configfile_parse_eat(vb__token t)
{
	if (vb__token_type != t)
		return 0;

	vb__configfile_lex_next();
	return 1;
}

static int vb__configfile_parse_peek(vb__token t)
{
	return vb__token_type == t;
}

/*
	keyvalue <- text [ ":" text ] EOL
*/
int vb__configfile_parse_keyvalue(const char** key, int* key_len, const char** value, int* value_len)
{
	*key = vb__token_string;
	*key_len = vb__token_length;

	VB__PARSE_EAT(VB__TOKEN_TEXT);

	if (vb__configfile_parse_peek(VB__TOKEN_VALUE))
	{
		VB__PARSE_EAT(VB__TOKEN_VALUE);

		*value = vb__token_string;
		*value_len = vb__token_length;

		VB__PARSE_EAT(VB__TOKEN_TEXT);
	}
	else
	{
		*value_len = 0;
		*value = NULL;
	}

	VB__PARSE_EAT(VB__TOKEN_EOL);

	return 1;
}

/*
	control <- keyvalue ["{" EOL { keyvalue } "}" EOL]
*/
int vb__configfile_parse_control()
{
	const char* control_name;
	int control_name_len;
	const char* control_value;
	int control_value_len;

	int is_const_control = 0;

	int has_value = 0;
	int int_value;
	float float_value;

	vb_control_t control_type = VB_CONTROL_NONE;

	vb__configfile->num_controls++;

	if (!vb__configfile_parse_keyvalue(&control_name, &control_name_len, &control_value, &control_value_len))
		return 0;

	vb__control_handle_t handle = vb__data_find_control_by_name(control_name, control_name_len);

	if (vb__strncmp(control_value, "float", 5, control_value_len) == 0)
		control_type = VB_CONTROL_SLIDER_FLOAT;
	else if (vb__strncmp(control_value, "int", 3, control_value_len) == 0)
		control_type = VB_CONTROL_SLIDER_INT;

	if (vb__configfile->action & CONFIGFILE_ACTION_LOAD_VB && handle != VB_CONTROL_HANDLE_NONE)
		VBAssert(VB->controls[handle].type == control_type);

	if (vb__configfile_parse_peek(VB__TOKEN_OPEN_CURLY))
	{
		VB__PARSE_EAT(VB__TOKEN_OPEN_CURLY);
		VB__PARSE_EAT(VB__TOKEN_EOL);

		while (vb__configfile_parse_peek(VB__TOKEN_TEXT))
		{
			const char* key;
			int key_len;
			const char* value;
			int value_len;

			if (!vb__configfile_parse_keyvalue(&key, &key_len, &value, &value_len))
				return 0;

			if (vb__strncmp(key, "const", key_len, 5) == 0)
				is_const_control = 1;
			else if (vb__strncmp(key, "value", key_len, 5) == 0)
			{
				has_value = 1;
				switch (control_type)
				{
				case VB_CONTROL_SLIDER_FLOAT:
					float_value = (float)atof(value);
					break;

				case VB_CONTROL_SLIDER_INT:
					int_value = atoi(value);
					break;

				default:
					VBUnimplemented();
				}
			}
			else
				VBUnimplemented();
		}

		vb__configfile->num_const_controls += is_const_control;

		if (has_value && (vb__configfile->action & CONFIGFILE_ACTION_LOAD_VB))
		{
			switch (control_type)
			{
			case VB_CONTROL_SLIDER_FLOAT:
				if (is_const_control)
				{
					char* control_name_persistent = vb__alloc_autofree(vb__configfile->config, control_name_len + 1);
					strncpy(control_name_persistent, control_name, control_name_len);
					vb__const_float(control_name_persistent, control_name_len, float_value);
				}
				else
					vb__data_set_control_slider_float_value_h(handle, float_value);
				break;

			case VB_CONTROL_SLIDER_INT:
				if (is_const_control)
				{
					char* control_name_persistent = vb__alloc_autofree(vb__configfile->config, control_name_len + 1);
					strncpy(control_name_persistent, control_name, control_name_len);
					vb__const_int(control_name_persistent, control_name_len, int_value);
				}
				else
					vb__data_set_control_slider_int_value_h(handle, int_value);
				break;

			default:
				VBUnimplemented();
			}
		}

		VB__PARSE_EAT(VB__TOKEN_CLOSE_CURLY);
		VB__PARSE_EAT(VB__TOKEN_EOL);
	}

	return 1;
}

/*
	controls <- { control }
*/
int vb__configfile_parse_controls()
{
	while (vb__configfile_parse_peek(VB__TOKEN_TEXT))
		vb__configfile_parse_control();

	return 1;
}

/*
	global <- EOL | "controls" [EOL] "{" EOL controls "}" EOL
*/
int vb__configfile_parse_global()
{
	if (vb__configfile_parse_peek(VB__TOKEN_EOL))
	{
		VB__PARSE_EAT(VB__TOKEN_EOL);
		return 1;
	}

	if (vb__configfile_parse_peek(VB__TOKEN_CONTROLS))
	{
		VB__PARSE_EAT(VB__TOKEN_CONTROLS);

		if (vb__configfile_parse_peek(VB__TOKEN_EOL))
			VB__PARSE_EAT(VB__TOKEN_EOL);

		VB__PARSE_EAT(VB__TOKEN_OPEN_CURLY);
		VB__PARSE_EAT(VB__TOKEN_EOL);

		vb__configfile_parse_controls();

		VB__PARSE_EAT(VB__TOKEN_CLOSE_CURLY);
		VB__PARSE_EAT(VB__TOKEN_EOL);

		return 1;
	}

	return 1;
}

int vb__configfile_parse(const char* file_contents, int file_size)
{
	vb__p = file_contents;
	vb__p_end = file_contents + file_size;

	// Prime the pump
	vb__configfile_lex_next();

	while (!vb__configfile_parse_peek(VB__TOKEN_EOF))
	{
		if (vb__configfile_parse_peek(VB__TOKEN_UNKNOWN))
		{
			vb__stack_allocate(char, bad_token, vb__token_length + 1);
			strncpy(bad_token, vb__token_string, vb__token_length);
			bad_token[vb__token_length] = '\0';
			VBPrintf("Unknown token in config file: '%s'. Stopping config read.\n", bad_token);
			return 0;
		}

		VB__PARSE_REQUIRE(vb__configfile_parse_global(), "global statement");
	}

	return 1;
}

void vb__configfile_load(vb__configfile_data_t* data)
{
	vb__configfile = data;

	vb__configfile->num_controls = vb__configfile->num_const_controls = 0;

	const char* configfile = vb__configfile_name();

	FILE* fp = fopen(configfile, "r");

	if (!fp)
	{
		VBPrintf("Config file not found.\n");
		return;
	}

	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	vb__stack_allocate(char, file_contents, file_size);

	file_size = fread(file_contents, 1, file_size, fp);

	fclose(fp);

	if (vb__configfile_parse(file_contents, file_size))
		VBPrintf("Config file loaded: '%s'.\n", configfile);
}

void vb__configfile_write()
{
	static vb__configfile_data_t write_configfile;
	vb__configfile = &write_configfile;
	vb__configfile->config = &VB->config;

	const char* configfile = vb__configfile_name();

	FILE* fp = fopen(configfile, "w");

	if (!fp)
	{
		VBPrintf("Config file could not be opened for writing.\n");
		return;
	}

	fwrite("controls {\n", 1, 11, fp);
	for (size_t i = 0; i < VB->next_control; i++)
	{
		switch (VB->controls[i].type)
		{
		case VB_CONTROL_SLIDER_FLOAT:
			vb__sprintf("\t%s: float\n\t{\n%s\t\tvalue: %f\n\t}\n", VB->controls[i].name, ((VB->controls[i].flags&CONTROL_FLAG_CONSTANT)?"\t\tconst\n":""), VB->controls[i].slider_float.value);
			break;

		case VB_CONTROL_SLIDER_INT:
			vb__sprintf("\t%s: int\n\t{\n%s\t\tvalue: %d\n\t}\n", VB->controls[i].name, ((VB->controls[i].flags&CONTROL_FLAG_CONSTANT) ? "\t\tconst\n" : ""), VB->controls[i].slider_int.value);
			break;

		default:
			continue;
		}

		fwrite(vb__sprintf_buffer, 1, strlen(vb__sprintf_buffer), fp);
	}
	fwrite("}\n", 1, 2, fp);

	fclose(fp);
}

#endif

