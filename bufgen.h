#ifndef BUFGEN_H
#define BUFGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUFGEN_COMMENT "Generated with BUFGEN by DRA"

enum bufgen_mode
{
	BUFGEN_MODE_ARRAY_INT = 0,
	BUFGEN_MODE_ARRAY_CHAR,
	BUFGEN_MODE_ARRAY_HEX,
	BUFGEN_MODE_STRING,
};

#define BUFGEN_VARIABLE_NAME_BUFFER "buffer"
#define BUFGEN_VARIABLE_NAME_CAPACITY "buffer_size"
#define BUFGEN_VARIABLE_NAME_LENGTH "buffer_length"

typedef struct {
	char const *input_file_name;
	char const *output_file_name;
	FILE *input_file;
	FILE *output_file;
	char *input_file_buffer;
	size_t input_file_bytes;
	size_t output_file_bytes;
	int mode;
	bool has_newlines;
	bool has_comments;
	bool has_variables;
	bool has_nul_terminator;
} bufgen_data;
/*
	char const *input_file_name;
	char const *output_file_name;
*/
/*
	maybe add 2 strings as parameters to determine the output buffer's data type and the length variables data types.
	example:
	char const *output_buffer_type; // this would be equal to something like "static char const * const" or whatever (need to think about how to handle if i want it to be a char [] due to var name placement)
	char const *output_length_type; // this would be equal to something like "size_t" or whatever
	
	also need a mode to get the raw string without a variable name so that I can assign it to a variable within a different file.
*/


size_t bufgen_get_file_bytes(char const *filename);
bool bufgen_is_printable_ascii(unsigned char byte);

/*
int bufgen_init(bufgen_data *data, char const *input_file_name, char const *output_file_name);
void bufgen_free(bufgen_data *data);
*/

int bufgen_generate_file(char const *input_file_name, char const *output_file_name, int *read_bytes, int *write_bytes, int mode, bool has_newlines, bool has_comments, bool has_variables, bool has_nul_terminator);

int bufgen_generate_file_array_int(bufgen_data *data);
int bufgen_generate_file_array_char(bufgen_data *data);
int bufgen_generate_file_array_hex(bufgen_data *data);
int bufgen_generate_file_string(bufgen_data *data);

int bufgen_generate_element_int(bufgen_data *data, char current);
int bufgen_generate_element_char(bufgen_data *data, char current);
int bufgen_generate_element_hex(bufgen_data *data, char current);
/* int bufgen_generate_element_string(bufgen_data *data, char c); */

#ifdef BUFGEN_IMPLEMENTATION

/*
	UTILITY FUNCTIONS
*/

size_t bufgen_get_file_bytes(char const *filename)
{
	FILE *file;
	file = fopen(filename, "rb");
	if(file == NULL)
		return 0;
	fseek(file, 0, SEEK_END);
	size_t ans = ftell(file);
	fclose(file);
	return ans;
}

bool bufgen_is_printable_ascii(unsigned char byte)
{
	return byte >= 33 && byte <= 126;
}

/*
	BUFGEN FUNCTIONS
*/

