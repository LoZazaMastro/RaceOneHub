"""Exact replacement for the pinned native INI antialiasing-reader block."""
import struct
START_RVA=0x7e2ec
END_RVA=0x7e3fc

def antialiasing_reader_code(start, end, target):
    if end-start != END_RVA-START_RVA:
        raise ValueError('Unexpected native antialiasing-reader extent')
    code=bytearray.fromhex('488d4d20418b5504')
    code += b'\xe8'+struct.pack('<i',target-start-len(code)-5)
    code += bytes.fromhex('41894504')
    code += b'\xe9'+struct.pack('<i',end-start-len(code)-5)
    code += b'\x90'*(end-start-len(code))
    return code

# This setter guard runs before graphics-device creation. Keep its later
# device-exists rejection, pending option, observer notification and actual
# render-thread application exactly as in the original native function.
VALIDATOR_RVA=0x558b3
VALIDATOR_ERROR_RVA=0x55a38
VALIDATOR_BEFORE=bytes.fromhex('8d41fea9fdffffff740983f9010f8572010000')

def antialiasing_validator_leaf():
    # ECX is preserved; EAX is BOOL, flags volatile. Upper bound prevents BT's
    # modulo32 indexing from accidentally accepting33,34,36 or40.
    return bytes.fromhex('83f908770fb8160100000fa3c80f92c00fb6c0c331c0c3')

def antialiasing_validator_patch(start, target, error):
    code=b'\xe8'+struct.pack('<i',target-start-5)
    code+=bytes.fromhex('85c00f84')+struct.pack('<i',error-start-len(code)-8)
    return code+b'\x90'*(len(VALIDATOR_BEFORE)-len(code))
