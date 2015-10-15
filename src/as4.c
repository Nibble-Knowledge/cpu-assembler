#include "as4.h"

/* This is the entirety of the assembler's code. It's under 640 executable lines, so a single file is acceptable. */

unsigned long long FILELINE = 0;

/* Let's get ready to party */
int main(int argc, char **argv)
{
	/* The minimum number of arguments is 3 (program name, input file, output file) */
	/* There's nothing to do if we don't have those so just quit. */
	
	if(argc < 3)
	{
		/* Also tell people how to use the program, usually helps. */
		
		help();
		return 0;
	}
	else
	{
		/* ANSI C mandates that variables be declared at the top of the current block. */
		/* It's a habit I got into and I think it makes nicely structured code. */
		
		/* Input file */
		FILE *input;
		/* Output file */
		FILE *output;
		/* Pointer to the current line being read */
		char *line = NULL;
		/* Length of the current line. If it is negative, that means we are at the end of the file. */
		/* Setting it to negative just checks that incase the function readline blows up we have a value we know is invalid. */
		ssize_t linelen = -1;
		/* Length of the current file. Used to allocate the output buffer (which is currently waaaaayyyy too big). */
		size_t len = 0;
		/* Delimiters for strtok(). Split the current line into tokens based on spaces and tabs (standard whitespace). */
		const char *delims = " \t";
		/* The pointer to the output buffer. */
		char *outbuf = NULL;
		/* The stat structure. We get the file size from this. */
		struct stat st;
		/* The size of the output buffer. 2 times the size of the original file. */
		size_t bufsize = 0;
		/* Generic loop counter */
		unsigned int i = 0;
		/* The current amount of bits in the output. Bits modulo 8 allows us to tell if we have to bytesplit or not */
		unsigned long long bits = 0;
		/* The current amount of whole bytes in the output. Used to address the output buffer as it is a byte array. */
		unsigned long long bytes = 0;
		/* The base address is the reference address that all memory addresses are calculated against. Make sure this is right! */
		unsigned short int baseaddr = 0;
		/* Add 2 to this if we are using a base address, prevents if then bollocks. */
		unsigned short int arg = 1;
		/* The total number of declared labels */
		unsigned long long numlabels = 0;
		/* The number of references to potentially valid but undeclared labels. */
		unsigned long long numunknownlabels = 0;
		/* Pointer to the collection of declared labels */
		label *labels = NULL;
		/* Pointer to the collection of referenced and potentially valid but undeclared labels */
		label *unknownlabels = NULL;
		
		/* Check if a base address has been specified */
		if(!strncmp(argv[1], "-b", 3))
		{
			/* We use this endptr to check if the input is valid */
			/* strtoul sets this endptr to the address of the first invalid character - if that's equal to argv[2], the entire string was invalid. */
			char *endptr = NULL;
			
			/* If yes, out input and output arguments are two elements farther in the array so increment arg */
			arg += 2;
			/* Get the base address from this conversion */
			baseaddr = strtoul(argv[2], &endptr, 0);
			/* Make sure to check if the base address is valid */
			/* It wouldn't break the program - baseaddr would just be zero - but it's probably not what the use wanted. And at this level of programming, these details matter a lot. */
			if(baseaddr == 0 && (argv[2] == endptr))
			{
				/* Give them a friendly message and get da heck outta dodge */
				fprintf(stderr, "%s is an invalid base address.\n", argv[2]);
				exit(21);
			}
		}
		/* Open our necessary files and check they opened correctly */
		/* NULL file points are NOT your friend! */
		
		/* "rb" is "Read, binary mode" */
		input = fopen(argv[arg], "rb");
		if(input == NULL)
		{
			perror("Could not open input file");
			return 1;
		}
		/* Likewise, "wb" is "Write, binary mode" */
		output = fopen(argv[arg+1], "wb");
		if(output == NULL)
		{
			perror("Could not open output file");
			return 2;
		}
		
		/* Check the size of the input file and mutiply it by two to get the size of the output buffer */
		/* Why? It's basically totally crap but each assembly instruction WITHOUT the memory address or label is about 4 bits bigger than an instruction */
		/* This ENSURES the buffer is big enough */
		/* Bar like an enormous .data statement */
		stat(argv[arg], &st);
		bufsize = (size_t)st.st_size * 2;
		/* Allocate it will all zeros to be safe (calloc() is malloc() but it zeros the memory) */
		outbuf = calloc(bufsize, 1);
		if(outbuf == NULL)
		{
			perror("Could not allocate output buffer");
		}
		/* Get the first line of the assembly file */
		linelen = getline(&line, &len, input);
		/* First line */
		FILELINE = 1;
		/* Loop until the end of the file */
		while(linelen > -1)
		{
			/* These are the whitespace delimited tokens in each line that we process into machine code */
			/* like LOD and .data */
			char *tokens;
			
			/* There is no useful line that is less than 2 characters long. */
			if(strlen(line) >= 2)
			{
				/* Get our tokens */
				tokens = strtok(line, delims);
				/* Check the token is valid, and not a comment (; is a comment). \n or \r means we're just reading the line ending (and that's likely the only character in the token) so skip the sucker */
				if(tokens != NULL && tokens[0] != ';' && tokens[0] != '\n' && tokens[0] != '\r')
				{
					/* Loop until we've gotten every token on this line. */
					while(tokens != NULL)
					{
						/* If we get to a ; on the line, ignore the rest. Allows inline comments */
						if(tokens[0] == ';')
						{
							break;
						}
						/* Again make sure the token is work dealing with */
						else if(tokens[0] != '\n' && tokens[0] != '\r')
						{
							/* Length of the token (useful for things) */
							size_t tokenlen = strlen(tokens);
							/* Certain assembly instructions don't require us to check the rest of the line, like NOP, so when we encounter these just add the instruction to the output buffer and go to the next line. */
							int doneline = 0;
							/* The address component of the instruction. Used in all instructions (though sometimes set to 0 and ignored). */
							unsigned short int address = 0;
							/* Remove the trailing newline. I think this was used more for displaying tokens during development, but it doesn't hurt. */
							if(tokens[tokenlen - 1] == '\n')
							{
								tokens[tokenlen - 1] = '\0';
							}
							/* Halt instruction */
							/* Easy one */
							if(!strncmp(tokens, "HLT", 4))
							{
								/* We only need to understand that it's the halt instruction, we don't care about the rest of the line. */
								doneline = 1;
								/* Add the instruction to the output buffer. */
								addinst(outbuf, HLT, NOADDR, &bits, &bytes);
							}
							/* Load instruction */
							else if(!strncmp(tokens, "LOD", 4))
							{
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									/* If not, print an error and quit. */
									fprintf(stderr, "Line %llu: A memory address must succeed a LOD instruction.\n", FILELINE);
									exit(8);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, tokens, numlabels, &numunknownlabels, bits);
								/* Add this to the output buffer. */
								addinst(outbuf, LOD, address, &bits, &bytes);
							}
							/* Store instruction. */
							else if(!strncmp(tokens, "STR", 4))
							{
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a STR instruction.\n", FILELINE);
									exit(12);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, tokens, numlabels, &numunknownlabels, bits);
								/* Add this to the output buffer. */
								addinst(outbuf, STR, address, &bits, &bytes);
							}
							/* Add instruction. */
							else if(!strncmp(tokens, "ADD", 4))
							{
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a ADD instruction.\n", FILELINE);
									exit(13);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, tokens, numlabels, &numunknownlabels, bits);
								/* Add this to the output buffer. */
								addinst(outbuf, ADD, address, &bits, &bytes);
							}
							/* No-operation instruction */
							else if(!strncmp(tokens, "NOP", 4))
							{
								/* We only need to understand that it's the nop instruction, we don't care about the rest of the line. */
								doneline = 1;
								/* Add the instruction to the output buffer. */
								addinst(outbuf, NOP, NOADDR, &bits, &bytes);
							}
							/* Nand instruction. */
							else if(!strncmp(tokens, "NND", 4))
							{
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a NND instruction.\n", FILELINE);
									exit(14);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, tokens, numlabels, &numunknownlabels, bits);
								/* Add this to the output buffer. */
								addinst(outbuf, NND, address, &bits, &bytes);
							}
							/* Jump instruction */
							else if(!strncmp(tokens, "JMP", 4))
							{
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a JMP instruction.\n", FILELINE);
									exit(15);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, tokens, numlabels, &numunknownlabels, bits);
								/* Add this to the output buffer. */
								addinst(outbuf, JMP, address, &bits, &bytes);
							}
							/* Carry and XOR bits to Accumulator instruction */
							/* A very niche but important instruction. */
							else if(!strncmp(tokens, "CXA", 4))
							{
								/* We only need to understand that it's the halt instruction, we don't care about the rest of the line. */
								doneline = 1;
								/* Add the instruction to the output buffer. */
								addinst(outbuf, CXA, NOADDR, &bits, &bytes);
							}
							/* .data arbitrary data section inputs */
							else if(!strncmp(tokens, ".data", 6))
							{
								/* Size of the data section */
								unsigned long long datasize = 0;
								/* Initial value */
								long long datavalue = 0;
								/* endptr for determining if the number used for initilisation is valid */
								char *endptr = NULL;
								
								/* Once we're done dealing with the .data element, we will have taken every useful token from this line, so declared the line done. */
								doneline = 1;
								/* Get the next token (which should be the size of the .data element). */
								tokens = strtok(NULL, delims);
								/* Check that the token is valid. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									/* If not, warn the user. And quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element.\n", FILELINE);
									exit(7);
								}
								/* Errno can be set non-zero for many reasons. Set it to zero now so we can check the result of strtoul. */
								errno = 0;
								/* Try to get the size of the data element */
								datasize = strtoul(tokens, &endptr, 0);
								/* Check the data size is valid by checking both errno and if the string was simply invalid (tokens == endptr, endptr points to the first invalid character) */
								if(errno != 0 || tokens == endptr)
								{
									/* If either is true, warn the user and quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element or an invalid value was used.\n", FILELINE);
									exit(7);
								}
								/* Get the next token (which should be the initial value of the .data element). */
								tokens = strtok(NULL, delims);
								/* Check that the token is valid. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									/* If not, warn the user. And quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element.\n", FILELINE);
									exit(7);
								}
								/* Errno can be set non-zero for many reasons. Set it to zero now so we can check the result of strtoul. */
								errno = 0;
								/* Try to get the initial value of the data element */
								datavalue = strtol(tokens, &endptr, 0);
								/* Check the initial value is valid by checking both errno and if the string was simply invalid (tokens == endptr, endptr points to the first invalid character) */
								if(errno != 0 || tokens == endptr)
								{
									/* If either is true, warn the user and quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element or an invalid value was used.\n", FILELINE);
									exit(7);
								}
								/* Get the next token as this will probably be a newline, and this speeds up the reading process for the next line. */
								tokens = strtok(NULL, delims);
								/* After we have gotten the size and initial value, add this to the output buffer. */
								adddata(&outbuf, bufsize, datasize, datavalue, &bits, &bytes);
							}
							/* .ascii and .asciiz string data sections. */
							/* .asciiz is the zero terminated version. */
							else if(!strncmp(tokens, ".ascii", 7) || !strncmp(tokens, ".asciiz", 8))
							{
								/* Zero termination flag */
								char zeroterm = 0;
								/* The total string string - we can't use tokens here alone, they remove spaces we actually need. */
								char *string = calloc(1, (size_t)linelen);
								
								/* If it's .asciiz, set the zero termination flag. */
								if(!strncmp(tokens, ".asciiz", 8))
								{
									zeroterm = 1;
								}
								/* Get the next token, which should be the string. */
								tokens = strtok(NULL, delims);
								/* Check if it is simply an empty string */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0')
								{
									/* If it is quit and print an error. */
									fprintf(stderr, "Line %llu: A string is required after .ascii or .asciiz.\n", FILELINE);
									exit(15);
								}
								/* If it's a space, it might just be a weird whitespace glitch. */
								else if(tokens[0] == ' ')
								{
									/* So try to get the token again. */
									tokens = strtok(NULL, delims);
									/* If it's still invalid, there is probably no string there. */
									if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
									{
										/* Complain and quit. */
										fprintf(stderr, "Line %llu: A string is required after .ascii or .asciiz.\n", FILELINE);
										exit(15);
									}
								}
								/* Loop until we reach the end of the line. */
								while(!(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0'))
								{
									/* Concatenate all the tokens on this line together with their missing whitespace to make the actual stored string. */
									strcat(string, tokens);
									strcat(string, " ");
									tokens = strtok(NULL, delims);
								}
								/* And null terminate it (and cut off the trailing newline) */
								string[strlen(string) - 2] = '\0';
								/* Add it to the output buffer. */
								addstring(outbuf, string, zeroterm, &bits, &bytes);
							}
							/* If the currently token ends in an colon, it's probably a label. */
							else if(tokens[strlen(tokens)-1] == ':')
							{
								/* Add the label to the list of known labels and replace any references to it with it's actual address. */
								addlabel(outbuf, &labels, &unknownlabels, &numlabels, numunknownlabels, tokens, bits, baseaddr);
							}
							/* If we're done the line, don't both getting another token. */
							if(doneline)
							{
								break;
							}
						}
						/* But if we're not done the line, get another token. */
						tokens = strtok(NULL, delims);
					}
				}	
			}
			/* Next line - increment FILELINE. */
			FILELINE++;
			/* And get another line. */
			linelen = getline(&line, &len, input);
		}
		/* If the modulo of bits and 8 is not zero, we have a fractional amount of bytes. */
		/* Since x86 machines and their operating systems are based on using the byte as the atomic unit, we cannot write fractional bytes to a file. */
		/* We also don't want to mess up the file and add something that would cause unexpected behaviour (like a nibble of zero padding). */
		if((bits % 8) != 0)
		{
			/* So we add a single NOP. */
			addinst(outbuf, NOP, NOADDR, &bits, &bytes);
		}
		/* If we get here, we've read the entire input file and we don't need it anymore. */
		fclose(input);
		/* If we ever had referenced but unknown labels, we should check them and make sure we found them all. */
		/* We should also free the data structure because that's a nice thing to do. */
		if(unknownlabels != NULL)
		{
			/* For each unknown label... */
			for(i = 0; i < numunknownlabels; i++)
			{
				/* Have a flag for whether it was found in the known labels collection. */
				char foundunknown = 0;
				/* Generic looping variable. */
				unsigned int j = 0;
				
				/* Loop through all the known labels and look for it. */
				for(j = 0; j < numlabels; j++)
				{
					if(labels[j].str != NULL)
					{
						/* If we find it, set found to true. */
						if(!strcmp(labels[j].str, unknownlabels[i].str))
						{
							foundunknown = 1;
						}
					}
				}
				/* If we don't find it, someone has used a label but never declared it. That's generally a bad idea. */
				if(!foundunknown)
				{
					/* So complain and close. */
					fprintf(stderr, "Used but undeclared label %s.\n", unknownlabels[i].str);
					exit(20);
				}
				/* Once we have made sure this label has been declared correctly, free the allocated memory for the label string. */
				/* We don't need it anymore. */
				if(unknownlabels[i].str != NULL)
				{
					free(unknownlabels[i].str);
				}
			}
			/* Once we've checked all the unknown labels, free the entire structure array. We don't need it anymore. */
			free(unknownlabels);
		}
		
		/* Once we're sure everything is in order, write the buffer to a file. */
		for(i = 0; i < bytes; i++)
		{
			fputc(outbuf[i], output);
		}
		
		/* Once we've written everything to a file, we don't need the known labels anymore, so get rid of them. */
		/* Iterate through and free all the label things and the array itself. */
		if(labels != NULL)
		{
			for(i = 0; i < numlabels; i++)
			{
				if(labels[i].str != NULL)
				{
					free(labels[i].str);
				}
			}
			free(labels);
		}
		/* We now have no use for the output buffer, as we wrote it to a file. Set it free. */
		free(outbuf);
		/* It's also a nice thing to close your file handles. We're done writing the buffer to the output, so close it. */
		fclose(output);
	}
	return 0;	
	/* This is the end of successful program execution. */
}