int bufgen_generate_file(char const *input_file_name, char const *output_file_name, int *read_bytes, int *write_bytes, int mode, bool has_newlines, bool has_comments, bool has_variables, bool has_nul_terminator)
{
	int ans = 0;
	bufgen_data data;
	
	data.input_file_name = input_file_name;
	data.output_file_name = output_file_name;
	
	data.mode = mode;
	data.has_newlines = has_newlines;
	data.has_comments = has_comments;
	data.has_variables = has_variables;
	data.has_nul_terminator = has_nul_terminator;
	
	data.input_file = fopen(input_file_name, "rb");
	if(data.input_file == NULL)
	{
		ans = 1;
		goto end;
	}
	
	data.output_file = fopen(output_file_name, "wb");
	if(data.output_file == NULL)
	{
		ans = 1;
		goto end;
	}
	
	data.input_file_bytes = bufgen_get_file_bytes(input_file_name);
	data.output_file_bytes = 0;
	
	data.input_file_buffer = (char*)malloc(data.input_file_bytes);
	if(data.input_file_buffer == NULL)
	{
		ans = 1;
		goto end;
	}
	memset(data.input_file_buffer, 0, data.input_file_bytes);
	
	fread(data.input_file_buffer, 1, data.input_file_bytes, data.input_file);
	
	if(data.has_variables)
		data.output_file_bytes += fprintf(data.output_file, "static char const %s[] = %s\n", BUFGEN_VARIABLE_NAME_BUFFER, (data.mode == BUFGEN_MODE_STRING ? "" : "{"));
	
	switch(data.mode)
	{
		default:
		case BUFGEN_MODE_ARRAY_INT:
		{
			ans = bufgen_generate_file_array_int(&data);
		}
		break;
		case BUFGEN_MODE_ARRAY_CHAR:
		{
			ans = bufgen_generate_file_array_char(&data);
		}
		break;
		case BUFGEN_MODE_ARRAY_HEX:
		{
			ans = bufgen_generate_file_array_hex(&data);
		}
		break;
		case BUFGEN_MODE_STRING:
		{
			ans = bufgen_generate_file_string(&data);
		}
		break;
	}
	
	if(data.has_variables)
	{
		int cap = (data.has_nul_terminator || data.mode == BUFGEN_MODE_STRING) ? 1 : 0;
		cap += data.input_file_bytes;
		
		data.output_file_bytes += fprintf(data.output_file, "%s;\n", (mode == BUFGEN_MODE_STRING ? "" : "}"));
		data.output_file_bytes += fprintf(data.output_file, "static unsigned long long int const %s = %d\n", BUFGEN_VARIABLE_NAME_CAPACITY, cap);
		data.output_file_bytes += fprintf(data.output_file, "static unsigned long long int const %s = %d\n", BUFGEN_VARIABLE_NAME_LENGTH, data.input_file_bytes);
	}
	
	if(data.has_comments)
	{
		data.output_file_bytes += fprintf(data.output_file, "/*\n\t%s\n*/", BUFGEN_COMMENT);
		
		// fprintf(data.output_file, "\tInput File \"%s\"  : %d bytes.\n", data.input_file_name, data.input_file_bytes);
		// fprintf(data.output_file, "\tOutput File \"%s\" : %d bytes.\n*/\n", data.output_file_name, data.output_file_bytes);
	}
	
	/* add a newline at the end of the file */
	data.output_file_bytes += fprintf(data.output_file, "\n");
	
	end:
	
	if(data.input_file_buffer != NULL)
		free(data.input_file_buffer);
	if(data.input_file != NULL)
		fclose(data.input_file);
	if(data.output_file != NULL)
		fclose(data.output_file);
	if(read_bytes != NULL)
		*read_bytes = data.input_file_bytes;
	if(write_bytes != NULL)
		*write_bytes = data.output_file_bytes;
	
	return ans;
}


/* BUFGEN INT ARRAY */

int bufgen_generate_element_int(bufgen_data *data, char current)
{
	data->output_file_bytes += fprintf(data->output_file, "%d, ", (int)current);
	
	if(data->has_comments && bufgen_is_printable_ascii(current))
		data->output_file_bytes += fprintf(data->output_file, "/* %c */", current);
	
	if(data->has_newlines)
		data->output_file_bytes += fprintf(data->output_file, "\n");
	
	return 0;
}

int bufgen_generate_file_array_int(bufgen_data *data)
{
	for(int i = 0; i < data->input_file_bytes; ++i)
		bufgen_generate_element_int(data, data->input_file_buffer[i]);
	
	if(data->has_nul_terminator)
		bufgen_generate_element_int(data, 0);
	
	return 0;
}



/* BUFGEN CHAR ARRAY */

