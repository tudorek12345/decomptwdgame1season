"""Build and round-trip verify one isolated runtime test archive."""
from pathlib import Path
import argparse,hashlib,json,re,shutil,struct,subprocess
ROOT=Path(__file__).resolve().parents[1]
def run(args):
    p=subprocess.run([str(x) for x in args],capture_output=True)
    if p.returncode:raise RuntimeError((p.stdout+p.stderr).decode('utf-8','replace'))
    return p
def pack(entries):
    table=bytearray(struct.pack('<II',1,1)+b'.'+struct.pack('<I',len(entries)));payload=bytearray()
    for name,data in sorted(entries.items()):
        assert '/' not in name and '\\' not in name and name.endswith('.lenc')
        encoded=name.encode();table+=struct.pack('<I',len(encoded))+encoded+struct.pack('<III',0,len(payload),len(data));payload+=data
    return struct.pack('<7I',3,0,2,1,0,len(payload),len(table))+table+payload
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log',default='reports/runtime-scene-probe.log')
    parser.add_argument('--save-load',action='store_true',help='Also request an isolated save/reload sentinel experiment')
    options=parser.parse_args();log=(ROOT/options.log).resolve()
    if not log.is_relative_to(ROOT/'reports'):raise ValueError('Probe logs belong under project reports')
    records=json.loads((ROOT/'reports/script-provenance.json').read_text())['scripts']
    matches=[r for r in records if r['name']=='_boot.lua' and Path(r['archive']).parent.name=='default' and not r['mod_candidate']]
    chosen=matches[-1];original=ROOT/'sources/lua'/(chosen['bytecode_sha256']+'.lua')
    stage=ROOT/'build/runtime-probe-stage';stage.mkdir(parents=True,exist_ok=True)
    shutil.copy2(original,stage/'recovery_original_boot.lua')
    systems=[r for r in records if r['name']=='_system.lua' and Path(r['archive']).parent.name=='default' and not r['mod_candidate']]
    system=systems[-1]
    shutil.copy2(ROOT/'sources/lua'/(system['bytecode_sha256']+'.lua'),stage/'recovery_original_system.lua')
    system_source=ROOT/'build/runtime_system_generated.lua'
    system_source.write_text('require("recovery_original_system.lua")\nrequire("recovery_scene_monitor.lua")\n')
    run([ROOT/'tools/telltale-lua/luac.exe','-o',stage/'_system.lua',system_source])
    monitor_source=ROOT/'build/runtime_monitor_generated.lua'
    monitor=(ROOT/'tests/runtime_scene_monitor.lua').read_text()
    phase=log.with_suffix('.phase')
    if options.save_load:
        if phase.exists():raise ValueError('Use a new --log filename for each save/reload experiment; existing phase is preserved')
        monitor+='\n'+(ROOT/'tests/runtime_save_load_monitor.lua').read_text()
    monitor_source.write_text(monitor.replace('@REPORT_PATH@',log.as_posix()).replace('@PHASE_PATH@',phase.as_posix()))
    run([ROOT/'tools/telltale-lua/luac.exe','-o',stage/'recovery_scene_monitor.lua',monitor_source])
    generated=ROOT/'build/runtime_probe_generated.lua'
    generated.write_text((ROOT/'tests/runtime_probe.lua').read_text().replace('@REPORT_PATH@',log.as_posix()))
    run([ROOT/'tools/telltale-lua/luac.exe','-o',stage/'_boot.lua',generated])
    archive=ROOT/'tools/ttarchext/ttarchext.exe';intermediate=ROOT/'build/runtime-probe-v8.ttarch'
    run([archive,'-o','-b','-x','-V','8','52',intermediate,stage])
    b=intermediate.read_bytes();assert struct.unpack_from('<5I',b)==(8,1,2,1,0)
    listing=run([archive,'-l','52',intermediate,ROOT]).stdout.decode('utf-8','replace')
    entries=re.findall(r'^\s+([0-9a-fA-F]{8})\s+(\d+)\s+(.+)$',listing,re.M);assert len(entries)==5
    dest=ROOT/'runtime/Pack/default/0_000000_recovery_probe.ttarch'
    dest.write_bytes(pack({n.strip():b[int(o,16):int(o,16)+int(s)] for o,s,n in entries}))
    verify=ROOT/'build/runtime-probe-verify';verify.mkdir(exist_ok=True)
    run([archive,'-o','52',dest,verify])
    for f in stage.glob('*.lua'):assert f.read_bytes()==(verify/f.name).read_bytes()
    (ROOT/'reports/runtime-probe-build.json').write_text(json.dumps({'original_boot':chosen,'archive':str(dest),'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'round_trip_verified':True,'enabled':True,'log':str(log),'save_load_experiment':options.save_load,'auto_start':'Only if IsRegistered() is true; isolated slot 1'},indent=2))
    print('Runtime probe built and round-trip verified')
if __name__=='__main__':main()
