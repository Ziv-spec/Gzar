

.code

main:
  mov rbp, rsp
  push rbx
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15
  sub rsp, 56
  mov qword ptr [rsp+32], 0
  mov rbx, 2
  mov r10, 2
  cmp rbx, r10
  setg al
  movzx rax, al
  mov r11, rax
  mov [rsp+32], r11
  mov rbx, [rsp+32]
  mov rax, rbx
  jmp main_epilogue
main_epilogue:
  add rsp, 56
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop rbx
  ret


.data

PUBLIC main
END
