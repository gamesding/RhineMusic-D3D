from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--packaged',action='store_true');args=parser.parse_args()
env=os.environ.copy();probe=root/'RhineDeviceProbe.exe';report=root/'build/haptic-validation.json'
if args.packaged:
    package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'))
    portable=json.loads((root/'build/portable-validation/standalone-validation.json').read_text('utf-8'))
    cache=Path(portable['runtimeDirectory']);probe=cache/'RhineDeviceProbe.exe'
    assert (cache/'.rhine-native-owned').read_text('utf-8').strip()==package['payloadSha256']
    assert hashlib.file_digest(probe.open('rb'),'sha256').hexdigest()==package['files']['RhineDeviceProbe.exe']
    env['PATH']=os.path.join(os.environ['SystemRoot'],'System32')+os.pathsep+os.environ['SystemRoot']
result=subprocess.run([str(probe),'--haptic-test','--json',str(report)],cwd=root,env=env,creationflags=0x08000000,stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding='utf-8',errors='replace',timeout=20)
assert result.returncode==0,(result.stdout,result.stderr)
data=json.loads(report.read_text('utf-8'));assert data['ok'] and len(data['checks'])==8
data['packagedRuntime']=args.packaged;data['probeSha256']=hashlib.file_digest(probe.open('rb'),'sha256').hexdigest()
if args.packaged:data['packagePayloadSha256']=package['payloadSha256']
report.write_text(json.dumps(data,ensure_ascii=False,indent=2),'utf-8')
print('Actual PS4 feedback passed: 8 checks, normal '+str(round(data['normalElapsedMs'],2))+' ms, cancellation '+str(round(data['cancelElapsedMs'],2))+' ms; no rendering or input polling during either pulse.')
