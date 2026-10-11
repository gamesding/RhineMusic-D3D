"""Exercise own native UI controls, preserving standard EDIT caret behavior."""
from pathlib import Path
import os,json,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/controls-update';out.mkdir(parents=True,exist_ok=True)
results=[]
for width,height in [(1600,900),(1000,750),(488,1055)]:
    name=f'{width}x{height}';print('Testing updated native controls '+name,flush=True);report=out/(name+'.json')
    args=[str(root/'RhineMusicD3D.exe'),'--self-test','--demo','--fixture','audio-tech','--api','dx11','--upscaler','native','--view','browse','--width',str(width),'--height',str(height),'--scenario','controls-update','--report',str(report),'--image',str(out/(name+'.png'))]
    env=os.environ.copy();env['RHINE_D3D_DEBUG']='1'
    child=subprocess.run(args,cwd=root,env=env,creationflags=0x08000000,timeout=210)
    data=json.loads(report.read_text('utf-8'));assert child.returncode==0 and data.get('ok'),data.get('error','native controls failed')
    results.append({'name':name,'ok':True,'checks':len(data['checks']),'report':str(report.relative_to(root))})
    (root/'build/controls-validation.json').write_text(json.dumps({'ok':True,'cases':results},indent=2),'utf-8')
print('Updated native UI scenarios passed:',len(results))
