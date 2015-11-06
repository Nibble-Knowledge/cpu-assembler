CPU Assembler
===
The assembler for the Nibble Knowledge CPU.

AS4
---

AS4 is the low level assembler written in C99 for the Nibble Knowledge CPU. It is under 1000 lines of executable code, and recognises only 12 language constructs.

### Useage ###
There are only two invocations of as4:
* ./as4 -b BASE_ADDRESS INPUT OUTPUT
* ./as4 INPUT OUTPUT

Where INPUT is your assembly language text file, OUTPUT is your binary file output, and BASE_ADDRESS is the numerical value (in hex, octal, decimal or binary) of the base address for compilation where all memory addresses are calculated relative to. BASE_ADDRESS is assumed to be 0 if not specified.

### 8 instructions ###
The Nibble Knowledge CPU has 8 instructions which are split into two different types:
* Solitary instructions that do not require a memory address (has the format INST)
	* NOP: No operation
	* HLT: Halt CPU
	* CXA: Copy the carry bit (soon to be overflow bit?) and the XOR of that bit and the MSB of the accumulator.
* Binary instructions that require a memory address (has the formal INST ADDR)
	* ADD: Add the nibble at the specified memory address to the accumulator.
	* NND: NAND the nibble at the specified memory address to the accumulator.
	* JMP: Jump to the specified memory address if the accumulator is 0.
	* LOD: Load the accumulator with the nibble at the specified address.
	* STR: Store the nibble in the accumulator to the specified memory address.



### Pseudo instructions ###
To aid in disassembly a new metadata format for binary files is now included in the assembler as of v1.1.0.
* INF - the information section must start with this, and this should be the first instruction of any file.
* PINF - the start of the executable information section. Any unknown tuples within the executable information section are treated as pseudo instructions with a data field.
* BADR - The base address of the binary file. Must be in the executable data section.
* EPINF - the end of the executable information section.
* DSEC - the memory location of the data section which should succeed the text section. Should be a label so that it is modified by the base address and can be reliably disassembled. If there is no data section, this should point to the end of the file.
* Data section descriptors:
	* DNUM -  the amount of data sections of the following size
	* DSIZE -  the size
	* There should be as many DNUM/DSIZE pairs as there are unique groups of data sections. The example below is illustrative. These pairs must be in the same order as the data sections themselves.
* EINF - end of the information section.

An example is below:
```nasm
INF
PINF
BADR 0x40
EPINF
DSEC datastart
DNUM 0x2
DSIZE 0x3
DNUM 0x1
DSIZE 0x9
EINF
LOD hello
LOD isitmeyourelookingfor
datastart:
hello: .data 3 0x2
isitmeyourelookingfor: .data 3 0x2
.data 9 0x0
```

### 2 data types ###
AS4 recognises two inbuilt data types:
* Numberical values. Format: ".data SIZE INITIALVALUE"
	* .data can also be used to create a static reference to a label. The SIZE must be 4. The format is ".data SIZE LABEL" or ".data SIZE LABEL[OFFSET]". This will save the static 16-bit memory location pointed to by LABEL or LABEL + OFFSET to the .data section.
* Strings, both plain and zero terminated. Format: ".ascii "String"" or ".asciiz "String""
	* Strings must start and end with double quotes.
	* AS4 recognises standard escape characters

### Labels ###
Labels are of the format "NAME:". They are used to refer to memory locations without having to memorise or calculate number.
An example of useage would be "number: .data 1 2", which is using the label "number" to point to a data element of 1 nibble in size with the initial value of 2.

Labels when referenced in instructions can be used in two forms:
* INST LABEL
	* Where the instruction INST simply references the memory location pointed to by LABEL
* INST LABEL[OFFSET]
	* Where the instruction INST references the memory location pointed to by LABEL + OFFSET. OFFSET is usually a hexidecimal value, optionally preceded by "0x". To use a binary value, prefix with "0b". For an octal value, prefix "0" or "0o". For a decimal value, prefix "0d".

An example of usage would be "LOD sum[F]", which loads the memory address pointed to by "sum" plus the offset of "F" (15 in decimal) into the accumulator.

### Address of operations ###
As all programs built by AS4 are assumed to be static, non-relocatable binary files, the addresses pointed by labels can be calculated at assemble time and used statically. The form is below:
* &(LABEL[OFFSET])[ADDRESS_OFFSET]
	* LABEL[OFFSET] is the same as for the standard label usage. &() indicates this is an address of operation, and [ADDRESS_OFFSET] is what 4-bit portion of the 16-bit address you want. Both [OFFSET] and [ADDRESS_OFFSET] are optional, without them an offset of zero is assumed.

This loads a corresponding value from the table of static values - for example, if the address of the label "Carmen" is 0x00FE, and you use the address of operation LOD &(Carmen)[1], you would load "0xF" into the accumulator; which is the same as using the instruction LOD N_[F] when the N_ static number series is defined.

### Comments ###
Comments in AS4 start with a semicolon, ";" or an octothorp, "#".

### Numbers ###
AS4 accepts binary, octal, hexadecimal and binary numbers. Binary numbers must always be preceded by "0b" and octal by "0" or "0o". The rules for decimal and hexadecimal vary depending on use case.
* When used for an offset, in the form LABEL[OFFSET], it is assumed the default is hexadecimal. Thus, hexadecimal numbers can be written with or without a preceding "0x". Decimal numbers must be written with a preceding "0d".
* In all other cases, decimal is assumed to be the default. "0d" can optionally precede the decimal number. Hexadecimal numbers must be written with a preceding "0x".

### Example code ###
```nasm
; This program reads in integers and adds them together
; until a negative number is read in.  Then it outputs
; the sum (not including the last number).

;Start:	read		; read n -> acc
;	jmpn  Done  	; jump to Done if n < 0.
;	add   sum  	; add sum to the acc
;	store sum 	; store the new sum
;	jump  Start	; go back & read in next number
;Done:	load  sum 	; load the final sum
;	write 		; write the final sum
;	stop  		; stop
;
;sum:	.data 2 0 ; 2-byte location where sum is stored

;; START NIbble-Knowledge code ;;

;; Instruction Section ;;

Start: 
	LOD n15
	ADD n2
	NND n15
	NOP
	CXA 
	JMP Jump
	STR sum
	
Jump:	HLT 


;; Data Section ;;
n0: .data  1 0  ; b0000
n1: .data  1 1  ; b0001
n2: .data  1 2  ; b0010
n3: .data  1 3  ; b0011
n4: .data  1 4  ; b0100
n5: .data  1 5  ; b0101
n6: .data  1 6  ; b0110
n7: .data  1 7  ; b0111
n8: .data  1 8  ; b1000
n9: .data  1 9  ; b1001
n10: .data 1 10 ; b1010
n11: .data 1 11 ; b1011
n12: .data 1 12 ; b1100
n13: .data 1 13 ; b1101
n14: .data 1 14 ; b1110
n15: .data 1 15 ; b1111

derp: .data 6 0x454534

derp3: .ascii "hey there"
derp4: .ascii "hey\" there\n"
derp5: .asciiz "hey there"
derp6: .asciiz "hey\" there\n"

sum: .data 1 0
```