int bufgen_generate_element_char(bufgen_data *data, char current)
{
	if(bufgen_is_printable_ascii(current))
		data->output_file_bytes += fprintf(data->output_file, "'%c', ", current);
	else
		data->output_file_bytes += fprintf(data->output_file, "%d, ", (int)current);
	
	if(data->has_newlines)
		data->output_file_bytes += fprintf(data->output_file, "\n");
	
	return 0;
}

int bufgen_generate_file_array_char(bufgen_data *data)
{
	for(int i = 0; i < data->input_file_bytes; ++i)
		bufgen_generate_element_char(data, data->input_file_buffer[i]);
	
	if(data->has_nul_terminator)
		bufgen_generate_element_char(data, '\0');
	
	return 0;
}

/* BUFGEN HEX ARRAY */

int bufgen_generate_element_hex(bufgen_data *data, char current)
{
	data->output_file_bytes += fprintf(data->output_file, "0x%X, ", (int)current);
	
	if(data->has_comments && bufgen_is_printable_ascii(current))
		data->output_file_bytes += fprintf(data->output_file, "/* %c */", current);
	
	if(data->has_newlines)
		data->output_file_bytes += fprintf(data->output_file, "\n");
	
	return 0;
}

int bufgen_generate_file_array_hex(bufgen_data *data)
{
	for(int i = 0; i < data->input_file_bytes; ++i)
		bufgen_generate_element_hex(data, data->input_file_buffer[i]);
	
	if(data->has_nul_terminator)
		bufgen_generate_element_hex(data, 0);
	
	return 0;
}

/* BUFGEN STRING */

int bufgen_generate_file_string(bufgen_data *data)
{
	int i = 0;
	bool is_new_line = true;
	
	if(!data->has_newlines)
		data->output_file_bytes += fprintf(data->output_file, "\"");
	
	while(i < data->input_file_bytes)
	{
		char current = data->input_file_buffer[i];
		if(data->has_newlines && is_new_line)
		{
			data->output_file_bytes += fprintf(data->output_file, "\"");
			is_new_line = false;
		}
		switch(current)
		{
			case '\\':
			{
				data->output_file_bytes += fprintf(data->output_file, "\\\\");
				++i;
			}
			break;
			case '\"':
			{
				data->output_file_bytes += fprintf(data->output_file, "\\\"");
				++i;
			}
			break;
			case '\r':
			case '\n':
			{
				if(data->input_file_buffer[i] == '\r' && data->input_file_buffer[i + 1] == '\n')
				{
					data->output_file_bytes += fprintf(data->output_file, "\\r\\n");
					i += 2;
					is_new_line = true;
				}
				else
				if(data->input_file_buffer[i] == '\r')
				{
					data->output_file_bytes += fprintf(data->output_file, "\\r");
					i += 1;
					is_new_line = true;
				}
				else
				if(data->input_file_buffer[i] == '\n')
				{
					data->output_file_bytes += fprintf(data->output_file, "\\n");
					i += 1;
					is_new_line = true;
				}
				
				if(data->has_newlines && is_new_line)
				{
					data->output_file_bytes += fprintf(data->output_file, "\"\n"); /* close the line and add a newline */
				}
			}
			break;
			default:
			{
				data->output_file_bytes += fprintf(data->output_file, "%c", current);
				++i;
			}
			break;
			/* TODO: Maybe handle cases such as tab and \v and replace with "\\t", etc... basically handle whitespace if needed */
		}
	}
	
	if(!data->has_newlines)
		data->output_file_bytes += fprintf(data->output_file, "\"");
	
	return 0;
}

/* REMEMBER TO HANDLE FPRINTF FAILURE AND INCREASING THE NUMBER OF WRITTEN BYTES IN THE OUTPUT BYTES VARIABLE... */


#endif /* BUFGEN_IMPLEMENTATION */

#endif /* BUFGEN_H */
