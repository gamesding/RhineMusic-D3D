"""Exercise the app's actual controller actions and touchpad transport route."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/controller-tracks';out.mkdir(parents=True,exist_ok=True)
manifest_path=root/'build/package/package-manifest.json'
packaged='--packaged' in sys.argv
binary=root/'RhineMusicD3D.exe';env=os.environ.copy();env['RHINE_D3D_DEBUG']='1'
env.pop('RHINE_DEMO',None);env.pop('RHINE_DEMO_FIXTURE',None)
if packaged:
    package=json.loads(manifest_path.read_text('utf-8'))
    cache=Path(os.environ['LOCALAPPDATA'])/'RhineMusicD3D/runtimes'/package['payloadSha256'][:32]
    binary=root/'RhineMusic.exe';assert hashlib.file_digest(binary.open('rb'),'sha256').hexdigest()==package['exeSha256']
    env['RHINE_RUNTIME_ROOT']=str(cache.parent)
    system=env.get('SystemRoot',env.get('SYSTEMROOT'));env['PATH']=str(Path(system)/'System32')+os.pathsep+system
results=[]
for width,height in [(1600,900),(1000,750),(488,1055)]:
    name=f'{width}x{height}';print('Testing controller selection / touchpad '+name,flush=True);report=out/(name+'.json')
    args=[str(binary),'--self-test','--demo','--fixture','parity','--api','dx11','--upscaler','native','--view','browse','--width',str(width),'--height',str(height),'--scenario','controller-tracks','--report',str(report),'--image',str(out/(name+'.png'))]
    child=subprocess.run(args,cwd=root,env=env,creationflags=0x08000000,timeout=160)
    data=json.loads(report.read_text('utf-8'));assert child.returncode==0 and data.get('ok'),data.get('error','controller UI failed')
    if packaged:
        assert (cache/'.rhine-native-owned').read_text('utf-8').strip()==package['payloadSha256']
        assert hashlib.file_digest((cache/'RhineMusicD3D.exe').open('rb'),'sha256').hexdigest()==package['files']['RhineMusicD3D.exe']
        for capture in (cache/'build/controller-tracks').glob('*-'+name+'.png'):
            shutil.copy2(capture,out/capture.name)
    results.append({'name':name,'ok':True,'checks':len(data['checks']),'physicalDs4Connected':any(p['name']=='DualShock 4' for p in data['controllers']['connected']),'report':str(report.relative_to(root))})
    result={'ok':True,'cases':results,'packagedRuntime':packaged,'nativeExeSha256':hashlib.file_digest((root/'RhineMusicD3D.exe').open('rb'),'sha256').hexdigest()}
    if packaged:result['packagePayloadSha256']=package['payloadSha256']
    (root/'build/controller-validation.json').write_text(json.dumps(result,indent=2),'utf-8')
print('Controller scenarios passed:',len(results))
