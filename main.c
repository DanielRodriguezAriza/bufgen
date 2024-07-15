#include <stdio.h>
#include <stdbool.h>

#define BUFGEN_IMPLEMENTATION
#include "bufgen.h"

int main(int argc, char **argv)
{
	/* example usage : bufgen in1.txt out1.h in2.txt out2.h */
	if(argc < (2 + 1) || (argc - 1) % 2 != 0)
	{
		fprintf(stderr, "Usage : %s <in_file_1> <out_file_1> <in_file_2> <out_file_2> etc...", argv[0]);
		return 1;
	}
	
	for(int i = 1; i < argc; i+=2)
	{
		char const *in_filename = argv[i + 0];
		char const *out_filename = argv[i + 1];
		int read_bytes = 0;
		int write_bytes = 0;
		int success = bufgen_generate_file(in_filename, out_filename, &read_bytes, &write_bytes, BUFGEN_MODE_STRING, true, true, true, true);
		if(success == 0)
		{
			fprintf(stdout, "[%d]:\n -from  : \"%s\"\n -to    : \"%s\"\n -read  : %d bytes.\n -write : %d bytes.\n", i, in_filename, out_filename, read_bytes, write_bytes);
		}	
		else
		{
			fprintf(stderr, "[%d]:\n -There was an error writing the file \"%s\".\n", i, out_filename);
			return 1;
		}
	}
	
	return 0;
}
