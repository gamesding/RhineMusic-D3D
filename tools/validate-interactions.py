"""Exercise actual rendered UI frames, pointer dragging and native EDIT text."""
from pathlib import Path
import argparse,json,subprocess,os,hashlib,shutil,math
from PIL import Image,ImageChops

def validate_caret_pixels(directory):
    checks=[]
    for metaFile in sorted(directory.glob('*-caret-on.json')):
        key=metaFile.name.removesuffix('-caret-on.json')
        on=json.loads(metaFile.read_text('utf-8'));off=json.loads((directory/(key+'-caret-off.json')).read_text('utf-8'))
        assert on['native'] and off['native'] and on['drawn'] and not off['drawn'],key
        assert on['bounds']==off['bounds'],key
        bounds=tuple((math.floor if i<2 else math.ceil)(v*on['scale']) for i,v in enumerate(on['bounds']))
        lit=Image.open(directory/(key+'-caret-on.png')).convert('RGB')
        dark=Image.open(directory/(key+'-caret-off.png')).convert('RGB')
        assert bounds[0]>=0 and bounds[1]>=0 and bounds[2]<=lit.width and bounds[3]<=lit.height,(key,bounds,lit.size)
        delta=ImageChops.difference(lit.crop(bounds),dark.crop(bounds))
        changed=sum(max(pixel)>24 for pixel in delta.get_flattened_data())
        assert changed>=max(4,(bounds[3]-bounds[1])//2),f'{key}: caret state changed but its rendered pixels did not ({changed})'
        rect=[int(round(v*on['scale'])) for v in on['editorBounds']]
        sample=dark.getpixel((rect[2]-4,rect[1]+4))
        text=dark.crop((rect[0],rect[1],min(rect[2],bounds[2]+4),min(rect[3],rect[1]+32)))
        ink=sum(max(abs(pixel[i]-sample[i]) for i in range(3))>48 for pixel in text.get_flattened_data())
        assert ink>=4,f'{key}: entered text disappeared from the rendered editor'
        if key.endswith('password'):
            assert on['masked'] and off['masked'] and on['textLength']==16,key
            assert ink>=32,f'{key}: masked text disappeared from the rendered editor'
            blurred=Image.open(directory/(key+'-blurred.png')).convert('RGB')
            sampleBounds=(rect[0],rect[1],min(rect[2],rect[0]+180),rect[3])
            assert ImageChops.difference(dark.crop(sampleBounds),blurred.crop(sampleBounds)).getbbox() is None,f'{key}: masked text shifted or changed when losing focus'
        checks.append({'field':key,'caretChangedPixels':changed,'passwordMask':on['masked']})
    assert len(checks)==15,checks
    return checks

parser=argparse.ArgumentParser();parser.add_argument('--packaged',action='store_true');parser.add_argument('--quick',action='store_true');args=parser.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'build/interaction-validation';out.mkdir(parents=True,exist_ok=True)
package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8')) if args.packaged else None
binary=root/('RhineMusic.exe' if args.packaged else 'RhineMusicD3D.exe');env=os.environ.copy();env['RHINE_D3D_DEBUG']='1'
if args.packaged:
    for name in list(env):
        if name.upper().startswith(('RHINE_','SONY_WALKMAN_','PYTHON','NODE_')) or name.upper()=='MUSIC_DATA_DIR':env.pop(name,None)
    env['RHINE_D3D_DEBUG']='1';system=env.get('SystemRoot',env.get('SYSTEMROOT'));env['PATH']=str(Path(system)/'System32')+os.pathsep+system
cases=[]
viewports=[(1600,900)] if args.quick else [(1920,974),(1280,720),(800,600),(488,1055)]
for width,height in viewports:
    for night in ([False] if args.quick else [False,True]):
        for scenario in ['interaction-fixes','search-input-fixes']:
            name=f'{scenario}-{width}x{height}-'+('night' if night else 'day');report=out/(name+'.json');print('Testing '+name,flush=True)
            cmd=[str(binary),'--self-test','--demo','--fixture','search-ui' if scenario=='search-input-fixes' else 'parity','--api','dx11','--upscaler','native','--view','browse','--width',str(width),'--height',str(height),'--scenario',scenario,'--report',str(report),'--image',str(out/(name+'.png'))]
            if night:cmd.append('--night')
            result=subprocess.run(cmd,cwd=root,env=env,creationflags=0x08000000,timeout=175)
            data=json.loads(report.read_text('utf-8'));assert result.returncode==0 and data.get('ok'),data.get('error',name)
            captureRoot=(Path(os.environ['LOCALAPPDATA'])/'RhineMusicD3D/runtimes'/package['payloadSha256'][:32] if args.packaged else root)/'build'/scenario
            captureOut=out/name;captureOut.mkdir(exist_ok=True)
            for file in captureRoot.iterdir():
                if file.suffix in {'.png','.json'}:shutil.copy2(file,captureOut/file.name)
            pixels=validate_caret_pixels(captureOut) if scenario=='search-input-fixes' else []
            cases.append({'name':name,'ok':True,'checks':len(data['checks']),'caretPixels':pixels,'report':str(report.relative_to(root))})
            record={'ok':True,'packagedRuntime':args.packaged,'cases':cases,'nativeExeSha256':hashlib.file_digest((root/'RhineMusicD3D.exe').open('rb'),'sha256').hexdigest()}
            if package:record['packagePayloadSha256']=package['payloadSha256']
            (out/'validation.json').write_text(json.dumps(record,indent=2)+'\n','utf-8')
print('Rendered interaction and Chinese search scenarios passed:',len(cases),flush=True)
