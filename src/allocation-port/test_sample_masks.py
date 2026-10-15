"""Execute the pinned original native mask block, rather than a mirrored formula."""
from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import ctypes as C
import hashlib
import random
import struct

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile

def verify(dll):
    original = (ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe').read_bytes()
    assert hashlib.sha256(original).hexdigest() == '579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc'
    image = pefile.PE(data=original).get_memory_mapped_image()
    block = image[0x130a77:0x130ad4]
    assert len(block)==93 and block.hex()=='413bc27458458b98c00100008bc841b9ffffffff41d3e14523cb458bc74585d274340f1f8000000000418bd3418bc8d3eaf6c201741883f804730f83f8027507f7d183e101eb03418bcf410fabc941ffc0453bc272d344898d60010000'
    kernel=C.WinDLL('kernel32',use_last_error=True)
    kernel.VirtualAlloc.argtypes=[C.c_void_p,C.c_size_t,C.c_uint,C.c_uint]
    kernel.VirtualAlloc.restype=C.c_void_p
    kernel.VirtualFree.argtypes=[C.c_void_p,C.c_size_t,C.c_uint]
    memory=kernel.VirtualAlloc(None,4096,0x3000,0x40)
    assert memory
    wrapper=bytes.fromhex('5541574883ec284c89c54881ed600100004989c889d0458b90680200004531ff49bb')+struct.pack('<Q',memory)+bytes.fromhex('41ffd38b85600100004883c428415f5dc3')
    C.memmove(memory,block+b'\xc3',94)
    C.memmove(memory+256,wrapper,len(wrapper))
    invoke=C.WINFUNCTYPE(C.c_uint,C.c_void_p,C.c_uint,C.POINTER(C.c_uint))(memory+256)
    mapped=dll.FM5MapNativeSampleMask
    mapped.argtypes=[C.c_uint]*4; mapped.restype=C.c_uint
    dll.FM5SetMsaaTestHost.argtypes=[C.c_void_p]
    host=C.create_string_buffer(0x319000)
    struct.pack_into('<H',host,0,0x5a4d);struct.pack_into('<I',host,0x3c,0x80)
    struct.pack_into('<I',host,0x80,0x4550);struct.pack_into('<H',host,0x98,0x20b)
    struct.pack_into('<I',host,0xd0,0x319000);struct.pack_into('<I',host,0x318ea0,2)
    dll.FM5SetMsaaTestHost(host)
    desc=C.create_string_buffer(0x290)
    randomizer=random.Random(9894)
    masks=list(range(256))+[0xffffffff,0x80000000]+[randomizer.getrandbits(32) for _ in range(256)]
    checks=0
    for effective in [1,2,4,8,16]:
        for count in [1,2,4,8,16]:
            for mask in masks:
                struct.pack_into('<I',desc,0x1c0,mask)
                struct.pack_into('<II',desc,0x268,count,2)
                result=C.c_uint(mask)
                expected=invoke(desc,effective,C.byref(result))
                assert expected==result.value
                actual=mapped(effective,count,2,mask)
                assert actual==expected,(effective,count,hex(mask),hex(actual),hex(expected))
                checks+=1
    dll.FM5SetMsaaTestHost(None)
    assert kernel.VirtualFree(memory,0,0x8000)
    return {'checks':checks,'pinned_original_block_executed':True,'all_nonforced_modes_exact':True}
