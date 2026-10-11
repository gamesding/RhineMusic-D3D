"""Collect physical Pad SDK actions from an isolated visible native demo."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys,time
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--cd',action='store_true');parser.add_argument('--packaged',action='store_true');options=parser.parse_args()
out=root/('build/physical-cd' if options.cd else 'build/physical-controller');out.mkdir(parents=True,exist_ok=True)
live=out/'live.json';report=out/'physical-validation.json'
env=os.environ.copy();env['RHINE_D3D_DEBUG']='1'
for name in ['RHINE_APP_HOME','MUSIC_DATA_DIR','RHINE_DEMO','RHINE_DEMO_FIXTURE']:env.pop(name,None)
binary=root/('Release/RhineMusic.exe' if options.packaged else 'RhineMusicD3D.exe');binary_hash=hashlib.file_digest(binary.open('rb'),'sha256').hexdigest()
args=[str(binary),'--interactive-test','--api','dx11','--upscaler','native','--view','browse','--width','1280','--height','720','--duration','600','--report',str(live)]
if not options.cd:args+=['--demo','--fixture','parity']
child=subprocess.Popen(args,cwd=root,env=env,creationflags=0x08000000)
actions={};last={};changed=0
def collect():
    global last,changed
    try:
        current=json.loads(live.read_text('utf-8'))
        for action in current.get('controllerActions',[]):
            if action.get('physical'):actions[action['sequence']]=action
        last=current
    except (OSError,ValueError):return
    data={'isolatedDemo':not options.cd,'isolatedAppHome':True,'scriptedInputs':False,'packagedRuntime':options.packaged,'launchedExeSha256':binary_hash,'actions':[actions[key] for key in sorted(actions)],'physicalActionCount':len(actions),'controllers':last.get('controllers',{}),'lastPlayer':last.get('player',{}),'selectedAlbum':last.get('selectedAlbum',{}),'albumCount':last.get('albumCount'),'cdEject':last.get('cdEject',{}),'lastUi':{'focus':last.get('ui',{}).get('focus'),'trackFocus':last.get('ui',{}).get('trackFocus')},'elapsedSeconds':last.get('elapsedSeconds',0),'finished':child.poll() is not None,'exitCode':child.poll()}
    report.write_text(json.dumps(data,ensure_ascii=False,indent=2),'utf-8')
    if len(actions)!=changed:
        print('Physical controller actions recorded: '+str(len(actions)),flush=True);changed=len(actions)
print('Visible isolated controller window started; report '+str(report),flush=True)
while child.poll() is None:collect();time.sleep(.15)
collect();print('Physical input collection finished: '+str(len(actions))+' actions, native exit '+str(child.returncode),flush=True)
raise SystemExit(child.returncode)
