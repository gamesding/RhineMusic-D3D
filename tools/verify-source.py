"""Verify immutable dependencies and the binary-only controller boundary."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[1]
stamp=json.loads((ROOT/'vendor/distribution-files.json').read_text('utf-8'))
failed=[]
for name,digest in stamp['files'].items():
    file=ROOT/name
    if not file.is_file():failed.append(name+' (missing)');continue
    with file.open('rb') as stream:
        if hashlib.file_digest(stream,'sha256').hexdigest()!=digest:failed.append(name+' (changed)')
for base in [ROOT/'native',ROOT/'vendor/scepad']:
    for file in base.rglob('*'):
        if not file.is_file() or file.suffix.lower() not in {'.h','.hpp','.cpp','.c','.inc','.lib'}:continue
        if base.name=='scepad':failed.append(str(file.relative_to(ROOT))+' (private SDK material)')
        elif file.suffix.lower()!='.lib' and any(x in file.read_bytes() for x in [b'SIE CONFIDENTIAL',b'ScePadData',b'SCE_PAD_BUTTON_',b'#include <pad.h>']):failed.append(str(file.relative_to(ROOT))+' (private SDK declaration)')
if failed:raise RuntimeError('Source distribution verification failed:\n'+'\n'.join(failed[:40]))
print(json.dumps({'ok':True,'immutableFilesVerified':len(stamp['files']),'controllerBridge':'runtime/drivers/rhine-pad-bridge.dll','controllerSdkSourceIncluded':False}))
