LOD 	addressY	; Put Y into A
NND 	n15		; Negate Y
ADD 	n1		; Adds 1 to Negated Y (Finished Two’s Compliment)
ADD	addressX	; Add X to Negated Y (X-Y)
CXA 			; Copies Carry and XORb to A
NND 	n8	
NND 	n15		; ANDs with 1000 to remove carry bit and leave XORb
JMP 	TAG		; Jumps to tag if X >= Y else continue
LOD	addressY		; Put Y into A
NND	n15		; Negate Y
ADD	n1		; Add 1 to Negated Y (Finished Two’s Compliment)
ADD	addressX	; Add X to Negated Y (X-Y)
NND	addressY	; NAND with Y
NND n15		; Negate A = Z
NND n15		; X = X’
STR	addressX’	; Store X’ in Mem
LOD     addressY	; Load Y
NND	n15		; Y = Y’
NND	addressX’	; Y’ NAND X’ = Z
STR	addressX	; Store X into Mem
NND	addressY	; X NAND Y = Inter1
STR	addressInter1	; Store Intermediate 1 in Mem
NND	addressX	; Inter1 NAND X = Inter2
STR	addressInter2	; Store Intermediate 2 in Mem
LOD	addressInter1	; Load Intermediate 1 into Mem
NND	addressY	; Inter1 NAND Y =Inter3
NND	addressInter2	; Inter3 NAND Inter2 = Z