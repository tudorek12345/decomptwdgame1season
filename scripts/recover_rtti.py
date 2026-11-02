"""Recover x86 MSVC RTTI/vtable candidates with structural and range checks."""
from pathlib import Path
import json,re,struct,pefile
ROOT=Path(__file__).resolve().parents[1]
def main():
    pe=pefile.PE(str(ROOT/'sources/WalkingDead101.analysis.exe'));base=pe.OPTIONAL_HEADER.ImageBase
    image=pe.get_memory_mapped_image();u=lambda a:struct.unpack_from('<I',image,a-base)[0]
    relocations={e.rva for block in pe.DIRECTORY_ENTRY_BASERELOC for e in block.entries if e.type==3}
    readable=lambda a,n=4:base<=a<=base+len(image)-n
    code=lambda a:base+0x1000<=a<base+0x57d72c
    types={}
    for m in re.finditer(rb'\.\?A[UV][\x21-\x7e]{1,500}?\x00',image):
        if m.start()<8:continue
        at=base+m.start()-8
        if u(at+4)!=0:continue
        types[at]={'rva':hex(at-base),'decorated_name':m.group()[:-1].decode(),'type_info_vtable_rva':hex(u(at)-base)}
    locators={}
    rdata=next(s for s in pe.sections if s.Name.startswith(b'.rdata'))
    begin=base+rdata.VirtualAddress;end=begin+rdata.Misc_VirtualSize
    for a in range(begin,end-20,4):
        if u(a)!=0 or u(a+12) not in types:continue
        hierarchy=u(a+16)
        if not readable(hierarchy,16) or u(hierarchy)!=0:continue
        n=u(hierarchy+8);arr=u(hierarchy+12)
        if not 1<=n<=512 or not readable(arr,n*4):continue
        bases=[];valid=True
        for i in range(n):
            desc=u(arr+i*4)
            if not readable(desc,24) or u(desc) not in types:valid=False;break
            bases.append({'type_rva':hex(u(desc)-base),'decorated_name':types[u(desc)]['decorated_name'],'contained_bases':u(desc+4),'member_displacement':struct.unpack_from('<i',image,desc-base+8)[0],'vbtable_displacement':struct.unpack_from('<i',image,desc-base+12)[0],'vbase_displacement':struct.unpack_from('<i',image,desc-base+16)[0],'attributes':u(desc+20)})
        if valid:locators[a]={'rva':hex(a-base),'type':types[u(a+12)],'offset':u(a+4),'constructor_displacement':u(a+8),'hierarchy_attributes':u(hierarchy+4),'bases':bases}
    tables=[];targets={}
    for a in range(begin,end-8,4):
        loc=u(a)
        if loc not in locators or a-base not in relocations or a+4-base not in relocations or not code(u(a+4)):continue
        entries=[];p=a+4
        while p<end and p-base in relocations and code(u(p)) and len(entries)<1024:
            rva=hex(u(p)-base);entries.append(rva)
            targets.setdefault(rva,[]).append({'origin':'rtti_vtable','type':locators[loc]['type']['decorated_name'],'vtable_rva':hex(a+4-base),'slot':len(entries)-1});p+=4
        tables.append({'rva':hex(a+4-base),'locator':locators[loc],'function_rvas':entries,'slot_count':len(entries),'limit_reached':len(entries)==1024})
    bindings=json.loads((ROOT/'reports/native-bindings.json').read_text())
    for b in bindings:targets.setdefault(b['function_rva'],[]).append({'origin':'lua_registration','name':b['name']})
    (ROOT/'reports/rtti-vtables.json').write_text(json.dumps({'type_descriptors':list(types.values()),'complete_object_locators':len(locators),'vtables':tables,'caveat':'Validated metadata candidates, not recovered class field layouts. Every vtable slot must have an IMAGE_REL_BASED_HIGHLOW relocation and point into .text. Scanning stops at first non-pointer/non-code entry; adjacent pointer tables may need further separation.'},indent=2))
    (ROOT/'reports/function-candidates.json').write_text(json.dumps([{'rva':r,'evidence':v} for r,v in sorted(targets.items(),key=lambda x:int(x[0],16))],indent=2))
    known={json.loads(l)['rva'] for l in (ROOT/'recovered/native/functions.jsonl').read_text().splitlines()}
    print({'types':len(types),'locators':len(locators),'vtables':len(tables),'unique_targets':len(targets),'targets_without_known_entry':len(set(targets)-known)})
if __name__=='__main__':main()
