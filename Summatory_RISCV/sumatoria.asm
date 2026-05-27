.text
main:
    li t0, 1
    li t1, 40

push_loop:
    addi sp, sp, -4
    sw t0, 0(sp)
    addi t0, t0, 1
    ble t0, t1, push_loop

    li a0, 0
    li t0, 40

pop_loop:
    lw t2, 0(sp)
    add a0, a0, t2
    addi sp, sp, 4
    addi t0, t0, -1
    bnez t0, pop_loop

end:
    j end