"""Validate the single executable using only its embedded program runtime."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,time,sys,uuid,argparse
sys.stdout.reconfigure(encoding='utf-8')
parser=argparse.ArgumentParser();parser.add_argument('--software-only',action='store_true');options=parser.parse_args()
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
out=root/'build/portable-validation';out.mkdir(parents=True,exist_ok=True)
delivery=out/'\u4e2d\u6587\u8def\u5f84 \u70b9\u5f00\u5373\u7528';delivery.mkdir(exist_ok=True)
exe=delivery/'RhineMusic.exe';shutil.copy2(root/'Release/RhineMusic.exe',exe)
assert sorted(file.name for file in delivery.iterdir())==['RhineMusic.exe']
runtimeBase=out/'\u5185\u7f6e\u8fd0\u884c\u65f6'/('run-'+uuid.uuid4().hex[:12]);runtimeBase.mkdir(parents=True)
env=os.environ.copy()
for name in list(env):
    if name.upper().startswith(('RHINE_','SONY_WALKMAN_','PYTHON','NODE_')) or name.upper()=='MUSIC_DATA_DIR':env.pop(name,None)
systemRoot=env.get('SystemRoot',env.get('SYSTEMROOT'))
assert systemRoot and Path(systemRoot).is_dir()
env['PATH']=os.path.join(systemRoot,'System32')+os.pathsep+systemRoot
env['RHINE_RUNTIME_ROOT']=str(runtimeBase);env['RHINE_D3D_DEBUG']='1'
checks=[]
def record(name,**fields):
    checks.append({'name':name,'ok':True,**fields});print(name+': PASS',flush=True)
    (out/'standalone-validation.json').write_text(json.dumps({'ok':True,'singleFileDelivery':str(exe),'runtimeDirectory':str(cache),'developerPathRemoved':True,'cleanOsVm':False,'physicalControllerRequired':not options.software_only,'packagePayloadSha256':package['payloadSha256'],'exeSha256':package['exeSha256'],'checks':checks},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def launch(args,timeout=180):
    started=time.monotonic();result=subprocess.run([str(exe),*args],cwd=delivery,env=env,creationflags=0x08000000,timeout=timeout)
    if result.returncode:raise RuntimeError('Standalone exit '+str(result.returncode)+'; '+str(args))
    return time.monotonic()-started
try:
    duration=launch(['--self-test','--extract-only'])
    package=json.loads((root/'build/package/package-manifest.json').read_text(encoding='utf-8'))
    cache=(runtimeBase/package['payloadSha256'][:32]).resolve();assert cache.is_dir() and cache.parent==runtimeBase.resolve() and not cache.is_symlink()
    assert (cache/'.rhine-native-owned').is_file()
    assert (cache/'.rhine-native-owned').read_text(encoding='utf-8').strip()==package['payloadSha256']
    record('Unicode path cold extraction with a single EXE',seconds=duration,fileCount=package['fileCount'])
    for name,digest in package['files'].items():
        target=cache/name;assert target.is_file() and not target.is_symlink()
        assert hashlib.file_digest(target.open('rb'),'sha256').hexdigest()==digest,name
    record('Every extracted program file matches the package SHA-256 inventory')
    shaders=sorted((cache/'shaders/compiled').glob('*.cso'));assert shaders
    shader=shaders[0];original=shader.read_bytes();shader.write_bytes(original+b'checksum-verification-test')
    duration=launch(['--self-test','--extract-only']);assert shader.read_bytes()==original
    record('Owned cache corruption is detected and atomically repaired',seconds=duration)
    manifest=cache/'runtime-files.sha256';originalManifest=manifest.read_bytes();manifest.write_bytes(originalManifest+b'\n')
    duration=launch(['--self-test','--extract-only']);assert manifest.read_bytes()==originalManifest
    record('A modified checksum manifest cannot bypass embedded verification',seconds=duration)
    for name,api,upscaler,scenario,view in [('dx11','dx11','native','stress','browse'),('dx12-dlss-rtx','dx12','dlss','','detail'),('dx12-fsr-rtx','dx12','fsr','','detail'),('original-badges','dx11','native','','dsd-detail')]:
        report=out/(name+'.json');image=out/(name+'.png')
        args=['--self-test','--demo','--fixture','audio-tech' if name=='original-badges' else 'parity','--api',api,'--upscaler',upscaler,'--reflections','rtx' if api=='dx12' else 'ssr','--width','1280','--height','720','--duration','9','--view',view,'--report',str(report),'--image',str(image)]
        if scenario:args+=['--scenario',scenario]
        duration=launch(args);data=json.loads(report.read_text(encoding='utf-8'));assert data['ok'] is True
        assert data['nativeAudioAvailable'] and data['sonyAvailable'];assert data['graphics']['debugErrors']==0 and data['graphics']['debugDiscarded']==0
        assert data['controllers']['scePadInitialized'],'The packaged Pad SDK did not initialize'
        if not options.software_only:
            assert any(p['name']=='DualShock 4' for p in data['controllers']['connected']),'No physical DualShock 4 connection was reported; check that the controller is awake and connected'
        if name=='original-badges':assert {'hi-res','dsd'}<={badge['name'] for badge in data['ui']['audioBadges'] if not badge['track']},'Packaged original badges missing'
        if api=='dx12':assert data['graphics']['upscaler']['active'] and data['graphics']['rayTracing']['active']
        record('Packaged '+name+' render, original Sony and Pad SDK initialization',seconds=duration,checks=len(data['checks']),physicalDs4Connected=any(p['name']=='DualShock 4' for p in data['controllers']['connected']),report=str(report.relative_to(root)))
    # Test code remains outside the embedded runtime and addresses it explicitly.
    tests=out/'tests';tests.mkdir(exist_ok=True)
    for name in ['backend-smoke','cd-playback','online-playback']:
        source=(root/'tests'/(name+'.mjs')).read_text(encoding='utf-8')
        before="path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..')"
        assert before in source;source=source.replace(before,'JSON.parse(process.env.RHINE_TEST_RUNTIME_JSON)',1)
        file=tests/(name+'.mjs');file.write_text(source,encoding='utf-8')
        (cache/'build').mkdir(exist_ok=True);testEnv={**env,'RHINE_TEST_RUNTIME_JSON':json.dumps(str(cache))}
        started=time.monotonic()
        with (out/(name+'.log')).open('w',encoding='utf-8') as log:
            result=subprocess.run([str(cache/'runtime/node/node.exe'),str(file)],cwd=delivery,env=testEnv,creationflags=0x08000000,stdout=log,stderr=subprocess.STDOUT,timeout=180)
        if result.returncode:raise RuntimeError('Embedded runtime test failed: '+name)
        data=json.loads((cache/'build'/(name+'.json')).read_text(encoding='utf-8'));assert data['ok']
        (out/(name+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        record('Embedded runtime '+name,seconds=time.monotonic()-started,report=str((out/(name+'.json')).relative_to(root)))
    # Confirm the delivery directory stayed portable after all launches.
    assert sorted(file.name for file in delivery.iterdir())==['RhineMusic.exe']
    record('Delivery folder still contains exactly one executable')
    print('Standalone validation passed.',flush=True)
except Exception as error:
    (out/'standalone-validation.json').write_text(json.dumps({'ok':False,'error':str(error),'runtimeDirectory':str(cache) if 'cache' in locals() else None,'singleFileDelivery':str(exe),'packagePayloadSha256':package['payloadSha256'] if 'package' in locals() else None,'checks':checks},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    raise
