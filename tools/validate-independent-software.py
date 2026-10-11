"""Finish independent package checks while physical controller input is pending."""
from pathlib import Path
import hashlib,json,os,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/portable-validation'
package=json.loads((root/'build/package/package-manifest.json').read_text(encoding='utf-8'))
portable=json.loads((out/'standalone-validation.json').read_text('utf-8'))
cache=Path(portable['runtimeDirectory']).resolve();runtimeBase=cache.parent
assert not cache.is_symlink()
assert (cache/'.rhine-native-owned').read_text(encoding='utf-8').strip()==package['payloadSha256']
exe=Path(portable['singleFileDelivery'])
assert hashlib.file_digest(exe.open('rb'),'sha256').hexdigest()==package['exeSha256']
env=os.environ.copy()
for name in list(env):
 if name.upper().startswith(('RHINE_','SONY_WALKMAN_','PYTHON','NODE_')) or name.upper()=='MUSIC_DATA_DIR':env.pop(name,None)
systemRoot=env.get('SystemRoot',env.get('SYSTEMROOT'));assert systemRoot
env['PATH']=os.path.join(systemRoot,'System32')+os.pathsep+systemRoot
env['RHINE_RUNTIME_ROOT']=str(runtimeBase);env['RHINE_D3D_DEBUG']='1'
results=[]
def record(name,**fields):
 results.append({'name':name,'ok':True,**fields})
 (out/'independent-software-validation.json').write_text(json.dumps({'ok':True,'physicalControllerVerificationPending':True,'packagePayloadSha256':package['payloadSha256'],'checks':results},indent=2),encoding='utf-8')
 print(name+': PASS',flush=True)
for name,api,upscaler,view in [('dx12-dlss-rtx','dx12','dlss','detail'),('dx12-fsr-rtx','dx12','fsr','detail'),('original-badges','dx11','native','dsd-detail')]:
 report=out/(name+'.json');args=['--self-test','--demo','--fixture','audio-tech' if name=='original-badges' else 'parity','--api',api,'--upscaler',upscaler,'--reflections','rtx' if api=='dx12' else 'ssr','--width','1280','--height','720','--view',view,'--duration','9','--report',str(report),'--image',str(out/(name+'.png'))]
 r=subprocess.run([str(exe),*args],cwd=exe.parent,env=env,creationflags=0x08000000,timeout=100)
 data=json.loads(report.read_text(encoding='utf-8'));assert r.returncode==0 and data['ok'];assert data['nativeAudioAvailable'] and data['sonyAvailable'] and data['controllers']['scePadInitialized']
 if api=='dx12':assert data['graphics']['upscaler']['active'] and data['graphics']['rayTracing']['active']
 if name=='original-badges':assert {'hi-res','dsd'}<={b['name'] for b in data['ui']['audioBadges'] if not b['track']}
 record('Packaged '+name+' software',physicalDs4Connected=any(p['name']=='DualShock 4' for p in data['controllers']['connected']))
tests=out/'tests';tests.mkdir(exist_ok=True);(cache/'build').mkdir(exist_ok=True)
for name in ['backend-smoke','cd-playback','online-playback']:
 source=(root/'tests'/(name+'.mjs')).read_text(encoding='utf-8');before="path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..')";assert before in source
 file=tests/(name+'.mjs');file.write_text(source.replace(before,'JSON.parse(process.env.RHINE_TEST_RUNTIME_JSON)',1),encoding='utf-8')
 with (out/(name+'.log')).open('w',encoding='utf-8') as log:
  r=subprocess.run([str(cache/'runtime/node/node.exe'),str(file)],cwd=exe.parent,env={**env,'RHINE_TEST_RUNTIME_JSON':json.dumps(str(cache))},stdout=log,stderr=subprocess.STDOUT,creationflags=0x08000000,timeout=180)
 assert r.returncode==0,name
 data=json.loads((cache/'build'/(name+'.json')).read_text(encoding='utf-8'));assert data['ok']
 (out/(name+'.json')).write_text(json.dumps(data,indent=2),encoding='utf-8');record('Packaged '+name)
assert sorted(p.name for p in exe.parent.iterdir())==['RhineMusic.exe']
record('Single executable delivery directory remains intact')
