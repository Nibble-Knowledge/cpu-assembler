#include "as4.h"

int main(int argc, char **argv)
{
	if(argc < 3)
	{
		help();
		return 0;
	}
	else
	{
		FILE *input;
		FILE *output;
		char *line = NULL;
		ssize_t linelen = -1;
		size_t len = 0;

		input = fopen(argv[1], "r");
		if(input == NULL)
		{
			perror("Could not open input file");
			return 1;
		}
		
		output = fopen(argv[2], "w");
		if(output == NULL)
		{
			perror("Could not open output file");
			return 2;
		}
		
		linelen = getline(&line, &len, input);
		while(linelen > -1)
		{
			puts(line);
			linelen = getline(&line, &len, input);
		}
		
	}
	
}

void help(void)
{
	puts("This is the low level assembler for the Nibble Knowledge 4-bit computer");
	puts("");
	puts("Usage: as4 input output");

}
