MOVC R0,#2
MOVC R1,#8
MOVC R2,#4
MOVC R3,#1
MOVC R4,#4
STORE R0,R2,#0
ADD R2,R2,R4
SUBL R1,R1,#0
CMP R2,R1
BZ #-16
MOVC R0,#0
MOVC R1,#8
MOVC R2,#4
LOAD R5,R2,#0
ADD R5,R5,R0
ADD R0,R0,R3
STORE R5,R2,#0
ADD R2,R2,R4
SUB R1,R1,R3
CMP R0,R1
BNZ #-28
HALT
MOVC R3,#10