/* Help() prints the help. More useful in large programs */
void help(void)
{
	puts("This is the low level assembler for the Nibble Knowledge 4-bit computer");
	puts("");
	puts("Usage: as4 [-b base_address] input output");

}
/*addinst() appends an instruction to the output buffer and handles bytesplitting */
void addinst(char *outbuf, uint8_t op, uint16_t addr, unsigned long long *bits, unsigned long long *bytes)
{
	/* If the output buffer is NULL, we have nothing to do. */
	if(outbuf == NULL)
	{
		return;
	}
	/* If the modulus of bits and 8 is not zero, we known we have to bytesplit. Because we're adding things in ascending order, it must be the lower 4 bits of a byte. */
	if((*bits) % 8 != 0)
	{
		/* We're cheating a little here as we know the buffer that isn't written to will be zero'd because of our calloc. */
		/* But mask to the lower 4 bits of op (which is the 4-bit opcode stored in a byte) */
		/* And OR that to the current byte. As the lower 4 should be zeros, this will add the nibble correctly to the low 4 bits. */
		outbuf[*bytes] |= ((op) & 0x0F);
		/* Since we've now filled the lower 4 bits, we can go up by a byte. */
		(*bytes)++;
		/* And up by 4 bits. */
		*bits += 4;
		/* Here we know we are writing an entire byte (the address is 2 bytes) so shift the top byte down and mask everything else away. */
		/* We can just directly assign this byte. */
		outbuf[*bytes] = (char)((addr >> 8) & 0x00FF);
		/* And we wrote an entire byte, so go up by a byte. */
		(*bytes)++;
		/* And 8 bits */
		*bits += 8;
		/* Here we know we are writing an entire byte (the address is 2 bytes) so just use the lowest byte of the address and mask everything else away. */
		/* We can just directly assign this byte. */
		outbuf[*bytes] = (char)((addr) & 0x00FF);
		/* And we wrote an entire byte, so go up by a byte. */
		(*bytes)++;
		/* And 8 bits */
		*bits += 8;
	}
	else
	{
		/* Since the modulo of bits and 8 is zero, we know we're at the start of a byte */
		/* Assign the opcode to the highest 4 bits, and the highest 4 bits of the address to the lower 4 bits of the byte. */
		outbuf[*bytes] = ((op << 4) & 0xF0) | ((addr >> 12) & 0x000F);
		/* And we wrote an entire byte, so go up by a byte. */
		(*bytes)++;
		/* And 8 bits */
		*bits += 8;
		/* Here we know we are writing an entire byte (the address is 2 bytes) so shift the middle byte down by 4 bits and mask everything else away. */
		outbuf[*bytes] = (char)((addr >> 4) & 0x00FF);
		/* And we wrote an entire byte, so go up by a byte. */
		(*bytes)++;
		/* And 8 bits */
		*bits += 8;
		/* We only have 4 bits left in the instruction, so we have to simply write it to the topmost 4 bits of a byte. */
		/* So take the lowest nibble of the address and shift it up 4 bits, mask the rest to zeros, then OR it with the empty (all zero) byte. */
		outbuf[*bytes] |= (char)((addr & 0x000F) << 4);
		/* We have only written 4 bits, so do not go up a byte. */
		/* Go up by 4 bits though. */
		*bits += 4;
	}
}

