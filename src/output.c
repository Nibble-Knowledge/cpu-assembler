#include "as4.h"

/* These are the operations that output binary data to the buffer */

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
	for(i = 1; i < strlen(string) - 1; i++)
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