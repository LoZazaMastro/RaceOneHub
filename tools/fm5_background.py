"""Validate native pre-game artwork through Windows WIC without opening a window."""

from pathlib import Path




def validate_image(path):
    """Decode with the same Windows WIC service used by the native pre-game."""
    import ctypes as c
    import uuid
    ole = c.OleDLL('ole32')
    ole.CoInitializeEx.argtypes = [c.c_void_p, c.c_uint]
    ole.CoInitializeEx.restype = c.c_long
    initialized = ole.CoInitializeEx(None, 0)
    if initialized < 0:
        raise OSError('Unable to initialize the image decoder.')
    pointers = []
    def guid(value):
        return (c.c_ubyte*16).from_buffer_copy(uuid.UUID(value).bytes_le)
    def method(pointer, slot, result, *types):
        address = c.cast(pointer, c.POINTER(c.POINTER(c.c_void_p)))[0][slot]
        return c.WINFUNCTYPE(result, c.c_void_p, *types)(address)
    def check(result):
        if result < 0:
            raise OSError('Windows could not decode the background (HRESULT %08x).' % (result & 0xffffffff))
    try:
        factory = c.c_void_p()
        ole.CoCreateInstance.argtypes = [c.c_void_p,c.c_void_p,c.c_uint,c.c_void_p,c.c_void_p]
        ole.CoCreateInstance.restype = c.c_long
        check(ole.CoCreateInstance(guid('cacaf262-9370-4615-a13b-9f5539da4c0a'), None, 1,
                                  guid('ec5ec8a9-c395-4314-9c77-54d7a935ff70'), c.byref(factory)))
        pointers.append(factory)
        decoder = c.c_void_p()
        check(method(factory,3,c.c_long,c.c_wchar_p,c.c_void_p,c.c_uint,c.c_uint,c.c_void_p)(
            factory,str(Path(path).resolve()),None,0x80000000,0,c.byref(decoder)))
        pointers.append(decoder)
        frame = c.c_void_p()
        check(method(decoder,13,c.c_long,c.c_uint,c.c_void_p)(decoder,0,c.byref(frame)))
        pointers.append(frame)
        width,height = c.c_uint(),c.c_uint()
        check(method(frame,3,c.c_long,c.c_void_p,c.c_void_p)(frame,c.byref(width),c.byref(height)))
        if not 32 <= width.value <= 8192 or not 32 <= height.value <= 8192 or width.value*height.value > 33554432:
            raise ValueError('Background must be between 32 and 8192 pixels per side, at most 32 megapixels.')
        codec = c.OleDLL('windowscodecs')
        codec.WICConvertBitmapSource.argtypes = [c.c_void_p,c.c_void_p,c.c_void_p]
        codec.WICConvertBitmapSource.restype = c.c_long
        converted = c.c_void_p()
        check(codec.WICConvertBitmapSource(guid('6fddc324-4e03-4bfe-b185-3d77768dc90f'),frame,c.byref(converted)))
        pointers.append(converted)
        stride = width.value*4
        pixels = c.create_string_buffer(stride*height.value)
        check(method(converted,7,c.c_long,c.c_void_p,c.c_uint,c.c_uint,c.c_void_p)(
            converted,None,stride,len(pixels),pixels))
        return {'width':width.value,'height':height.value,'windows_wic_decode_passed':True}
    finally:
        for pointer in reversed(pointers):
            method(pointer,2,c.c_ulong)(pointer)
        ole.CoUninitialize()