/* adddata() handles adding .data statements to the output buffer. It also handles resizing the buffer if need be. */
void adddata(char **outbuf, size_t bufsize, unsigned long long size, long long value, unsigned long long *bits, unsigned long long *bytes)
{
	/* Because we write 4 bits at a time in a loop, which nibble we are on is useful. */
	unsigned long long nibbles = 0;
	
	/* If the output buffer is NULL, we have no work to do. So just return. */
	if(outbuf == NULL)
	{
		return;
	}
	/* If the current amount of bits written (the current size of the program) plus the size of this data element is larger than the buffer, make the buffer bigger. */
	/* This code is a little hacky and and I haven't tested it. */
	if(((((*bits)+20)/8) + (size/2)) > bufsize)
	{
		/* But basically resize the buffer to the size of the bits plus the size of this data element plus one instruction (just in case) and mutiply it all by 1.5 so that it's likely that it's the only time we resize the buffer. */
		/* This formula is entirely guesswork, but it should do the job. */
		(*outbuf) = realloc(*outbuf, (size_t)(((((*bits) + 20)/8) + (size/2)) * 1.5));
		/* Many people will tell you to keep the old pointer to the memory location you are resizing because the contents are still there and usable even though realloc sets the pointer that it returns to NULL. That is true and useful. */
		/* In this case we're kinda screwed if we can't make the buffer bigger so just quit and complain. */
		if((*outbuf) == NULL)
		{
			perror("Could not reallocate memory");
			exit(24);
		}
		
	}
	/* Because of the way we write each nibble at at a time, we basically have to shift the most significant nibble first down to the byte level and then keep moving down the initial value as we go. */
	for(nibbles = size; nibbles > 0; nibbles--)
	{
		/* If the modulo of bits and 8 isn't zero, we're writing to the lower nibble of a byte */
		if((*bits) % 8 != 0)
		{
			/* So shift the nibble down to the lower part of the byte and OR it in. */
			(*outbuf)[*bytes] |= ((value >> (4*(nibbles-1))) & 0xF);
			/* And because we have filled the lower nibble, we've completed a byte. */
			(*bytes)++;
			/* And written 4 bits. */
			*bits += 4;
		}
		/* If the modulo of bits and 8 is zero, we're writing to the higher nibble of a byte */
		else
		{
			/* So shift it to where it was before, mask it, then move it back up 4 bits. */
			/* There is probably a more efficient way to do this. */
			(*outbuf)[*bytes] |= ((((value >> (4*(nibbles-1))) & 0xF) << 4) & 0xF0);
			/* Since we have only written to the highest nibble of the byte, we have written 4 bits, but we have not completed a byte. */
			*bits += 4;
		}
	}
}

