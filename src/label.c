#include "as4.h"

/* These are the label operations. */

/* addlabel() adds a label to the collection of labels, to be used by other functions when a label reference is made. */
/* Additionally, if there are outstanding "queries" for a certain label (if the label has been used before it has been declared) it replaces the "unknown address" address in an instruction with the address of the label on declaration */
void addlabel(char *outbuf, label **labels, label **unknownlabels, unsigned long long *numlabels, unsigned long long *numunknownlabels, char *labelstr, unsigned long long bits, unsigned short int baseaddr)
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
	strcpy(tempstr, labelstr);
	/* Strip the trailing ":" */
	for(i = 0; i < strlen(tempstr); i++)
	{
		if(tempstr[i] == ':')
		{
			tempstr[i] = '\0';
			break;
		}
	}
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
	/* If the label is for N_, set N_START. */
	if(!(strcmp(tempstr, "N_")))
	{
		/* If we've defined it already, don't let redefinition. */
		if(N_START != 0xFFFF)
		{
			fprintf(stderr, "Line %llu: N_ already defined!.\n", FILELINE);
			exit(45);
		}
		N_START = ((bits/4) + baseaddr);
	}
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
			for(i = 0; i < (*numunknownlabels); i++)
			{
				/* If the unknown label name exits (otherwise it would be hard to identify it)... */
				if((*unknownlabels)[i].str != NULL && strcmp((*unknownlabels)[i].str, ""))
				{
					/* Check if the name of the unknown label is the same as the label that we just had declared. */
					if(!strcmp((*unknownlabels)[i].str, tempstr))
					{
						/* If it is, then take stock of both the address it was referenced. If a label is referencing a label, we need to move 1 nibble back (as there is no instruction, just 4 nibbles). Cheaper than doing an if below. */
						unsigned short int instaddress = (*unknownlabels)[i].addr - ((*unknownlabels)[i].type & 1);
						/* And the address the label points to plus the requested offset. We need to add one nibble if it is an instruction referencing a nibble as we moved one back above. */
						unsigned short int labeladdr = (*labels)[(*numlabels) - 1].addr + (*unknownlabels)[i].offset + ((*unknownlabels)[i].type & 1);
						/* If we are doing an address of operation... */
						if(((*unknownlabels)[i].type & 2))
						{
							/* If N_START is not defined, we don't know how to load just a number. Wait until we do by storing a label to be found later... */
							if(N_START == 0xFFFF)
							{
								char *nstr = calloc(1, 6);
								/* As this is getting a portion of the address of the label accessed, we get the label's address and bitwise and that section then shift it back. */
								sprintf(nstr, "N_[%X]", (labeladdr & (0xF << ((*unknownlabels)[i].addroffset * 4))) >> ((*unknownlabels)[i].addroffset * 4));
								labeladdr = findlabel(unknownlabels, labels, nstr, (*numlabels), numunknownlabels, instaddress * 4UL, ((*unknownlabels)[i].type) & 1);
								continue;
							}
							else
							{
								labeladdr = ((labeladdr & (0xF << ((*unknownlabels)[i].addroffset * 4))) >> ((*unknownlabels)[i].addroffset * 4)) + N_START;
							}
						}
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
unsigned short int findlabel(label **unknownlabels, label **labels, char *labelstr, unsigned long long numlabels, unsigned long long *numunknownlabels, unsigned long long bits, uint8_t type)
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
	/* offset stores the offset in nibbles from the specific label. */
	unsigned short int offset = 0;
	unsigned short int addroffset = 0;

	if(type > 1)
	{
		fprintf(stderr, "findlabel: Type can only be 0 or 1\n");
		exit(33);
	}
	if(labelstr == NULL)
	{
		fprintf(stderr, "findlabel: labelstr cannot be null!\n");
		exit(34);
	}
	/* Set errno to 0 so we can check for errors from strtol, which will tell us if the token after the assembly instruction is a number or a label (if it's neither we find out a bit later, not in this function). */
	errno = 0;
	/* tempaddress is 0 if the labelstr is actually a label and not a value. */
	tempaddress = estrtol(labelstr, &endptr, STDHEX);
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
		/* Remove any possible whitespace */
		for(i = 0; i < strlen(labelstr); i++)
		{
			if(isspace((unsigned char)labelstr[i]))
			{
				break;
			}
			tempstr[i] = labelstr[i];
		}		
		tempstr[i] = '\0';
		
		/* Check to see if this is an address of operation */
		if(tempstr[0] == '&')
		{
			/* The formatting for this operation is very specific and we can find failures in multiple levels of parsing */
			/* Have one value we set */
			unsigned char formatcorrect = 0;
			
			/* The second character must be a ( */
			if(tempstr[1] == '(')
			{
				/* Go until we find the ), which encompasses the label we are taking the address of */
				for(i = 2; i < strlen(tempstr); i++)
				{
					if(tempstr[i] == ')')
					{
						/* If the ) is right beside the end of the string, it means that it is in the form &(LABEL). This is ok */
						if(tempstr[i+1] == '\0')
						{
							formatcorrect = 1;
						}
						/* If the ) is right beside [, the string should be in the form &(LABEL)[ADDRESS_OFFSET] */
						else if(tempstr[i+1] == '[')
						{
							/* Try to convert the offset */
							addroffset = estrtol(tempstr + ((i + 2) * sizeof(char)), &endptr, NSTDHEX);
							/* If the value is valid (there is a convertable number)... */
							if(tempstr != endptr)
							{
								/* And if the first invalid character was ] and then the string ends, this is correctly in the form &(LABEL)[ADDRESS_OFFSET] */
								if(endptr[0] == ']' && endptr[1] == '\0')
								{
									/* However, the offset must be between 0 and 3 as addresses are 4 nibbles. */
									if(addroffset > 3)
									{
										fprintf(stderr, "Line %llu: The offset of an address of operation must be between 0 and 3.\n", FILELINE);
										exit(51);
									}
									formatcorrect = 1;
								}
							}
						}
					}
					if(formatcorrect)
					{
						/* Bitwise or ADDROF (which is 2) to the type. We can use this in addlabel to know if we are doing an address of operation. */
						type |= ADDROF;
						/* Strip the address of parts from this string so we can use the rest of the function to correctly fill the string, address and offset parts of the unknownlabel structure. */
						for(i = 2; i < strlen(tempstr); i++)
						{
							if(tempstr[i] == ')')
							{
								tempstr[i-2] = '\0';
								break;
							}
							tempstr[i-2] = tempstr[i];
						}
					}
				}
			}
			/* If the string has not passed all the trials, punish the user. */
			if(formatcorrect == 0)
			{
				fprintf(stderr, "Line %llu: Format of an address of operation must be &(LABEL[OFFSET])[ADDRESS_OFFSET].\n", FILELINE);
				exit(52);
			}
		}

		/* Search for the square brackets to determine label offset */
		for(i = 0; i < strlen(tempstr); i++)
		{
			if(tempstr[i] == '[')
			{
				/* The macro assembler produces hex offsets that have no 0x, and usually hex offsets only. Assume this. */
				offset = estrtol(tempstr + ((i + 1) * sizeof(char)), &endptr, NSTDHEX);
				/* Check the offset is in the correct form. If the last character isn't ] for both where we stopped reading the string to find the offset value and where the string ends, then it's wrong. */
 				/* If both are ], but they're not pointing to the same place (ie: LABEL[OFFSET]uh oh]) then it's still wrong. */
				if((*endptr) != ']' || tempstr[strlen(tempstr) - 1] != ']' || (tempstr + ((strlen(tempstr) - 1) * sizeof(char))) != endptr)
				{
					fprintf(stderr, "Line %llu: Label offsets must be declared in the form LABEL[OFFSET].\n", FILELINE);
					exit(26);
				}
				tempstr[i] = '\0';
			}
		}
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
							/* If they are, use the address the label points to and add the offset we want. */
							address = (*labels)[i].addr + offset;
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
					/* and the offset, so we can add it later. */
					(*unknownlabels)[0].offset = offset;
					(*unknownlabels)[0].addroffset = addroffset;
					/* and whether a label or instruction is referencing this */
					(*unknownlabels)[0].type = type;
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
					/* And the offset */
					(*unknownlabels)[(*numunknownlabels) - 1].offset = offset;
					(*unknownlabels)[(*numunknownlabels) - 1].addroffset = addroffset;
					/* and the type */
					(*unknownlabels)[(*numunknownlabels) - 1].type = type;
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
