"""Compare native browse/detail geometry to the unchanged original DOM fixture."""
from pathlib import Path
import json,os,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/ui-parity';out.mkdir(parents=True,exist_ok=True)
references=json.loads((root/'build/ui-parity-reference/main-metrics.json').read_text(encoding='utf-8'));results=[]
assert len(references)==8
for case in references:
 v=case['viewport'];name=f"{v['width']}x{v['height']}-{case['view']}";print('Compare '+name,flush=True);report=out/(name+'.json')
 args=[str(root/'RhineMusicD3D.exe'),'--self-test','--demo','--fixture','audio-tech','--api','dx11','--upscaler','native','--quality','performance','--reduced','--view',case['view'],'--width',str(round(v['width']*1.25)),'--height',str(round(v['height']*1.25)),'--duration','9','--report',str(report),'--image',str(out/(name+'.png'))]
 env=os.environ.copy();env['RHINE_D3D_DEBUG']='1';p=subprocess.run(args,cwd=root,env=env,creationflags=0x08000000,timeout=90);data=json.loads(report.read_text(encoding='utf-8'));assert p.returncode==0 and data.get('ok'),data.get('error')
 deltas={}
 for selector,b in data['ui']['layoutNodes'].items():
  if selector in {'back-keycap','playback-repeat','playback-glass','cd-eject','browse-caption','music-navigation-occupied','music-navigation-hint'}:continue # New controls / diagnostic bounds are checked by controls and CD/footer regressions. Existing .music-navigation remains compared.
  original=case['metrics'].get(selector);assert original,(name,selector,'Original node missing');original=original[0];delta=max(abs(b[i]-original[k]) for i,k in enumerate(['x','y','width','height']));deltas[selector]=delta
  assert delta<2.0,(name,selector,delta,b,original)
 assert len(deltas)>1,(name,'Missing internal native geometry evidence')
 results.append({'name':name,'ok':True,'geometryDeltaCssPixels':deltas,'report':str(report.relative_to(root))});(root/'build/main-ui-validation.json').write_text(json.dumps({'ok':True,'completed':len(results),'planned':8,'cases':results},indent=2),encoding='utf-8')
print('Passed original browse/detail measurements:',len(results))
