#ifndef _AS4_H_
#define _AS4_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
/* This is a 4-bit assembler for the Nibble Knowledge computer */
/* Hacked together by Ryan Harvey Oct 12 2015 - for a minicompter design that should be from the 60s! */
/* This was essentially hacked together in a single day - beware! */

/* These are the opcodes for each instruction. Defined here for clarity */
#define HLT 0
#define LOD 1
#define STR 2
#define ADD 3
#define NOP 4
#define NND 5
#define JMP 6
#define CXA 7

/* All instructions have the form INST MEMLOC */
/* Where INST is a 4-bit opcode and MEMLOC is a 16-bit address */
/* For instructions HLT, NOP, and CXA MEMLOC is ignored and NOADDR below is used */

/* These are the default memory locations for memory locations when nothing is specified */
#define NOADDR 0x0000
#define EOFADDR 0x4444
#define UNKNOWNADDR 0xFFFF

/* Macros to determine if we are using strange, macro-generated hex values (no 0x) or standard. */
#define STDHEX 0
#define NSTDHEX 1

/* When we are using labels references labels, we need account for the 4 nibble size instead of 5 for an instruction */
#define INST 0
#define LABEL 1

/* The singular global variable - what line of the assembly file we are on. Helps with error messages */
extern unsigned long long FILELINE;

/* This is the data structure used to identify labels */
typedef struct _label
{
	uint16_t addr;
	char *str;
	uint16_t offset;
	uint8_t type;
} label;

/* Help() prints the help. More useful in large programs */
void help(void);
/* addinst() appends an instruction to the output buffer and handles bytesplitting */
void addinst(char *outbuf, uint8_t op, uint16_t addr, unsigned long long *bits, unsigned long long *bytes);
/* adddata() handles adding .data statements to the output buffer. It also handles resizing the buffer if need be. */
void adddata(char **outbuf, size_t bufsize, unsigned long long size, long long value, unsigned long long *bits, unsigned long long *bytes);
/* addlabel() adds a label to the collection of labels, to be used by other functions when a label reference is made. */
/* Additionally, if there are outstanding "queries" for a certain label (if the label has been used before it has been declared) it replaces the "unknown address" address in an instruction with the address of the label on declaration */
void addlabel(char *outbuf, label **labels, label **unknownlabels, unsigned long long *numlabels, unsigned long long numunknownlabels, char *labelstr, unsigned long long bits, unsigned short int baseaddr);
/* findlabel() determines the memory address that follows the opcode. If the memory address is already a number, returns it */
/* If it is an undeclared label, it adds the label name and the instruction location to the "unknown labels" collection. If the label is declared, it returns it's address */
unsigned short int findlabel(label **unknownlabels, label **labels, char *labelstr, unsigned long long numlabels, unsigned long long *numunknownlabels, unsigned long long bits, uint8_t type);
/* addstring() handles adding .ascii and .ascii (zero terminating ascii string) to the output buffer, including handling escape characters. */
void addstring(char *outbuf, char *string, char zeroterm, unsigned long long *bits, unsigned long long *bytes);

/* estrtol (extended strtol) and estrtoul (extended strtoul) are functions to intelligently determine the base of the number to be converted by the strtol functions and return them. */
/* They function the same as strtol except for the final parameter they accept a type parameter which switches them from expecting hex values in the form 0xF (STDHEX or 0) to just F (NSTDHEX or 1). This is to accomodate the macro assembler. */
/* Switching to NSTDHEX means the default assumption for numbers is hex instead of decimal, so 12 will be assume to be 0x12, or 1. Decimal values will be accepted in this mode in the form 0d9. */
long estrtol(char *str, char **endptr, uint8_t type);

unsigned long estrtoul(char *str, char **endptr, uint8_t type);

/* Whitespace trimming function. */
char *trim(char *str);

#endif /* _AS4_H_ */
