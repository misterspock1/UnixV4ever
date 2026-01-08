start:
    jsr r5,print; msg
    clr r0
    sys exit

msg:    <Hello, world!\n>; .even

print:
    mov (r5),r0
    jsr r5,size
    mov r4,0f
    mov (r5)+,0f-2
    mov $1,r0
    sys 0; 7f
.data
7:
    sys write; ..; 0:..
.text
    rts r5

size:
    clr r4
1:
    inc r4
    cmpb (r0)+,$'\n
    bne 1b
    rts r5