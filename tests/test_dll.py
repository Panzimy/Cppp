import ctypes
import os
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent.parent

DLL_PATH = PROJECT_DIR / "native" / "system_memory.dll"
MINGW_BIN = PROJECT_DIR / "mingw64" / "bin"

# Přidání portable MinGW knihoven
dll_directory = os.add_dll_directory(str(MINGW_BIN))

if not DLL_PATH.is_file():
    raise FileNotFoundError(DLL_PATH)

# Načtení nativní DLL
dll = ctypes.CDLL(str(DLL_PATH))

dll.sm_alloc.argtypes = [ctypes.c_uint64]
dll.sm_alloc.restype = ctypes.c_uint64

dll.sm_write.argtypes = [
    ctypes.c_uint64,
    ctypes.c_uint64,
    ctypes.c_uint8
]
dll.sm_write.restype = ctypes.c_int

dll.sm_read.argtypes = [
    ctypes.c_uint64,
    ctypes.c_uint64,
    ctypes.POINTER(ctypes.c_uint8)
]
dll.sm_read.restype = ctypes.c_int

dll.sm_free.argtypes = [ctypes.c_uint64]
dll.sm_free.restype = ctypes.c_int

address = dll.sm_alloc(64)

if not address:
    raise RuntimeError("VirtualAlloc failed")

try:
    print("Allocated:", hex(address))

    if not dll.sm_write(address, 0, 123):
        raise RuntimeError("Write failed")

    result = ctypes.c_uint8()

    if not dll.sm_read(address, 0, ctypes.byref(result)):
        raise RuntimeError("Read failed")

    print("Read value:", result.value)

finally:
    if not dll.sm_free(address):
        raise RuntimeError("VirtualFree failed")

    print("Memory released")
