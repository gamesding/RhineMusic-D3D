from pathlib import Path
import argparse, shutil, hashlib, json
p=argparse.ArgumentParser();p.add_argument('--target',required=True);args=p.parse_args();root=Path(args.target).resolve()
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
files={'nvngx_dlss.dll':'vendor/dlss/lib/Windows_x86_64/rel/nvngx_dlss.dll','amd_fidelityfx_upscaler_dx12.dll':'vendor/fsr/Kits/FidelityFX/signedbin/amd_fidelityfx_upscaler_dx12.dll'}
out=root/'runtime/graphics';out.mkdir(parents=True,exist_ok=True);manifest={}
for name,source in files.items():
    shutil.copy2(root/source,out/name);manifest[name]={'sha256':hashlib.sha256((out/name).read_bytes()).hexdigest(),'source':source}
(out/'assets.json').write_text(json.dumps(manifest,indent=2)+'\n','utf-8')
print('Staged official x64 graphics runtimes with fixed hashes.')
