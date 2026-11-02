"""Identify the observed Telltale static Lua registration instruction pattern."""
from pathlib import Path
import json,re,struct,pefile
ROOT=Path(__file__).resolve().parents[1]
def main():
    image=ROOT/'sources/WalkingDead101.analysis.exe'
    pe=pefile.PE(str(image));base=pe.OPTIONAL_HEADER.ImageBase
    sec=next(s for s in pe.sections if s.Name.startswith(b'.text'));code=sec.get_data()
    rows=[]
    for m in re.finditer(rb'\xa1(.{4})\x68(.{4})\x68(.{4})\x50\xb9(.{4})\xe8(.{4})',code,re.S):
        global_,string,func,record,disp=struct.unpack('<IIIII',b''.join(m.groups()))
        if not (base+sec.VirtualAddress<=func<base+sec.VirtualAddress+sec.Misc_VirtualSize): continue
        if not base+0x57e000<=string<base+0x670000: continue
        name=pe.get_data(string-base,200).split(b'\0',1)[0]
        if not re.fullmatch(rb'[A-Za-z_][A-Za-z_0-9]{1,150}',name):continue
        rows.append({'name':name.decode(),'function_rva':hex(func-base),'registration_rva':hex(sec.VirtualAddress+m.start()),'string_rva':hex(string-base),'record_rva':hex(record-base),'registrar_rva':hex((sec.VirtualAddress+m.end()+disp)&0xffffffff),'confidence':'instruction-pattern candidate; runtime ABI unverified'})
    (ROOT/'reports/native-bindings.json').write_text(json.dumps(rows,indent=2))
    print('Candidate native bindings:',len(rows))
if __name__=='__main__':main()
