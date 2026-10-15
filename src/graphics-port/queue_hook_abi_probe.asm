option casemap:none
EXTERN FM5DiagnosticExecuteCommandLists:PROC
PUBLIC FM5QueueHookAbiProbe
PUBLIC FM5QueueHookCandidateStub
.code
; A test caller reproduces the native submission owner's R15 and [RBP-8].
FM5QueueHookAbiProbe PROC FRAME
    push rbp
    .pushreg rbp
    push r15
    .pushreg r15
    sub rsp,58h
    .allocstack 58h
    .endprolog
    lea rbp,[rsp+40h]
    mov r15,r9
    mov rax,[rsp+90h]
    mov [rbp-8],rax
    xor r9d,r9d
    call FM5QueueHookCandidateStub
    add rsp,58h
    pop r15
    pop rbp
    ret
FM5QueueHookAbiProbe ENDP

; Exact proposed replacement stub: entry RSP%16=8, caller's original frame
; remains in RBP. Allocate new shadow space and the fifth argument; unwindable.
FM5QueueHookCandidateStub PROC FRAME
    sub rsp,38h
    .allocstack 38h
    .endprolog
    mov r9,r15
    mov rax,[rbp-8]
    mov [rsp+20h],rax
    call FM5DiagnosticExecuteCommandLists
    add rsp,38h
    ret
FM5QueueHookCandidateStub ENDP
END
