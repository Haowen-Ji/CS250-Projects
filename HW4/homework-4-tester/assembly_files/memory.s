.text
# test lw, sw instructions
addi $r2, $r0, 3  #initializing r2,r3
addi $r3, $r0, 4

la $r4, val

sw $r2, 0($r4)
sw $r3, 1($r4)

addi $r2, $r0, 0  #reset r2,r3 to 0
addi $r3, $r0, 0

lw $r2, 0($r4)
lw $r3, 1($r4)

halt

.data
val: .word 300 400
