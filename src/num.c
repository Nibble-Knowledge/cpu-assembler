#include "as4.h"

/* These are simply utility functions to wrap strtol (string to long) and strtoul (string to unsigned long) so that we can effectively use decimal, octal, binary, and hexadecimal together. */
/* The formats are as follows: */
/*
Decimal: If type is STDHEX or 0, decimal is the assumed default. It can also be specified by using the prefix 0d. If NSTDHEX or 1 is specified, only by prefixing 0d can you use decimal.
Octal: Prefixing with either 0 or 0o is supported.
Binary: Must prefix with 0b.
Hexadecimal: If type is STDHEX or 0, must prefix with 0x. If type is NSTDHEX or 1, hexadecimal is the assumed default, and can also be specified by using the prefix 0x.
*/

long estrtol(char *str, char **endptr, uint8_t type)
{
	if(type > 1 || str == NULL || endptr == NULL)
	{
		(*endptr) = str;
		return 0;
	}
	else if(str[0] == '0')
	{
		if(isdigit((unsigned char)str[1]))
		{
			return strtol(str, endptr, 8);
		}
		else if(str[1] == 'o')
		{
			return strtol((str + 2), endptr, 8);
		}
		else if(str[1] == 'x')
		{
			return strtol(str, endptr, 16);
		}
		else if(str[1] == 'd')
		{
			return strtol((str + 2), endptr, 10);
		}
		else if(str[1] == 'b')
		{
			return strtol((str + 2), endptr, 2);
		}
	}
	else
	{
		if(type == STDHEX)
		{
			return strtol(str, endptr, 10);
		}
		else if(type == NSTDHEX)
		{
			return strtol(str, endptr, 16);
		}
	}
	return strtol(str, endptr, 0);
}


unsigned long estrtoul(char *str, char **endptr, uint8_t type)
{
	if(type > 1 || str == NULL || endptr == NULL)
	{
		(*endptr) = str;
		return 0;
	}
	else if(str[0] == '0')
	{
		if(isdigit((unsigned char)str[1]))
		{
			return strtoul(str, endptr, 8);
		}
		else if(str[1] == 'o')
		{
			return strtoul((str + 2), endptr, 8);
		}
		else if(str[1] == 'x')
		{
			return strtoul(str, endptr, 16);
		}
		else if(str[1] == 'd')
		{
			return strtoul((str + 2), endptr, 10);
		}
		else if(str[1] == 'b')
		{
			return strtoul((str + 2), endptr, 2);
		}
	}
	else
	{
		if(type == STDHEX)
		{
			return strtoul(str, endptr, 10);
		}
		else if(type == NSTDHEX)
		{
			return strtoul(str, endptr, 16);
		}
	}
	return strtoul(str, endptr, 0);

}
