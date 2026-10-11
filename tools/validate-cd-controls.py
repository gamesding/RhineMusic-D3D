from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/cd-controls';out.mkdir(parents=True,exist_ok=True)
parser=argparse.ArgumentParser();parser.add_argument('--packaged',action='store_true');options=parser.parse_args()
env=os.environ.copy();env['RHINE_D3D_DEBUG']='1'
for name in ['RHINE_APP_HOME','MUSIC_DATA_DIR','RHINE_DEMO','RHINE_DEMO_FIXTURE']:env.pop(name,None)
binary=root/('Release/RhineMusic.exe' if options.packaged else 'RhineMusicD3D.exe');report=out/'ui.json'
result=subprocess.run([str(binary),'--self-test','--demo','--fixture','cd-ui','--scenario','cd-controls','--api','dx11','--upscaler','native','--width','1280','--height','720','--report',str(report)],cwd=root,env=env,creationflags=0x08000000,timeout=150)
data=json.loads(report.read_text('utf-8'))
if result.returncode or not data['ok']:raise RuntimeError(data.get('error','Native CD/footer test failed; see '+str(report)))
validation={'ok':True,'packagedRuntime':options.packaged,'nativeExeSha256':hashlib.file_digest((root/'RhineMusicD3D.exe').open('rb'),'sha256').hexdigest(),'checks':data['checks'],'report':'build/cd-controls/ui.json','actualEject':False,'isolatedSyntheticFixture':True}
if options.packaged:
    package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'));validation['packagePayloadSha256']=package['payloadSha256']
    assert validation['nativeExeSha256']==package['files']['RhineMusicD3D.exe']
    base=Path(os.environ.get('RHINE_RUNTIME_ROOT',str(Path(os.environ['LOCALAPPDATA'])/'RhineMusicD3D/runtimes')))
    import shutil
    for image in (base/package['payloadSha256'][:32]/'build/cd-controls').glob('*.png'):shutil.copy2(image,out/image.name)
(root/'build/cd-controls-validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2),'utf-8')
print('CD/footer native regression passed: '+str(len(data['checks']))+' checks; physical ejection recorded separately.',flush=True)
