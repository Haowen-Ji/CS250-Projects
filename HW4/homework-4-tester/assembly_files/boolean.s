.text
# test not, xor instrctions

addi $r1, $r0, 15
addi $r2, $r0, 12

#and
and $r3, $r1, $r2
and $r6, $r3, $r2

#or
or $r4, $r3, $r2
or $r5, $r6, $r1

halt

.data
