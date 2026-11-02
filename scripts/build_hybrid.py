"""Install the checked C++ replacement manifest through an added PE import table.
No injection into any running process. Original installation remains untouched.
"""
from pathlib import Path
import hashlib,json,struct,shutil,subprocess
import pefile
ROOT=Path(__file__).resolve().parents[1]
def align(x,n):return (x+n-1)//n*n
def main():
    dll=ROOT/'build/x86/Release/twd_reconstructed.dll'
    validation=[]
    # Tie every hybrid build to fresh execution of its exact compiled library.
    for tool,filename,with_dll in [
        ('geometry_differential','geometry-comparison.json',False),
        ('geometry_differential','geometry-dll-comparison.json',True),
        ('hash_symbol_differential','hash-symbol-static-comparison.json',False),
        ('hash_symbol_differential','hash-symbol-dll-comparison.json',True)]:
        exe=ROOT/'build/x86/Release'/(tool+'.exe')
        command=[str(exe)]+([str(dll)] if with_dll else [])
        result=subprocess.run(command,capture_output=True,text=True,cwd=ROOT/'build',check=True)
        comparison=json.loads(result.stdout)
        assert comparison['failed']==0,comparison
        if 'bit_exact' in comparison:assert comparison['bit_exact']==comparison['cases']
        comparison['test_exe_sha256']=hashlib.sha256(exe.read_bytes()).hexdigest()
        if with_dll:comparison['dll_sha256']=hashlib.sha256(dll.read_bytes()).hexdigest()
        (ROOT/'reports'/filename).write_text(json.dumps(comparison,indent=2)+'\n')
        validation.append({'report':'reports/'+filename,'cases':comparison['cases'],'failed':0})
    source=ROOT/'runtime/WalkingDead101.exe';b=bytearray(source.read_bytes())
    expected=json.loads((ROOT/'reports/binary-rebuild.json').read_text(encoding='utf-8-sig'))['sha256']
    assert hashlib.sha256(b).hexdigest()==expected
    pe=pefile.PE(data=b);base=pe.OPTIONAL_HEADER.ImageBase
    fa,sa=pe.OPTIONAL_HEADER.FileAlignment,pe.OPTIONAL_HEADER.SectionAlignment
    rva=align(pe.OPTIONAL_HEADER.SizeOfImage,sa);raw=align(len(b),fa)
    section_header=pe.sections[-1].get_file_offset()+40
    assert section_header+40<=pe.OPTIONAL_HEADER.SizeOfHeaders
    old_import=pe.OPTIONAL_HEADER.DATA_DIRECTORY[1]
    descs=bytearray()
    for item in pe.DIRECTORY_ENTRY_IMPORT:descs+=item.struct.__pack__()
    new_desc_offset=len(descs);descs+=bytes(40)
    data=descs
    def append(blob,alignment=4):
        data.extend(bytes(align(len(data),alignment)-len(data)));at=len(data);data.extend(blob);return rva+at
    name=append(b'twd_reconstructed.dll\0')
    replacements=json.loads((ROOT/'re/replacements.json').read_text())
    for comparison in ['hash-symbol-static-comparison.json','hash-symbol-dll-comparison.json']:
        assert json.loads((ROOT/'reports'/comparison).read_text(encoding='utf-8-sig'))['failed']==0
    hints=[append(b'\0\0'+row['export'].encode('ascii')+b'\0',2) for row in replacements]
    thunks=struct.pack('<'+'I'*(len(hints)+1),*hints,0)
    lookup=append(thunks);iat=append(thunks)
    struct.pack_into('<5I',data,new_desc_offset,lookup,0,0,name,iat)
    reloc=pe.OPTIONAL_HEADER.DATA_DIRECTORY[5]
    relbytes=pe.get_data(reloc.VirtualAddress,reloc.Size)
    # Every FF 25 absolute-IAT jump needs its own HIGHLOW relocation.
    # Reject existing relocations in overwritten bytes to avoid double relocation.
    pages={}
    old_relocs={e.rva for block in pe.DIRECTORY_ENTRY_BASERELOC for e in block.entries if e.type==3}
    for row in replacements:
        site=int(row['rva'],16)
        assert not any(site-3<=x<site+6 for x in old_relocs), ('Overlapping old relocation',row)
        pages.setdefault((site+2)&~4095,[]).append(0x3000|((site+2)&4095))
    newrel=bytearray(relbytes)
    for page,entries in sorted(pages.items()):
        if len(entries)%2:entries.append(0)
        newrel+=struct.pack('<II',page,8+2*len(entries))+struct.pack('<'+'H'*len(entries),*entries)
    reloc_rva=append(newrel)
    virtual_size=len(data);raw_size=align(virtual_size,fa);data.extend(bytes(raw_size-len(data)))
    b.extend(bytes(raw-len(b)));b.extend(data)
    header=struct.pack('<8sIIIIIIHHI',b'.twdsrc\0',virtual_size,rva,raw_size,raw,0,0,0,0,0xc0000040)
    b[section_header:section_header+40]=header
    struct.pack_into('<H',b,pe.FILE_HEADER.get_field_absolute_offset('NumberOfSections'),pe.FILE_HEADER.NumberOfSections+1)
    struct.pack_into('<I',b,pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfImage'),align(rva+virtual_size,sa))
    struct.pack_into('<I',b,pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfInitializedData'),pe.OPTIONAL_HEADER.SizeOfInitializedData+raw_size)
    struct.pack_into('<II',b,old_import.get_file_offset(),rva,len(descs))
    struct.pack_into('<II',b,reloc.get_file_offset(),reloc_rva,len(newrel))
    for index,row in enumerate(replacements):
        site=pe.get_offset_from_rva(int(row['rva'],16))
        assert b[site:site+6]==bytes.fromhex(row['expected_prefix']),row
        row['new_import_iat_rva']=hex(iat+index*4)
        b[site:site+6]=b'\xff\x25'+struct.pack('<I',base+iat+index*4)
    output=ROOT/'runtime/WalkingDead101.hybrid.exe';output.write_bytes(b)
    shutil.copy2(dll,ROOT/'runtime'/dll.name)
    check=pefile.PE(str(output));assert check.DIRECTORY_ENTRY_IMPORT[-1].dll==b'twd_reconstructed.dll'
    new_relocs={e.rva for block in check.DIRECTORY_ENTRY_BASERELOC for e in block.entries if e.type==3}
    assert all(int(row['rva'],16)+2 in new_relocs for row in replacements)
    exported={e.name.decode() for e in pefile.PE(str(dll)).DIRECTORY_ENTRY_EXPORT.symbols if e.name}
    assert all(row['export'] in exported for row in replacements)
    assert len(check.DIRECTORY_ENTRY_IMPORT[-1].imports)==len(replacements)
    report={'sha256':hashlib.sha256(b).hexdigest(),'input_sha256':expected,'output':str(output),'entry_rva':hex(check.OPTIONAL_HEADER.AddressOfEntryPoint),'replacements':replacements,'dll_sha256':hashlib.sha256(dll.read_bytes()).hexdigest(),'source_reconstructed_functions':len(replacements),'validation':validation,'remaining_engine':'Original captured machine code; not a standalone source rebuild','parser_warnings':check.get_warnings()}
    (ROOT/'reports/hybrid-build.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:report[k] for k in ['sha256','dll_sha256','source_reconstructed_functions','validation','parser_warnings']}))
if __name__=='__main__':main()
