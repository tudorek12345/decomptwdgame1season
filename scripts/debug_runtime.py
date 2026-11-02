"""Bounded native startup smoke test (Windows debug events, no UI automation).

Starts its own process, records OutputDebugString/exception events, and terminates
only that process after --seconds. Run with the game closed.
"""
import argparse
import ctypes as C
from ctypes import wintypes as W
from pathlib import Path
import time
import json
import hashlib


class STARTUPINFO(C.Structure):
    _fields_ = [("cb", W.DWORD), ("reserved", W.LPWSTR), ("desktop", W.LPWSTR),
                ("title", W.LPWSTR), ("x", W.DWORD), ("y", W.DWORD),
                ("xsize", W.DWORD), ("ysize", W.DWORD), ("xchars", W.DWORD),
                ("ychars", W.DWORD), ("fill", W.DWORD), ("flags", W.DWORD),
                ("show", W.WORD), ("cb2", W.WORD), ("res2", C.c_void_p),
                ("stdin", W.HANDLE), ("stdout", W.HANDLE), ("stderr", W.HANDLE)]


class PROCESSINFO(C.Structure):
    _fields_ = [("process", W.HANDLE), ("thread", W.HANDLE), ("pid", W.DWORD), ("tid", W.DWORD)]


class DEBUGEVENT(C.Structure):
    # The union starts at offset 16 and is 160 bytes in the 64-bit debugger ABI.
    _fields_ = [("code", W.DWORD), ("pid", W.DWORD), ("tid", W.DWORD),
                ("pad", W.DWORD), ("data", C.c_ubyte * 160)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=float, default=25)
    parser.add_argument("--output", type=Path, default=Path("build/startup-smoke.json"))
    parser.add_argument("--variant",choices=['binary','hybrid'],default='binary')
    parser.add_argument("--game", type=Path, default=Path(__file__).resolve().parents[1]/'runtime')
    args = parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    if args.game.resolve()!=root/'runtime':raise RuntimeError('Only the isolated project runtime is permitted')
    report='binary-rebuild.json' if args.variant=='binary' else 'hybrid-build.json'
    executable='WalkingDead101.exe' if args.variant=='binary' else 'WalkingDead101.hybrid.exe'
    expected=json.loads((root/'reports'/report).read_text(encoding='utf-8-sig'))['sha256']
    actual=hashlib.sha256((args.game/executable).read_bytes()).hexdigest()
    if actual!=expected:raise RuntimeError('Runtime executable differs from recorded rebuilt image')
    counter_rva=None;counter_names=[];dll_hash=None
    if args.variant=='hybrid':
        import pefile
        dll=args.game/'twd_reconstructed.dll';dll_hash=hashlib.sha256(dll.read_bytes()).hexdigest()
        build=json.loads((root/'reports'/report).read_text(encoding='utf-8-sig'))
        if dll_hash!=build['dll_sha256']:raise RuntimeError('Replacement DLL differs from recorded build')
        exports=pefile.PE(str(dll)).DIRECTORY_ENTRY_EXPORT.symbols
        counter_rva=next(x.address for x in exports if x.name==b'twd_reconstruction_counters')
        counter_names=[row['name'] for row in build['replacements']]
    k = C.WinDLL("kernel32", use_last_error=True)
    k.CreateProcessW.argtypes = [W.LPCWSTR, W.LPWSTR, C.c_void_p, C.c_void_p, W.BOOL, W.DWORD,
                                C.c_void_p, W.LPCWSTR, C.POINTER(STARTUPINFO), C.POINTER(PROCESSINFO)]
    k.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    k.CloseHandle.argtypes = [W.HANDLE]
    k.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
    k.WaitForSingleObject.argtypes = [W.HANDLE, W.DWORD]
    startup = STARTUPINFO(cb=C.sizeof(STARTUPINFO))
    proc = PROCESSINFO()
    exe = args.game / executable
    if not k.CreateProcessW(str(exe), None, None, None, False, 2, None, str(args.game), C.byref(startup), C.byref(proc)):
        raise C.WinError(C.get_last_error())
    events = []
    modules=[]
    image_base=None
    started = time.monotonic()
    alive = True
    replacement_calls={}
    try:
        while time.monotonic() - started < args.seconds:
            event = DEBUGEVENT()
            if not k.WaitForDebugEvent(C.byref(event), 200):
                continue
            raw = bytes(event.data)
            record = {"elapsed": round(time.monotonic() - started, 3), "event": event.code}
            status = 0x00010002  # DBG_CONTINUE
            if event.code == 1:
                record.update(exception=hex(int.from_bytes(raw[:4], "little")),
                              address=hex(int.from_bytes(raw[16:24], "little")),
                              first_chance=int.from_bytes(raw[152:156], "little"))
                address=int(record['address'],16)
                if image_base is not None:record['main_module_relative_address']=hex(address-image_base)
                if record["exception"] != "0x80000003":
                    status = 0x80010001  # DBG_EXCEPTION_NOT_HANDLED
            elif event.code == 8:
                address = int.from_bytes(raw[:8], "little")
                wide = int.from_bytes(raw[8:10], "little")
                size = int.from_bytes(raw[10:12], "little") * (2 if wide else 1)
                buffer = C.create_string_buffer(min(size, 65536))
                count = C.c_size_t()
                if k.ReadProcessMemory(proc.process, address, buffer, len(buffer), C.byref(count)):
                    record["text"] = buffer.raw[:count.value].decode("utf-16-le" if wide else "utf-8", "replace").rstrip("\0")
            elif event.code == 5:
                record["exit_code"] = int.from_bytes(raw[:4], "little")
                alive = False
            if event.code in (1, 5, 8):
                events.append(record)
            # Close debugger-supplied image/DLL handles.
            if event.code in (3, 6):
                handle = int.from_bytes(raw[:8], "little")
                if event.code==3:image_base=int.from_bytes(raw[24:32],'little')
                module_base=image_base if event.code==3 else int.from_bytes(raw[8:16],'little')
                buffer=C.create_unicode_buffer(32768)
                k.GetFinalPathNameByHandleW.argtypes=[W.HANDLE,W.LPWSTR,W.DWORD,W.DWORD]
                path=buffer.value if not handle else ''
                if handle and k.GetFinalPathNameByHandleW(handle,buffer,len(buffer),0):path=buffer.value
                modules.append({'base':hex(module_base),'path':path})
                if handle:
                    k.CloseHandle(handle)
            k.ContinueDebugEvent(event.pid, event.tid, status)
            if not alive:
                break
    finally:
        if alive:
            if counter_rva is not None:
                module=next((m for m in modules if m['path'].lower().endswith('\\twd_reconstructed.dll')),None)
                if module:
                    counters=(W.DWORD*len(counter_names))();count=C.c_size_t()
                    if k.ReadProcessMemory(proc.process,int(module['base'],16)+counter_rva,counters,C.sizeof(counters),C.byref(count)) and count.value==C.sizeof(counters):
                        replacement_calls=dict(zip(counter_names,map(int,counters)))
            k.TerminateProcess(proc.process, 0)
            k.WaitForSingleObject(proc.process, 5000)
        k.CloseHandle(proc.thread)
        k.CloseHandle(proc.process)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"pid": proc.pid, "exe":str(exe),"sha256":actual,"image_base":hex(image_base) if image_base else None,"modules":modules,"terminated_at_timeout": alive,
                                      "events": events,"replacement_dll_sha256":dll_hash,"source_replacement_calls":replacement_calls}, indent=2))
    print(f"Recorded {len(events)} debug events in {args.output}")
    for event in events[-15:]:
        print(event)


if __name__ == "__main__":
    main()
