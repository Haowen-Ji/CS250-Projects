.text
# test bgt, beqz, jal, j, j instructions
addi $r3, $r0, 0
addi $r1, $r0, 15
addi $r2, $r0, 16

blt $r1, $r2, greater_than #branch is taken
halt #shouldn't be executed

greater_than:
jal my_func #procedure call
ret:
beq $r1, $r0, not_equal #branch is not taken
j end #jump to end
add $r1, $r2, $r2 #shouldn't be executed

my_func:
addi $r1, $r1, -14 #make $r1 equal to 1
j ret #return
addi $r1, $r2, 1 #shouldn't be executed
halt #shouldn't be executed

not_equal:
addi $r1, $r2, 1 #shouldn't be executed
halt #shouldn't be executed

end:
halt

.data
