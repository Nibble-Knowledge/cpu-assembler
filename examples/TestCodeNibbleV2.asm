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




