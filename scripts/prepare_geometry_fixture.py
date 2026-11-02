"""Generate the private geometry oracle from the exact local analysis image."""
from pathlib import Path
import hashlib
import pefile
ROOT=Path(__file__).resolve().parents[1]
def main():
    blob=(ROOT/'sources/WalkingDead101.analysis.exe').read_bytes()
    assert hashlib.sha256(blob).hexdigest()=='39df65b5e03e68703e04da90d516fbc15392706fbc9a4580f2aaf44ef78c33d9'
    code=pefile.PE(data=blob).get_data(0xcbfc0,331)
    assert hashlib.sha256(code).hexdigest()=='13a5d8f965bc269dcaefc3d32228ec147f70a9c0abbc250d99e4e77f2761870d'
    lines=['// Local captured-machine-code fixture. Do not publish.','#pragma once','inline constexpr unsigned char original_geometry[]={']
    lines.extend(','.join(f'0x{x:02x}' for x in code[i:i+16])+',' for i in range(0,len(code),16))
    lines.append('};')
    (ROOT/'tests/original_geometry_bytes.hpp').write_text('\n'.join(lines)+'\n')
    print('Generated local geometry oracle; 331 hash-checked bytes.')
if __name__=='__main__':main()
