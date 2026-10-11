from pathlib import Path
from prepare import api,download
import argparse,json,zipfile,shutil
p=argparse.ArgumentParser();p.add_argument('--target',required=True);args=p.parse_args();root=Path(args.target).resolve();manifest=json.loads((root/'vendor/sdk-sources.json').read_text('utf-8'))
release=api('https://api.github.com/repos/microsoft/DirectXShaderCompiler/releases/latest');asset=next(a for a in release['assets'] if a['name'].startswith('dxc_') and a['name'].endswith('.zip'));path=root/'vendor/downloads'/asset['name'];record=download(asset['browser_download_url'],path)
out=root/'vendor/dxc';out.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(path) as archive:
    for name in archive.namelist():
        if name.startswith(('bin/x64/','lib/x64/','inc/')) or name.lower().startswith('license'):
            destination=out/name
            if name.endswith('/'):destination.mkdir(parents=True,exist_ok=True)
            else:destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(archive.read(name))
manifest['dxc']={'repository':'microsoft/DirectXShaderCompiler','tag':release['tag_name'],'publishedAt':release['published_at'],'archive':record}
branch=api('https://api.github.com/repos/GPUOpen-Effects/FidelityFX-FSR/commits/master');revision=branch['sha'];records={}
for name in ['ffx-fsr/ffx_a.h','ffx-fsr/ffx_fsr1.h','license.txt']:
    records[name]=download('https://raw.githubusercontent.com/GPUOpen-Effects/FidelityFX-FSR/'+revision+'/'+name,root/'vendor/fsr1'/name)
manifest['fsr1']={'repository':'GPUOpen-Effects/FidelityFX-FSR','revision':revision,'purpose':'DX11 compatibility only; DX12 uses current FSR SDK','files':records}
(root/'vendor/sdk-sources.json').write_text(json.dumps(manifest,indent=2)+'\n','utf-8')
for name in ['dxcompiler.dll','dxil.dll']:
    if (out/'bin/x64'/name).exists():shutil.copy2(out/'bin/x64'/name,root/'runtime/graphics'/name)
print(json.dumps({'dxc':release['tag_name'],'fsr11Revision':revision}))
