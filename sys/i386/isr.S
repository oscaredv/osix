.globl isr_common
isr_common:
  pusha
  movl %ds, %eax
  pushl %eax

  movl $0x10, %eax
  movl %eax, %ds
  movl %eax, %es
  movl %eax, %fs
  movl %eax, %gs

  pushl %esp  /* struct interrupt_frame parameter */
  call isr_handler
  addl $4, %esp
   
  popl %eax
  movl %eax, %ds
  movl %eax, %es
  movl %eax, %fs
  movl %eax, %gs

  popa
  addl $8, %esp
  iret

.globl sysent_trap
sysent_trap:
  pusha
  movl %ds, %eax
  pushl %eax

  movl $0x10, %eax
  movl %eax, %ds
  movl %eax, %es
  movl %eax, %fs
  movl %eax, %gs

  pushl %esp  /* struct interrupt_frame parameter */
  call sysent
  addl $4, %esp

.globl sysleave
sysleave:
  popl %eax
  movl %eax, %ds
  movl %eax, %es
  movl %eax, %fs
  movl %eax, %gs

  popa
  addl $8, %esp
  iret

.globl isr0
isr0:
  pushl $0
  pushl $0
  jmp isr_common
.globl isr1
isr1:
  pushl $0
  pushl $1
  jmp isr_common
.globl isr2
isr2:
  pushl $0
  pushl $2
  jmp isr_common
.globl isr3
isr3:
  pushl $0
  pushl $3
  jmp isr_common
.globl isr4
isr4:
  pushl $0
  pushl $4
  jmp isr_common
.globl isr5
isr5:
  pushl $0
  pushl $5
  jmp isr_common
.globl isr6
isr6:
  pushl $0
  pushl $6
  jmp isr_common
.globl isr7
isr7:
  pushl $0
  pushl $7
  jmp isr_common
.globl isr8
isr8:
  pushl $8
  jmp isr_common
.globl isr9
isr9:
  pushl $0
  pushl $9
  jmp isr_common
.globl isr10
isr10:
  pushl $10
  jmp isr_common
.globl isr11
isr11:
  pushl $11
  jmp isr_common
.globl isr12
isr12:
  pushl $12
  jmp isr_common
.globl isr13
isr13:
  pushl $13
  jmp isr_common
.globl isr14
isr14:
  pushl $14
  jmp isr_common
.globl isr15
isr15:
  pushl $0
  pushl $15
  jmp isr_common
.globl isr16
isr16:
  pushl $0
  pushl $16
  jmp isr_common
.globl isr17
isr17:
  pushl $17
  jmp isr_common
.globl isr18
isr18:
  pushl $0
  pushl $18
  jmp isr_common
.globl isr19
isr19:
  pushl $0
  pushl $19
  jmp isr_common
.globl isr20
isr20:
  pushl $0
  pushl $20
  jmp isr_common
.globl isr21
isr21:
  pushl $0
  pushl $21
  jmp isr_common
.globl isr22
isr22:
  pushl $0
  pushl $22
  jmp isr_common
.globl isr23
isr23:
  pushl $0
  pushl $23
  jmp isr_common
.globl isr24
isr24:
  pushl $0
  pushl $24
  jmp isr_common
.globl isr25
isr25:
  pushl $0
  pushl $25
  jmp isr_common
.globl isr26
isr26:
  pushl $0
  pushl $26
  jmp isr_common
.globl isr27
isr27:
  pushl $0
  pushl $27
  jmp isr_common
.globl isr28
isr28:
  pushl $0
  pushl $28
  jmp isr_common
.globl isr29
isr29:
  pushl $0
  pushl $29
  jmp isr_common
.globl isr30
isr30:
  pushl $0
  pushl $30
  jmp isr_common
.globl isr31
isr31:
  pushl $0
  pushl $31
  jmp isr_common
.globl isr32
isr32:
  pushl $0
  pushl $32
  jmp isr_common
.globl isr33
isr33:
  pushl $0
  pushl $33
  jmp isr_common
.globl isr34
isr34:
  pushl $0
  pushl $34
  jmp isr_common
.globl isr35
isr35:
  pushl $0
  pushl $35
  jmp isr_common
.globl isr36
isr36:
  pushl $0
  pushl $36
  jmp isr_common
.globl isr37
isr37:
  pushl $0
  pushl $37
  jmp isr_common
.globl isr38
isr38:
  pushl $0
  pushl $38
  jmp isr_common
.globl isr39
isr39:
  pushl $0
  pushl $39
  jmp isr_common
.globl isr40
isr40:
  pushl $0
  pushl $40
  jmp isr_common
.globl isr41
isr41:
  pushl $0
  pushl $41
  jmp isr_common
.globl isr42
isr42:
  pushl $0
  pushl $42
  jmp isr_common
.globl isr43
isr43:
  pushl $0
  pushl $43
  jmp isr_common
.globl isr44
isr44:
  pushl $0
  pushl $44
  jmp isr_common
.globl isr45
isr45:
  pushl $0
  pushl $45
  jmp isr_common
.globl isr46
isr46:
  pushl $0
  pushl $46
  jmp isr_common
.globl isr47
isr47:
  pushl $0
  pushl $47
  jmp isr_common
.globl isr128
isr128:
  pushl $0
  pushl $0x80
  jmp sysent_trap
