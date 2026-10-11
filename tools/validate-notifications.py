from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys,shutil
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/notifications';out.mkdir(parents=True,exist_ok=True)
parser=argparse.ArgumentParser();parser.add_argument('--packaged',action='store_true');args=parser.parse_args()
env=os.environ.copy();env['RHINE_D3D_DEBUG']='1'
for key in ['RHINE_APP_HOME','MUSIC_DATA_DIR','RHINE_DEMO','RHINE_DEMO_FIXTURE']:env.pop(key,None)
report=out/'ui.json';binary=root/('RhineMusic.exe' if args.packaged else 'RhineMusicD3D.exe')
result=subprocess.run([str(binary),'--self-test','--demo','--fixture','parity','--scenario','notifications','--api','dx11','--upscaler','native','--width','1920','--height','974','--report',str(report)],cwd=root,env=env,creationflags=0x08000000,timeout=150)
data=json.loads(report.read_text('utf-8'))
assert result.returncode==0 and data['ok'],data.get('error','Notification regression failed')
empty_report=out/'empty.json'
empty_result=subprocess.run([str(binary),'--self-test','--demo','--fixture','empty','--scenario','notification-empty','--api','dx11','--upscaler','native','--width','1920','--height','974','--report',str(empty_report)],cwd=root,env=env,creationflags=0x08000000,timeout=60)
empty=json.loads(empty_report.read_text('utf-8'));assert empty_result.returncode==0 and empty['ok'],empty.get('error','Empty shelf notification test failed')
data['checks'].extend(empty['checks'])
validation={'ok':True,'packagedRuntime':args.packaged,'nativeExeSha256':hashlib.file_digest((root/'RhineMusicD3D.exe').open('rb'),'sha256').hexdigest(),'checks':data['checks'],'report':'build/notifications/ui.json','emptyShelfReport':'build/notifications/empty.json','sizes':[[1920,974],[1920,1080],[1280,720],[2100,900],[900,1600],[800,600],[480,800],[360,320]],'themes':['day','night']}
if args.packaged:
    package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'));validation['packagePayloadSha256']=package['payloadSha256']
    assert validation['nativeExeSha256']==package['files']['RhineMusicD3D.exe']
    base=Path(os.environ.get('RHINE_RUNTIME_ROOT',str(Path(os.environ['LOCALAPPDATA'])/'RhineMusicD3D/runtimes')))
    for file in (base/package['payloadSha256'][:32]/'build/notifications').glob('*.png'):shutil.copy2(file,out/file.name)
(root/'build/notification-validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2),'utf-8')
print('Responsive notification regression passed: '+str(len(data['checks']))+' checks.')
