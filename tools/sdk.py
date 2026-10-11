from pathlib import Path
from prepare import api, download
import argparse,json

parser=argparse.ArgumentParser();parser.add_argument('--target',required=True);parser.add_argument('--fetch',action='store_true');args=parser.parse_args()
root=Path(args.target).resolve();manifest=json.loads((root/'vendor/sdk-sources.json').read_text('utf-8'))
for repository,name in [('NVIDIA/DLSS','dlss'),('GPUOpen-LibrariesAndSDKs/FidelityFX-SDK','fsr')]:
    tag=manifest[name]['tag']
    tree=api('https://api.github.com/repos/'+repository+'/git/trees/'+tag+'?recursive=1')
    if tree.get('truncated'):raise RuntimeError('Truncated SDK tree')
    selected=[]
    for item in tree['tree']:
        path=item['path'];low=path.lower()
        if item['type']!='blob':continue
        if name=='dlss':
            take=(low.startswith('include/') or low=='lib/windows_x86_64/x64/nvsdk_ngx_s.lib' or low=='lib/windows_x86_64/rel/nvngx_dlss.dll' or low in ['license.txt','license','readme.md'])
        else:
            take=(low.startswith('kits/fidelityfx/api/include/') or low.startswith('kits/fidelityfx/upscalers/include/') or low=='kits/fidelityfx/signedbin/amd_fidelityfx_upscaler_dx12.dll' or ('license' in low and '/' not in low) or low=='readme.md')
        if take:selected.append(item)
    print(json.dumps({'name':name,'tag':tag,'files':[{'path':i['path'],'size':i.get('size')} for i in selected]}),flush=True)
    if args.fetch:
        records={}
        for item in selected:
            url='https://raw.githubusercontent.com/'+repository+'/'+tag+'/'+item['path']
            destination=root/'vendor'/name/item['path']
            existing=manifest[name].get('files',{}).get(item['path'])
            if existing and destination.is_file():records[item['path']]=existing
            else:records[item['path']]=download(url,destination)
        manifest[name]['files']=records
        (root/'vendor/sdk-sources.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