/* addlabel() adds a label to the collection of labels, to be used by other functions when a label reference is made. */
/* Additionally, if there are outstanding "queries" for a certain label (if the label has been used before it has been declared) it replaces the "unknown address" address in an instruction with the address of the label on declaration */
void addlabel(char *outbuf, label **labels, label **unknownlabels, unsigned long long *numlabels, unsigned long long numunknownlabels, const char *labelstr, unsigned long long bits, unsigned short int baseaddr)
{
	/* This is the pointer to the label string, which is used for comparison and assigned to it's element in the collection when we are finished. */
	char *tempstr = NULL;
	/* Generic looping unit. */
	unsigned int i = 0;
	
	/* Because labels is actually placed in here as a pointer to a pointer, if it's NULL there's not pointer to work with, so we cannot do any work. */
	if(labels == NULL)
	{
		return;
	}
	/* If the pointer to the collection is NULL, it means we haven't made it yet. */
	else if(*labels == NULL)
	{
		/* Make the collection 1 structure big at the start. */
		*labels = malloc(sizeof(label));
		/* If we can't allocate memory, we're going to have a bad time. */
		if(*labels == NULL)
		{
			/* Quit and complain. */
			perror("Could not allocate memory");
			exit(3);
		}
		/* Start with the string being NULL. */
		(*labels)[0].str = NULL;
		/* We have one label. */
		*numlabels = 1;
	}
	/* If the pointer to the collection is non-NULL, it exists (or it should exist). */
	else
	{
		/* We are being called because there is a new label. Tell ourselves we have another one. */
		(*numlabels)++;
		/* Resize the collection to hold the new label. */
		*labels = realloc(*labels, sizeof(label) * (*numlabels));
		/* If we can't, that sucks. */
		if(*labels == NULL)
		{
			/* Quit and complain. */
			perror("Could not reallocate memory");
			exit(4);
		}
	}
	/* If we get here, we have a place to put the new label. */
	/* Allocate some zeroed memory so we can use it for a comparison and assign it to the structure element and use it in other parts of the program. */
	tempstr = calloc(1, strlen(labelstr));
	/* If we can't allocate the memory, that sucks. */
	if(tempstr == NULL)
	{
		/* Quit and complain. */
		perror("Could not allocate memory");
		exit(5);
	}
	/* Copy the contains of the label's name, up until the zero terminator (our calloc gave us an all zero string array, so any data copied in is automatically zero terminated). */
	memcpy(tempstr, labelstr, (strlen(labelstr) - 1));
	/* Before we commit to keeping this label, check if we've used the name already. */
	/* Because using the same label for two different locations makes no sense. */
	for(i = 0; i < ((*numlabels) - 1); i++)
	{
		/* If we find a duplicate, tell the programmer. */
		if(!strcmp((*labels)[i].str, tempstr))
		{
			/* By quitting and complaining. */
			fprintf(stderr, "Line %llu: Label %s already used.\n", FILELINE, tempstr);
			exit(6);
		}
	}
	
	/* Assign the label string to the structure. We're sure we want to keep it. */
	(*labels)[(*numlabels) - 1].str = tempstr;
	/* Assign the current location of the output buffer to as the address, because then the label will point to the next instruction or data element added, which is what we want. */
	/* Also add the base address, which is important. */
	(*labels)[(*numlabels) - 1].addr = ((bits/4) + baseaddr);
	/* Now we've saved the declared label, we can check for it being used before it was declared. */
	/* But if there is no output buffer, we can't do much. */
	if(outbuf == NULL)
	{
		return;
	}
	/* Make sure the pointer to the pointer to the unknown label collection is valid. */
	else if(unknownlabels != NULL)
	{
		/* Make sure the pointer to the collection is valid too. */
		if((*unknownlabels) != NULL)
		{
			/* Check every unknown label. */
			for(i = 0; i < numunknownlabels; i++)
			{
				/* If the unknown label name exits (otherwise it would be hard to indentify it)... */
				if((*unknownlabels)[i].str != NULL)
				{
					/* Check if the name of the unknown label is the same as the label that we just had declared. */
					if(!strcmp((*unknownlabels)[i].str, tempstr))
					{
						/* If it is, then take stock of both the address it was referenced */
						unsigned short int instaddress = (*unknownlabels)[i].addr;
						/* And the address the label points to. */
						unsigned short int labeladdr = (*labels)[(*numlabels) - 1].addr;
						
						/* The address it was referenced at is actually the opcode of the instruction, so go up one nibble to point to the address section. */
						instaddress++;
						/* Since the instaddress is actually what nibble is being addressed, we correctly address a byte every 2 nibbles. */
						/* If it's not equal to zero, we're starting with the lower nibble of a byte. */
						if((instaddress % 2) != 0)
						{
							/* As unknown addresses are set to 0xFFFF (which is much safer than 0x0000 as that is a I/O device selection nibble) we cannot just OR things. */
							/* So we mask away the lower nibble (the opcode must be the higher nibble of this byte) and OR the highest nibble of the address to it. */
							outbuf[instaddress / 2] = (char) ((outbuf[instaddress / 2] & 0xF0) | ((labeladdr >> 12) & 0x000F));
							/* Move up a nibble (and implicitly a byte). */
							instaddress++;
							/* We can now directly assign the middle two nibbles to this byte. */
							outbuf[instaddress / 2] = (char) ((labeladdr >> 4) & 0x00FF);
							/* Move up 2 nibbles. */
							instaddress += 2;
							/* However, now we need to assign this nibble to the high nibble of the next byte. */
							/* There is also likely data on the lower nibble, so we have to protect it. */
							/* So we mask away the upper nibble and OR the lowest nibble of data to it (shifted up 4 bits). */
							outbuf[instaddress / 2] = (char) ((outbuf[instaddress / 2] & 0x0F) | (((labeladdr) & 0x000F) << 4));
						}
						else
						/* If it's equal to zero, we're starting with the higher nibble of a byte. */
						{
							/* We can simply assign the top two nibbles and lower two nibbles to 2 bytes. */
							outbuf[instaddress / 2] = (char)(0xFF & (labeladdr >> 8));
							instaddress += 2;
							outbuf[instaddress / 2] = (char)(0xFF & (labeladdr));
						}
					}
				}
			}
		}
	}
}

