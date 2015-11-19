#include "as4.h"

/* This is the main file */

unsigned long long FILELINE = 0;
uint16_t N_START = 0xFFFF;

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
		/* If we are in a PINF, treat every unknown tuple as a pseudo instruction with a data field. */
		unsigned char inpinf = 0;
		/* endptr is used to check if numbers are valid. */
		char *endptr = NULL;

		/* Check if a base address has been specified */
		if(!strncmp(argv[1], "-b", 3))
		{
			/* strtoul sets this endptr to the address of the first invalid character - if that's equal to argv[2], the entire string was invalid. */
			
			/* If yes, out input and output arguments are two elements farther in the array so increment arg */
			arg += 2;
			/* Get the base address from this conversion */
			baseaddr = estrtoul(argv[2], &endptr, STDHEX);
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
			char *trimed;
			
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
						/* Trim the whitespace */
						trimed = trim(tokens);

						/* If we get to a ; or # on the line, ignore the rest. Allows inline comments */
						if(trimed[0] == ';' || trimed[0] == '#')
						{
							break;
						}
						/* Again make sure the token is work dealing with */
						else if(trimed[0] != '\n' && trimed[0] != '\r')
						{
							/* Length of the token (useful for things) */
							size_t tokenlen = strlen(trimed);
							/* Certain assembly instructions don't require us to check the rest of the line, like NOP, so when we encounter these just add the instruction to the output buffer and go to the next line. */
							int doneline = 0;
							/* The address component of the instruction. Used in all instructions (though sometimes set to 0 and ignored). */
							unsigned short int address = 0;
							/* Remove the trailing newline. I think this was used more for displaying tokens during development, but it doesn't hurt. */
							if(trimed[tokenlen - 1] == '\n')
							{
								trimed[tokenlen - 1] = '\0';
							}
							/* Halt instruction */
							/* Easy one */
							if(!strncmp(trimed, "HLT", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									/* Address is optional - if it isn't there, just put zeros. */
									addinst(outbuf, HLT, NOADDR, &bits, &bytes);
								}
								else
								{
									/* Get the address location. */
									/* If it's just a number after the instruction, that will be returned with the base address added to it. */
									/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
									/* If it's an already declared label return the address relative to the base address. */
									address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
									/* Add this to the output buffer. */
									addinst(outbuf, HLT, address, &bits, &bytes);
								}

							}
							/* Load instruction */
							else if(!strncmp(trimed, "LOD", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									/* If not, print an error and quit. */
									fprintf(stderr, "Line %llu: A memory address must succeed a LOD instruction.\n", FILELINE);
									exit(8);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								/* Add this to the output buffer. */
								addinst(outbuf, LOD, address, &bits, &bytes);
							}
							/* Store instruction. */
							else if(!strncmp(trimed, "STR", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a STR instruction.\n", FILELINE);
									exit(12);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								/* Add this to the output buffer. */
								addinst(outbuf, STR, address, &bits, &bytes);
							}
							/* Add instruction. */
							else if(!strncmp(trimed, "ADD", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a ADD instruction.\n", FILELINE);
									exit(13);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								/* Add this to the output buffer. */
								addinst(outbuf, ADD, address, &bits, &bytes);
							}
							/* No-operation instruction */
							else if(!strncmp(trimed, "NOP", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									/* Address is optional - if it isn't there, just put zeros. */
									addinst(outbuf, NOP, NOADDR, &bits, &bytes);
								}
								else
								{
									/* Get the address location. */
									/* If it's just a number after the instruction, that will be returned with the base address added to it. */
									/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
									/* If it's an already declared label return the address relative to the base address. */
									address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
									/* Add this to the output buffer. */
									addinst(outbuf, NOP, address, &bits, &bytes);
								}

							}
							/* Start of the information section. Save as NOP with address 0xFFFF so the processor isn't bothered but we can tell later. */
							else if(!strncmp(trimed, "INF", 4))
							{
								/* ignore anything after INF on this line */
								doneline = 1;
								/* Add this to the output buffer. */
								addinst(outbuf, NOP, 0xFFFF, &bits, &bytes);

							}
							/* Start of the program information section within the information section. Save as a NOP with address 0xFFFF so the processor isn't bothered but we can tell later. */
							else if(!strncmp(trimed, "PINF", 5))
							{
								/* ignore anything after PINF on this line. */
								doneline = 1;
								/* Within the PINF...EPINF section, treat all unknown tuples as psuedo-instructions with a data section. */
								inpinf = 1;
								/* Add this to the output buffer */
								addinst(outbuf, NOP, 0xFFFF, &bits, &bytes);

							}
							/* Record the base address into the executable. If the base address is specified externally, use that. */
							else if(!strncmp(trimed, "BADR", 5))
							{

								free(trimed);
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a BADR instruction.\n", FILELINE);
									exit(33);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								/* If allow program argument baseaddr to take precedence */
								if(baseaddr == 0)
								{
									baseaddr = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
									/* If the base address has changed, we should check if there are labels that have been declared before this that need adjustment. */
									for(i = 0; i < numlabels; i++)
									{
										label *templabels = NULL;
										unsigned long long templabelnum = 0;

										addlabel(outbuf, &templabels, &unknownlabels, &templabelnum, &numunknownlabels, labels[i].str, (labels[i].addr * 4UL), baseaddr);
										labels[i].addr = templabels[0].addr;
									}
								}
								/* Add this to the output buffer. */
								addinst(outbuf, NOP, baseaddr, &bits, &bytes);
							}
							/* End of the program information section within the information section. Save as a NOP with address 0xFFFF so the processor isn't bothered but we can tell later. */
							else if(!strncmp(trimed, "EPINF", 6))
							{
								/* ignore everything after EPINF on this line. */
								doneline = 1;
								/* We're out of the PINF, actually ignore unknowns now. */
								inpinf = 0;
								/* Add to the output buffer */
								addinst(outbuf, NOP, 0xFFFF, &bits, &bytes);

							}
							/* Record the start of the data section. Allows easier disassembly. */
							else if(!strncmp(trimed, "DSEC", 5))
							{
								free(trimed);
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a DSEC instruction.\n", FILELINE);
									exit(34);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								/* Add this to the output buffer. */
								addinst(outbuf, NOP, address, &bits, &bytes);

							}
							/* Find where the N_ section starts */
							else if(!strncmp(tokens, "NSTART", 7))
							{
								tokens = strtok(NULL, delims);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(tokens == NULL || tokens[0] == '\n' || tokens[0] == '\r' || tokens[0] == '\0' || tokens[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a DSEC instruction.\n", FILELINE);
									exit(34);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								N_START = findlabel(&unknownlabels, &labels, tokens, numlabels, &numunknownlabels, bits, INST);
							}
							/* Each group of same size data sections should be recorded with the pair DNUM DSIZE */
							else if(!strncmp(trimed, "DNUM", 5))
							{
								free(trimed);
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A number of data fields must succeed a DNUM instruction.\n", FILELINE);
									exit(35);
								}
								/* Should just be a number - no labels! */
								address = estrtoul(trimed, &endptr, STDHEX);
								if(trimed == endptr)
								{
									fprintf(stderr, "Line %llu: A invalid number of data sections for DNUM.\n", FILELINE);
									exit(37);

								}
								/* Add this to the output buffer. */
								addinst(outbuf, NOP, address, &bits, &bytes);

							}
							/* Each group of same size data sections should be recorded with the pair DNUM DSIZE */
							else if(!strncmp(trimed, "DSIZE", 6))
							{
								free(trimed);
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A size must succeed a DSIZE instruction.\n", FILELINE);
									exit(36);
								}		
								/* Should just be a number - no labels! */
								address = estrtoul(trimed, &endptr, STDHEX);
								if(trimed == endptr)
								{
									fprintf(stderr, "Line %llu: A invalid size of data sections for DSIZE.\n", FILELINE);
									exit(38);

								}
								/* Add this to the output buffer. */
								addinst(outbuf, NOP, address, &bits, &bytes);

							}
							/* End of the information section. Record it. */
							else if(!strncmp(trimed, "EINF", 5))
							{
								/* Nothing else on this line matters */
								doneline = 1;
								/* Just in case someone forgot EPINF... */
								inpinf = 0;
								/* Prevent the address of EINF from getting written over... */
								if(unknownlabels != NULL)
								{
									for(i = 0; i < numunknownlabels; i++)
									{
										if((unknownlabels[i].str != NULL) && (!strcmp(unknownlabels[i].str, "N_")) && (unknownlabels[i].addr == (bits/4)))
										{
											char *emptystr = calloc(1, 2);
											memcpy(emptystr, "", 2);
											unknownlabels[i].str = emptystr;
											unknownlabels[i].addr = 0xFFFF;
										}	
									}
								}
								/* Add to output buffer. */
								addinst(outbuf, NOP, 0xFFFF, &bits, &bytes);
							}
							/* Nand instruction. */
							else if(!strncmp(trimed, "NND", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a NND instruction.\n", FILELINE);
									exit(14);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								/* Add this to the output buffer. */
								addinst(outbuf, NND, address, &bits, &bytes);
							}
							/* Jump instruction */
							else if(!strncmp(trimed, "JMP", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									fprintf(stderr, "Line %llu: A memory address must succeed a JMP instruction.\n", FILELINE);
									exit(15);
								}
								/* Get the address location. */
								/* If it's just a number after the instruction, that will be returned with the base address added to it. */
								/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
								/* If it's an already declared label return the address relative to the base address. */
								address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								/* Add this to the output buffer. */
								addinst(outbuf, JMP, address, &bits, &bytes);
							}
							/* Carry and XOR bits to Accumulator instruction */
							/* A very niche but important instruction. */
							else if(!strncmp(trimed, "CXA", 4))
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									/* Address is optional - if it isn't there, just put zeros. */
									addinst(outbuf, CXA, NOADDR, &bits, &bytes);
								}
								else
								{
									/* Get the address location. */
									/* If it's just a number after the instruction, that will be returned with the base address added to it. */
									/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
									/* If it's an already declared label return the address relative to the base address. */
									address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
									/* Add this to the output buffer. */
									addinst(outbuf, CXA, address, &bits, &bytes);
								}
							}
							/* .data arbitrary data section inputs */
							else if(!strncmp(trimed, ".data", 6))
							{
								/* Size of the data section */
								unsigned long long datasize = 0;
								/* Initial value */
								long long datavalue = 0;
								
								/* Once we're done dealing with the .data element, we will have taken every useful token from this line, so declared the line done. */
								doneline = 1;
								free(trimed);
								/* Get the next token (which should be the size of the .data element). */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check that the token is valid. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									/* If not, warn the user. And quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element.\n", FILELINE);
									exit(7);
								}
								/* Errno can be set non-zero for many reasons. Set it to zero now so we can check the result of strtoul. */
								errno = 0;
								/* Try to get the size of the data element */
								datasize = estrtoul(trimed, &endptr, STDHEX);
								/* Check the data size is valid by checking both errno and if the string was simply invalid (tokens == endptr, endptr points to the first invalid character) */
								if(errno != 0 || trimed == endptr)
								{
									/* If either is true, warn the user and quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element or an invalid value was used.\n", FILELINE);
									exit(7);
								}
								free(trimed);
								/* Get the next token (which should be the initial value of the .data element). */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check that the token is valid. */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									/* If not, warn the user. And quit. */
									fprintf(stderr, "Line %llu: Both a size and a value must be declared for a .data element.\n", FILELINE);
									exit(7);
								}
								/* Errno can be set non-zero for many reasons. Set it to zero now so we can check the result of strtoul. */
								errno = 0;
								/* Try to get the initial value of the data element */
								datavalue = estrtol(trimed, &endptr, STDHEX);
								/* If tokens == endptr, this isn't a number, so it should be a label. */
								if(errno != 0 || trimed == endptr)
								{
									/* However, if the storage size isn't 4, the programmer isn't using this correctly. */
									if(datasize != 4)
									{
										/* Complain and die. */
										fprintf(stderr, "Line %llu: When a .data declaration is used with a label, the storage size must be 4.\n", FILELINE);
										exit(31);
									}
									/* Otherwise, try and get the address of that label. */
									datavalue = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, LABEL);
								}
								free(trimed);
								/* Get the next token as this will probably be a newline, and this speeds up the reading process for the next line. */
								tokens = strtok(NULL, delims);
								/* After we have gotten the size and initial value, add this to the output buffer. */
								adddata(&outbuf, bufsize, datasize, datavalue, &bits, &bytes);
							}
							/* .ascii and .asciiz string data sections. */
							/* .asciiz is the zero terminated version. */
							else if(!strncmp(trimed, ".ascii", 7) || !strncmp(trimed, ".asciiz", 8))
							{
								/* Zero termination flag */
								char zeroterm = 0;
								/* The total string string - we can't use tokens here alone, they remove spaces we actually need. */
								char *string = calloc(1, (size_t)linelen);
								
								/* If it's .asciiz, set the zero termination flag. */
								if(!strncmp(trimed, ".asciiz", 8))
								{
									zeroterm = 1;
								}
								free(trimed);
								/* Get the next token, which should be the string. */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check if it is simply an empty string */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0')
								{
									/* If it is quit and print an error. */
									fprintf(stderr, "Line %llu: A string is required after .ascii or .asciiz.\n", FILELINE);
									exit(15);
								}
								/* If it's a space, it might just be a weird whitespace glitch. */
								else if(trimed[0] == ' ')
								{
									free(trimed);
									/* So try to get the token again. */
									tokens = strtok(NULL, delims);
									trimed = trim(tokens);
									/* If it's still invalid, there is probably no string there. */
									if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
									{
										/* Complain and quit. */
										fprintf(stderr, "Line %llu: A string is required after .ascii or .asciiz.\n", FILELINE);
										exit(15);
									}
								}
								/* Loop until we reach the end of the line. */
								while(!(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0'))
								{
									/* Concatenate all the tokens on this line together with their missing whitespace to make the actual stored string. */
									strcat(string, trimed);
									free(trimed);
									tokens = strtok(NULL, delims);
									trimed = trim(tokens);
									if(!(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0'))
									{
										strcat(string, " ");
									}
									else
									{
										break;
									}
								}
								/* Add it to the output buffer. */
								addstring(outbuf, string, zeroterm, &bits, &bytes);
							}
							/* If the currently token ends in an colon, it's probably a label. */
							else if(trimed[strlen(trimed)-1] == ':')
							{
								/* Add the label to the list of known labels and replace any references to it with it's actual address. */
								addlabel(outbuf, &labels, &unknownlabels, &numlabels, &numunknownlabels, trimed, bits, baseaddr);
							}
							else if(inpinf)
							{
								free(trimed);
								/* Get the next token, which is likely a label (though it could be a number) */
								tokens = strtok(NULL, delims);
								trimed = trim(tokens);
								/* Check firstly that there is a valid label or address after the instruction. */
								/* If not, assume the pseudo instruction wants a 0 data field */
								if(trimed == NULL || trimed[0] == '\n' || trimed[0] == '\r' || trimed[0] == '\0' || trimed[0] == ' ')
								{
									address = 0;
								}
								else
								{
									/* Get the address location. */
									/* If it's just a number after the instruction, that will be returned with the base address added to it. */
									/* If it's a yet undeclared label, 65535 (UNKNOWNADDR) is returned. The instruction will be modified when the label is declared. */
									/* If it's an already declared label return the address relative to the base address. */
									address = findlabel(&unknownlabels, &labels, trimed, numlabels, &numunknownlabels, bits, INST);
								}
								/* Add this to the output buffer. */
								addinst(outbuf, NOP, address, &bits, &bytes);
							}
							/* If we're done the line, don't both getting another token. */
							if(doneline)
							{
								break;
							}
						}
						/* But if we're not done the line, get another token. */
						free(trimed);
						tokens = strtok(NULL, delims);
					}
				}	
			}
			/* Next line - increment FILELINE. */
			FILELINE++;
			/* Free line and set len to zero - otherwise we are leaking memory. */
			free(line);
			line = NULL;
			len = 0;
			/* And get another line. */
			linelen = getline(&line, &len, input);
		}
		/* If the modulo of bits and 8 is not zero, we have a fractional amount of bytes. */
		/* Since x86 machines and their operating systems are based on using the byte as the atomic unit, we cannot write fractional bytes to a file. */
		/* We also don't want to mess up the file and add something that would cause unexpected behaviour (like a nibble of zero padding). */
		if((bits % 8) != 0)
		{
			/* So we add a single NOP. */
			addinst(outbuf, NOP, EOFADDR, &bits, &bytes);
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
						/* if both str is "" and addr is 0xFFFF, this was an unknown label we were meant to ignore. */
						if(!strcmp(labels[j].str, unknownlabels[i].str) || (!strcmp("", unknownlabels[i].str) && (unknownlabels[i].addr == 0xFFFF)))
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

/* Whitespace trimming function. */
char *trim(char *str)
{
	unsigned int i = 0;
	unsigned int j = 0;
	char empty[1] = {'\0'};
	char *retstr = NULL;

	if(str == NULL)
	{
		return NULL;
	}
	/* Trim leading whitespace */
	while(isspace((unsigned char)str[i]))
	{	
		i++;
		/* If we travel past the end of the string, the string is empty. */
		if(i > strlen(str))
		{
			retstr = empty;
			return retstr;
		}
	}
	retstr = calloc(1, strlen(str) + 1);
	if(retstr == NULL)
	{
		perror("Could not allocate memory");
		exit(4);
	}
	/* Trim trailing whitespace. */
	for(i = i, j = 0; i < strlen(str) && j < strlen(str); i++, j++)
	{
		if(isspace((unsigned char)str[i]))
		{
			break;
		}
		retstr[j] = str[i];
	}
	/* Terminate the string here as it is the end, and stop some potential weirdness. */
	retstr[j] = '\0';
	return retstr;
}

/* Help() prints the help. More useful in large programs */
void help(void)
{
	puts("This is the low level assembler for the Nibble Knowledge 4-bit computer");
	puts("");
	puts("Usage: as4 [-b base_address] input output");

}

/* Finito! */
