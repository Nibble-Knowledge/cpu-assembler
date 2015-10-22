#a comment
NND lit[F]
NND lit[F]
#Trying a nested Macro
NND lit[F]
ADD lit[1]
#The carry macro
CXA 0
NND lit[1]
NND lit[F]

#simple acc stuff - being exhaustive in my tests
ADD lit[1]
ADD lit[F]

#shifts
STR macro[0]
ADD macro[0]

STR macro[0]
ADD macro[0]
STR macro[0]
CXA 0
NND lit[1]
NND lit[F]
ADD macro[0]

#unary macros! Testing simple first
LOD lit[f]
STR place[0]

#testing that the math works
LOD place[3]
STR elsewhere[a]
LOD place[4]
STR elsewhere[b]
LOD place[5]
STR elsewhere[c]
LOD place[6]
STR elsewhere[d]
LOD place[7]
STR elsewhere[e]
LOD place[8]
STR elsewhere[f]
LOD place[9]
STR elsewhere[10]
LOD place[a]
STR elsewhere[11]
LOD place[b]
STR elsewhere[12]
LOD place[c]
STR elsewhere[13]
LOD place[d]
STR elsewhere[14]
LOD place[e]
STR elsewhere[15]
LOD place[f]
STR elsewhere[16]
LOD place[10]
STR elsewhere[17]
LOD place[11]
STR elsewhere[18]
LOD place[12]
STR elsewhere[19]

#does recursion between acc and unary work?
LOD somewhere[1f]
NND lit[F]
STR place[0]

#more complex recursion - 8-bit negations!
LOD source[0]
NND lit[F]
STR dest[0]
LOD source[1]
NND lit[F]
ADD lit[1]
STR dest[1]
CXA 0
NND lit[1]
NND lit[F]
ADD dest[0]
STR dest[0]

#test addition
LOD lit[0]
ADD num[1]
STR macro[1]
CXA 0
NND lit[1]
NND lit[F]
NND lit[F]
STR macro[0]
LOD macro[1]
ADD num[1]
STR sum[1]
CXA 0
NND lit[1]
NND lit[F]
NND lit[F]
NND macro[0]
ADD num[0]
STR macro[1]
CXA 0
NND lit[1]
NND lit[F]
NND lit[F]
STR macro[0]
LOD macro[1]
ADD num[0]
STR sum[0]
CXA 0
NND lit[1]
NND lit[F]
NND lit[F]
NND macro[0]

macro: .data 50 1
lit: .data 50 1
source: .data 50 1
place: .data 50 1
elsewhere: .data 50 1
somewhere: .data 50 1
dest: .data 50 1
num: .data 50 1
sum: .data 50 1
