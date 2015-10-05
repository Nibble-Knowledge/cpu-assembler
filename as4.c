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
		size_t outputsize = 0;
		const char *delims = " \t";
		char *outbuf = NULL;
		struct stat st;
		size_t bufsize = 0;
		unsigned int i = 0;
		unsigned long long bits = 0;
		unsigned long long bytes = 0;	

		input = fopen(argv[1], "rb");
		if(input == NULL)
		{
			perror("Could not open input file");
			return 1;
		}
		
		output = fopen(argv[2], "wb");
		if(output == NULL)
		{
			perror("Could not open output file");
			return 2;
		}
		
		stat(argv[1], &st);
		bufsize = (size_t)st.st_size * 2;
		outbuf = calloc(bufsize, 1);
		if(outbuf == NULL)
		{
			perror("Could not allocate output buffer");
		}
		linelen = getline(&line, &len, input);
		while(linelen > -1)
		{
			char *tokens;

			if(strlen(line) >= 2)
			{
				tokens = strtok(line, delims);
				if(tokens != NULL && tokens[0] != ';' && tokens[0] != '\n' && tokens[0] != '\r')
				{
			
					while(tokens != NULL)
					{
						if(tokens[0] == ';')
						{
							break;
						}
						else if(tokens[0] != '\n' && tokens[0] != '\r')
						{
							size_t tokenlen = strlen(tokens);
							int doneline = 0;
							if(tokens[tokenlen - 1] == '\n')
							{
								tokens[tokenlen - 1] = '\0';
							}
							
							if(!strncmp(tokens, "HLT", 3))
							{
								printf("0000 0000 0000 0000 0000");
								doneline = 1;
							}
							else if(!strncmp(tokens, "LOD", 3))
							{
								printf("0001|");
							}
							else if(!strncmp(tokens, "STR", 3))
							{
								printf("0010|");
							}
							else if(!strncmp(tokens, "ADD", 3))
							{
								printf("0011|");
							}
							else if(!strncmp(tokens, "NOP", 3))
							{
								printf("0100 0000 0000 0000 0000");
								doneline = 1;
							}
							else if(!strncmp(tokens, "NND", 3))
							{
								printf("0101|");
							}
							else if(!strncmp(tokens, "JMP", 3))
							{
								printf("0110|");
							}
							else if(!strncmp(tokens, "CXA", 3))
							{
								printf("0111 0000 0000 0000 0000");
								doneline = 1;
								if(bits % 8 != 0)
								{
									outbuf[bytes] |= 0x70;
									bytes++;
									bits += 4;
									outbuf[bytes] = 0;
									bytes++;
									bits += 8;
									outbuf[bytes] = 0;
									bytes++;
									bits += 8;
								}
								else
								{
									outbuf[bytes] = 7;
									bytes++;
									bits += 8;
									outbuf[bytes] = 0;
									bytes++;
									bits += 8;
									outbuf[bytes] &= 0xF0;
									bits += 4;
								}
							}
							else
							{
								printf("%s|", tokens);
							}

							if(doneline)
							{
								break;
							}
						}
						tokens = strtok(NULL, delims);
					}
					puts("");
				}	
			}
			linelen = getline(&line, &len, input);
		}
		for(i = 0; i < bufsize; i++)
		{
			char temp = outbuf[i];
			char j;
			for(j = 0; j < 8; j++)
			{
				if(temp & 1)
				{
					printf("1");
				}
				else
				{
					printf("0");
				}
				if((j+1)%4 == 0)
				{
					printf(" ");
				}
				temp >>= 1;
			}
		}
		printf("\n");
		free(outbuf);
	}
	return 0;	
}

void help(void)
{
	puts("This is the low level assembler for the Nibble Knowledge 4-bit computer");
	puts("");
	puts("Usage: as4 input output");

}
