"""Validate insertion and actual disc renders using isolated synthetic audio."""
from pathlib import Path
import os,subprocess,json,argparse,hashlib,shutil
from PIL import Image,ImageChops

def validate_static_thumbnail(directory):
    first=json.loads((directory/'thumbnail-playing-a.json').read_text('utf-8'))
    last=json.loads((directory/'thumbnail-playing-b.json').read_text('utf-8'))
    assert first['player']['playing'] and last['player']['playing']
    assert first['cdMedia']['discs']['angles']['demo-optical']!=last['cdMedia']['discs']['angles']['demo-optical']
    rect=first['ui']['layoutNodes']['playback-cover-pixels']
    assert rect==last['ui']['layoutNodes']['playback-cover-pixels']
    x,y,w,h=rect;bounds=tuple(round(v) for v in (x+2,y+2,x+w-2,y+h-2))
    a=Image.open(directory/'thumbnail-playing-a.png').convert('RGB')
    b=Image.open(directory/'thumbnail-playing-b.png').convert('RGB')
    delta=ImageChops.difference(a.crop(bounds),b.crop(bounds))
    changed=sum(max(pixel)>2 for pixel in delta.get_flattened_data())
    assert changed==0,f'Playing CD thumbnail rotated or changed: {changed} pixels'
    return {'ok':True,'comparedPixels':(bounds[2]-bounds[0])*(bounds[3]-bounds[1]),'changedPixels':changed,'bounds':bounds,'discPhaseChanged':True,'images':{file.name:hashlib.file_digest(file.open('rb'),'sha256').hexdigest() for file in [directory/'thumbnail-playing-a.png',directory/'thumbnail-playing-b.png']}}
root=Path(__file__).resolve().parents[1];parser=argparse.ArgumentParser();parser.add_argument('--packaged',action='store_true');parser.add_argument('--quick',action='store_true');args=parser.parse_args()
out=root/'build/cd-media-validation';out.mkdir(parents=True,exist_ok=True)
package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8')) if args.packaged else None
binary=root/('Release/RhineMusic.exe' if args.packaged else 'RhineMusicD3D.exe');env=os.environ.copy()
for name in list(env):
    if name.upper().startswith(('RHINE_','SONY_WALKMAN_','PYTHON','NODE_')) or name.upper()=='MUSIC_DATA_DIR':env.pop(name,None)
env['RHINE_D3D_DEBUG']='1';system=env.get('SystemRoot',env.get('SYSTEMROOT'));env['PATH']=str(Path(system)/'System32')+os.pathsep+system
cases=[]
matrix=[('dx11','ssr',1280,720,False)] if args.quick else [(api,ref,w,h,night) for api,ref in [('dx11','off'),('dx11','ssr'),('dx12','rtx')] for w,h,night in [(1600,900,False),(488,1055,True)]]
for api,ref,w,h,night in matrix:
    name=f'{api}-{ref}-{w}x{h}-'+('night' if night else 'day');directory=out/name;directory.mkdir(parents=True,exist_ok=True);report=directory/'report.json';print('Testing optical media '+name,flush=True)
    cmd=[str(binary),'--self-test','--demo','--fixture','cd-media','--api',api,'--upscaler','native','--reflections',ref,'--width',str(w),'--height',str(h),'--scenario','cd-media','--report',str(report)]
    if night:cmd.append('--night')
    result=subprocess.run(cmd,cwd=root,env=env,creationflags=0x08000000,timeout=175);data=json.loads(report.read_text('utf-8'));assert result.returncode==0 and data.get('ok'),data.get('error',name)
    assert not data.get('graphicsRecoveries') and data['graphics']['debugErrors']==0
    if ref=='rtx':assert data['graphics']['rayTracing']['active'],data['graphics']['rayTracing']
    cache=root
    if package:cache=Path(os.environ.get('RHINE_RUNTIME_ROOT',str(Path(os.environ['LOCALAPPDATA'])/'RhineMusicD3D/runtimes')))/package['payloadSha256'][:32]
    for file in (cache/'build/cd-media').glob('*'):shutil.copy2(file,directory/file.name)
    thumbnail=validate_static_thumbnail(directory)
    cases.append({'case':name,'ok':True,'checks':len(data['checks']),'staticThumbnail':thumbnail,'debugErrors':data['graphics']['debugErrors'],'graphicsRecoveries':data['graphicsRecoveries'],'rayTracing':data['graphics'].get('rayTracing'),'performance':data['graphics'].get('performance'),'report':str(report.relative_to(root))})
validation={'ok':True,'syntheticSourcesOnly':True,'physicalInsertOrEject':False,'packagedRuntime':args.packaged,'cases':cases,'nativeExeSha256':hashlib.file_digest((root/'RhineMusicD3D.exe').open('rb'),'sha256').hexdigest()}
if package:validation['packagePayloadSha256']=package['payloadSha256'];assert validation['nativeExeSha256']==package['files']['RhineMusicD3D.exe']
(out/'validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2),'utf-8');print('Optical media regression passed: '+str(sum(case['checks'] for case in cases))+' checks.',flush=True)
