"""Capture real native UI documentation with synthetic, isolated fixtures."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1]
media=root/'docs/media';media.mkdir(parents=True,exist_ok=True)
reports=root/'build/readme-captures';reports.mkdir(parents=True,exist_ok=True)
package=json.loads((root/'build/package/package-manifest.json').read_text('utf-8'))
env=os.environ.copy()
for key in list(env):
    if key.upper().startswith(('RHINE_','SONY_WALKMAN_','PYTHON','NODE_')) or key.upper()=='MUSIC_DATA_DIR':env.pop(key,None)
system=env.get('SystemRoot',env.get('SYSTEMROOT'))
env['PATH']=str(Path(system)/'System32')+os.pathsep+system;env['RHINE_D3D_DEBUG']='1'
captures=[]
for name,fixture,view,night,api,upscaler in [
    ('browse-day','parity','browse',False,'dx11','native'),
    ('browse-night','parity','browse',True,'dx12','dlss'),
    ('album-hires-night','audio-tech','detail',True,'dx11','native'),
    ('dsd-night','audio-tech','dsd-detail',True,'dx11','native'),
    ('search-day','search-ui','search',False,'dx11','native'),
    ('library-day','parity','library',False,'dx11','native'),
    ('settings-day','parity','settings',False,'dx11','native'),
    ('controller-help','parity','controller-help',True,'dx11','native'),
]:
    print('Capturing '+name,flush=True);report=reports/(name+'.json');image=media/(name+'.png')
    command=[str(root/'RhineMusic.exe'),'--self-test','--demo','--fixture',fixture,'--view',view,'--api',api,'--upscaler',upscaler,'--reflections','ssr','--width','1600','--height','900','--quality','high','--duration','8','--report',str(report),'--image',str(image)]
    if night:command.append('--night')
    result=subprocess.run(command,cwd=root,env=env,creationflags=0x08000000,timeout=85)
    data=json.loads(report.read_text('utf-8'))
    if result.returncode or not data.get('ok'):raise RuntimeError(name+': '+data.get('error','Screenshot run failed'))
    if not data['controllers'].get('padBridgeLoaded'):raise RuntimeError('Screenshot runtime omitted the controller bridge')
    if name=='browse-night' and not data['graphics']['upscaler']['active']:raise RuntimeError('Requested DLSS did not activate')
    with image.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    captures.append({'file':'docs/media/'+image.name,'size':[1600,900],'fixture':fixture,'view':view,'theme':'night' if night else 'day','api':data['graphics']['api'],'upscaler':data['graphics']['upscaler'].get('provider'),'sha256':digest,'syntheticLibrary':True,'debugErrors':data['graphics']['debugErrors']})

playback=root/'build/controller-tracks/track-focus-night-1600x900.png'
if not playback.exists():raise RuntimeError('Run validate-controller.py --packaged first to capture playback controls')
image=media/'playback-night.png';shutil.copy2(playback,image)
with image.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
captures.append({'file':'docs/media/'+image.name,'size':[1600,900],'fixture':'parity','theme':'night','api':'Direct3D 11','sha256':digest,'syntheticLibrary':True,'sourceScenario':'controller-tracks'})

optical=root/'build/cd-media-validation/dx12-rtx-1600x900-day'
source=optical/'thumbnail-playing-a.png';proof=optical/'thumbnail-playing-a.json'
if not source.exists():raise RuntimeError('Run validate-cd-media.py --packaged first to capture the actual RTX disc')
data=json.loads(proof.read_text('utf-8'))
if not data['graphics']['rayTracing']['active']:raise RuntimeError('RTX screenshot must come from an active RT renderer')
image=media/'cd-raytracing.png';shutil.copy2(source,image)
with image.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
captures.append({'file':'docs/media/'+image.name,'size':[1600,900],'fixture':'cd-media','theme':'day','api':data['graphics']['api'],'reflections':'rtx','sha256':digest,'syntheticLibrary':True,'debugErrors':data['graphics']['debugErrors']})
(media/'screenshots.json').write_text(json.dumps({'packagePayloadSha256':package['payloadSha256'],'exeSha256':package['exeSha256'],'syntheticOnly':True,'captures':captures},ensure_ascii=False,indent=2)+'\n','utf-8')
print('Captured README images:',len(captures),flush=True)