/* findlabel() determines the memory address that follows the opcode. If the memory address is already a number, and returns it */
/* If it is an undeclared label, it adds the label name and the instruction location to the "unknown labels" collection. If the label is declared, it returns it's address */
unsigned short int findlabel(label **unknownlabels, label **labels, const char *labelstr, unsigned long long numlabels, unsigned long long *numunknownlabels, unsigned long long bits)
{
	/* endptr is used to check if the strtol finds a valid number. */
	/* If not, it's likely a label. */
	char *endptr = NULL;
	/* tempstr stores the name of the label, and becomes copied into the unknown label collection. */
	char *tempstr = NULL;
	/* Generic looping unit. */
	unsigned int i = 0;
	/* If we can't find the address of the label, return UNKNOWNADDR (0xFFFF) and wait for it to be changed later. */
	unsigned short int address = UNKNOWNADDR;
	/* tempaddress is what is assigned to the return value of strtol. We use this value if we find that the token after the assembly instruction is actually a number. */
	unsigned short int tempaddress = address;
	
	/* Set errno to 0 so we can check for errors from strtol, which will tell us if the token after the assembly instruction is a number or a label (if it's neither we find out a bit later, not in this function). */
	errno = 0;
	/* tempaddress is 0 if the labelstr is actually a label and not a value. */
	tempaddress = strtol(labelstr, &endptr, 0);
	/* If errno == 0 or labelstr == endptr (saying that the first invalid value was at the start of the string) we're not dealing with a hard coded value. Likely a label. */
	if(errno != 0 || labelstr == endptr)
	{
		/* Allocate some zero-initialised memory for comparison and assignment later. */
		tempstr = calloc(1, strlen(labelstr));
		/* If we cannot allocate memory, that sucks. */
		if(tempstr == NULL)
		{
			/* Quit and complain. */
			perror("Could not allocate memory");
			exit(9);
		}
		/* Copy the name of the label into the temporary string pointer for comparison. */
		memcpy(tempstr, labelstr, strlen(labelstr));
		/* Check that the pointer to the pointer to the collection of labels is valid .*/
		if(labels != NULL) 
		{	
			/* Check the pointer to the collection is valid. */
			if(*labels != NULL)
			{
				/* Interate through the entire collection and check if the label has been declared. */
				for(i = 0; i < numlabels; i++)
				{
					/* Always check that the dynamically allocated string has been allocated. */
					if((*labels)[i].str != NULL)
					{						
						/* Check to see if any names match the one we are looking for. */
						if(!strcmp((*labels)[i].str, tempstr))
						{
							/* If they are, use the address the label points to. */
							address = (*labels)[i].addr;
						}
					}
				}
			}
		}
		/* If address still equals UNKNOWNADDR, we didn't find the label in the label collection. */
		if(address == UNKNOWNADDR)
		{
			/* Check that the pointer to the pointer to the collection is valid. */
			if(unknownlabels != NULL)
			{
				/* If the pointer to the collection of unknown labels is NULL, we haven't made the collection yet. */
				if((*unknownlabels) == NULL)
				{
					/* Create the collection for a single label. */
					*unknownlabels = malloc(sizeof(label));
					/* If we cannot allocate memory, that sucks. */
					if(*unknownlabels == NULL)
					{
						/* Quit and complain. */
						perror("Could not allocate memory");
						exit(10);
					}
					/* Assign the name so we know what to look for later. */
					(*unknownlabels)[0].str = tempstr;
					/* and the current location, so the unknown address number can be replaced later. */
					(*unknownlabels)[0].addr = (bits/4);
					/* We have one unknown label now, so record that. */
					(*numunknownlabels) = 1;
				}
				/* If the pointer to the collection of unknown labels is non-NULL, it should exists. */
				else
				{
					/* We have another unknown label, so record that. */
					/* As a note, the unknown labels collection (unlike the known labels collection) does not contain only unique label names. */
					/* This is because later when the label is declared every instance where it is referenced is replaced using the address location stored in the structure. */ 
					(*numunknownlabels)++;
					/* We have another label, so increase the size of the collection. */
					*unknownlabels = realloc(*unknownlabels, sizeof(label) * (*numunknownlabels));
					/* If we can't resize the memory, that sucks. */
					if(*unknownlabels == NULL)
					{
						/* Quit and complain. */
						perror("Could not reallocate memory");
						exit(11);
					}
					/* Save the label name we need to look for. */
					(*unknownlabels)[(*numunknownlabels) - 1].str = tempstr;
					/* And where we used it. */
					(*unknownlabels)[(*numunknownlabels) - 1].addr = (bits/4);
				}
			}
		}
	}
	/* If there is no error from strtol, the token after the assembly instruction is probably a numerical value. */
	else
	{
		/* Assume the programmer knows what he's doing and directly assign it. */
		address = tempaddress;
	}
	/* Return the final address found. */
	return address;
}

