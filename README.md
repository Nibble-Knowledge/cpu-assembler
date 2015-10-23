CPU Assembler
===
The assembler for the Nibble Knowledge CPU.

AS4
---

AS4 is the low level assembler on written in C99 for the Nibble Knowledge CPU. It is under 700 lines of executable code, and recognises only 12 constructs:

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

### 2 data types ###
AS4 recognises two inbuilt data types:
* Numberical values. Format: ".data SIZE INITIALVALUE"
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
	* Where the instruction INST references the memory location pointed to by LABEL + OFFSET. OFFSET must be a hexidecimal value, optionally preceded by "0x".

An example of usage would be "LOD sum[F]", which loads the memory address pointed to by "sum" plus the offset of "F" (15 in decimal) into the accumulator.

### Comments ###
Comments in AS4 start with a semicolon, ";" or an octothorp, "#".

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
