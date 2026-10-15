from pathlib import Path
HERE=Path(__file__).resolve().parent
asm=['OPTION CASEMAP:NONE','.code']
header=[]
for slot in range(3,54):
    name=f'fm5_device_forward_{slot}'
    header.append(f'extern void WINAPI {name}(void *, ...);')
    asm.extend([f'PUBLIC {name}',f'{name} PROC','    mov rcx, qword ptr [rcx + 10h]','    mov rax, qword ptr [rcx]',f'    jmp qword ptr [rax + 0{slot*8:X}h]',f'{name} ENDP'])
asm.append('END')
header.append('static UnknownFn device_forwarders[51] = {')
header.extend(f'    fm5_device_forward_{slot},' for slot in range(3,54));header.append('};')
(HERE/'device_forwarders.asm').write_text('\n'.join(asm)+'\n')
(HERE/'device_forwarders.h').write_text('\n'.join(header)+'\n')