/* addstring() handles adding .ascii and .ascii (zero terminating ascii string) to the output buffer, including handling escape characters. */
void addstring(char *outbuf, char *string, char zeroterm, unsigned long long *bits, unsigned long long *bytes)
{
	/* Generic looping unit */
	unsigned int i = 0;
	/* We start the string with an opening double quote and keep reading until we find a closing double quote. */
	/* When we find it, we set this flag to 1 and get out. */
	char foundendquote = 0;
	/* The character that is written to the output buffer. */
	char charout = 0;
	
	/* If the first character is not a double quote, quit and complain. */
	/* We can actually function without it, but it may be a user mistake and it's good to have consistent behaviour. */
	if(string[0] != '"')
	{
		/* Quit and complain. */
		fprintf(stderr, "Line %llu: A string must begin with a quote.\n", FILELINE);
		exit(16);
	}
	/* Iterate through the string until we find the closing double quote */
	/* Make sure it isn't preceded by a \ (because that escapes the double quote and includes it in the string) and make sure it's actually at the end of the string (\0 is the next character). */
	for(i = 1; i < strlen(string); i++)
	{
		if(string[i] == '"' && string[i-1] != '\\' && string[i+1] == '\0')
		{
			/* If we found it, say so. */
			foundendquote = 1;
		}
	}
	/* If not... */
	if(!foundendquote)
	{
		/* Quit and complain. */
		/* Again, we can work without it, but this is a way of passively reducing user mistakes by forcing them to have well-formed strings. */
		fprintf(stderr, "Line %llu: A string must end with a quote.\n", FILELINE);
		exit(17);
	}
	/* Handle each character. */
	for(i = 1; i < (strlen(string) - 1); i++)
	{
		/* Handle escape characters */
		/* They all start with a \ */
		if(string[i] == '\\')
		{
			/* Then they have another character - this maps easily to the escape characters already present in C. */
			/* Null character */
			if(string[i+1] == '0')
			{
				charout = '\0';
			}
			/* Bell */
			else if(string[i+1] == 'a')
			{
				charout = '\a';
			}
			/* Backspace */
			else if(string[i+1] == 'b')
			{
				charout = '\b';
			}
			/* Form feed */
			else if(string[i+1] == 'f')
			{
				charout = '\f';
			}
			/* Newline */
			else if(string[i+1] == 'n')
			{
				charout = '\n';
			}
			/* Carriage return */
			else if(string[i+1] == 'r')
			{
				charout = '\r';
			}
			/* Tab */
			else if(string[i+1] == 't')
			{
				charout = '\t';
			}
			/* Vertical tab */
			else if(string[i+1] == 'v')
			{
				charout = '\v';
			}
			/* Escaped single quote */
			else if(string[i+1] == '\'')
			{
				charout = '\'';
			}
			/* Escaped double quote */
			else if(string[i+1] == '\"')
			{
				charout = '\"';
			}
			/* Escaped backslash (\) */
			else if(string[i+1] == '\\')
			{
				charout = '\\';
			}
			/* Escaped question mark. */
			else if(string[i+1] == '?')
			{
				charout = '\?';
			}
			/* If it's something we've never seen before, it's probably a user mistake. */
			else
			{
				/* Quit and complain. */
				fprintf(stderr, "Line %llu: Unknown escape sequences.\n", FILELINE);
				exit(18);
			}
			i++;
		}
		else
		{
			/* If it's not an escape character, we can just directly copy the character. */
			charout = string[i];
		}
		/* If the modulus of the current Bits and 8 is not zero, we need to write to the lower nibbles of the current byte and the higher nibbles of the next one. */
		if(((*bits) % 8) != 0)
		{
			/* We can assume the buffer is zero where it hasn't been written. */
			/* OR the higher 4 bits to the lower nibble of the first byte */
			outbuf[(*bytes)] |= ((charout >> 4) & 0x0F);
			/* Go up one byte */
			(*bytes)++;
			/* Go up 4 bits */
			(*bits) += 4;
			/* OR the lower 4 bits to the higher nibble of the second byte */
			outbuf[(*bytes)] |= ((charout << 4) & 0xF0);
			/* Go up 4 bits */
			(*bits) += 4;
		}
		/* If the modulus is zero, we can simply write the character directly to a byte. */
		else
		{
			/* Do so. */
			outbuf[(*bytes)] = charout;
			/* Go up one byte */
			(*bytes)++;
			/* And 8 bits */
			(*bits) += 8;
		}
	}
	/* If the string is supposed to be zero terminated, simply write a null character the same way as above after writing the string is complete. */
	if(zeroterm)
	{
		/* If the modulus of the current Bits and 8 is not zero, we need to write to the lower nibbles of the current byte and the higher nibbles of the next one. */
		charout = 0;
		if(((*bits) % 8) != 0)
		{
			/* We can assume the buffer is zero where it hasn't been written. */
			/* OR the higher 4 bits to the lower nibble of the first byte */
			outbuf[(*bytes)] |= ((charout >> 4) & 0x0F);
			/* Go up one byte */
			(*bytes)++;
			/* Go up 4 bits */
			(*bits) += 4;
			/* OR the lower 4 bits to the higher nibble of the second byte */
			outbuf[(*bytes)] |= ((charout << 4) & 0xF0);
			/* Go up 4 bits */
			(*bits) += 4;
		}
		/* If the modulus is zero, we can simply write the character directly to a byte. */
		else
		{
			/* Do so. */
			outbuf[(*bytes)] = charout;
			/* Go up one byte */
			(*bytes)++;
			/* And 8 bits */
			(*bits) += 8;
		}
	}
}

/* Finito! */
