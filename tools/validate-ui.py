"""Compare native modal geometry with original DOM measurements and render badges."""
from pathlib import Path
import os,json,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1];out=root/'build/ui-parity';out.mkdir(parents=True,exist_ok=True)
references=json.loads((root/'build/ui-parity-reference/metrics.json').read_text(encoding='utf-8'))
assert len(references)==16 and all(x['metrics']['.music-panel'] for x in references),'Incomplete original DOM reference'
cases=[]
for item in references:
    view=item['panel'];v=item['viewport'];cases.append({'name':f"{v['width']}x{v['height']}-{view}",'view':view,'width':round(v['width']*1.25),'height':round(v['height']*1.25),'reference':item['metrics']['.music-panel'],'nodes':item['metrics']})
cases.extend([{'name':'hi-res-browse','view':'browse'},{'name':'hi-res-detail','view':'detail','width':1920,'height':1080},{'name':'dsd-detail','view':'dsd-detail','width':1920,'height':1080},{'name':'dsd-night','view':'dsd-detail','night':True,'api':'dx12','width':1920,'height':1080},{'name':'ui-input-and-motion','view':'browse','scenario':'ui-parity'}])
results=[]
for case in cases:
    name=case['name'];print('Rendering '+name,flush=True);report=out/(name+'.json')
    args=[str(root/'RhineMusicD3D.exe'),'--self-test','--demo','--fixture','audio-tech','--api',case.get('api','dx11'),'--upscaler','native','--view',case['view'],'--width',str(case.get('width',1280)),'--height',str(case.get('height',720)),'--duration','9','--report',str(report),'--image',str(out/(name+'.png'))]
    if case.get('night'):args+=['--night']
    if case.get('scenario'):args+=['--scenario',case['scenario']]
    env=os.environ.copy();env['RHINE_D3D_DEBUG']='1';child=subprocess.run(args,cwd=root,env=env,creationflags=0x08000000,timeout=150);data=json.loads(report.read_text(encoding='utf-8'));assert child.returncode==0 and data.get('ok'),data.get('error','Native render failed')
    delta=None
    node_deltas={}
    if 'reference' in case:
        original=case['reference'];b=data['ui']['panelBounds'];delta=max(abs(b[0]-original['x']),abs(b[2]-b[0]-original['width']))
        delta=max(delta,abs(b[1]-original['y']),abs(b[3]-b[1]-original['height']))
        assert delta<2.0,(name,delta,b,original)
        for selector,original_node in case['nodes'].items():
            if selector=='.music-panel' or original_node is None:continue
            native_node=data['ui']['panelNodes'].get(selector)
            assert native_node is not None,(name,'Missing original UI control',selector)
            dimensions=['x','y','width'] if selector=='#panel-body' and case['view']=='settings' else ['x','y','width','height']
            node_delta=max(abs(native_node[i]-original_node[dimension]) for i,dimension in enumerate(dimensions))
            assert node_delta<2.0,(name,selector,node_delta,native_node,original_node)
            node_deltas[selector]=node_delta
    results.append({'name':name,'ok':True,'geometryDeltaCssPixels':delta,'internalNodeDeltaCssPixels':node_deltas,'checks':len(data.get('checks',[])),'badgeCount':len(data['ui']['audioBadges']),'report':str(report.relative_to(root))})
    (root/'build/ui-parity-validation.json').write_text(json.dumps({'ok':True,'completed':len(results),'planned':len(cases),'cases':results},indent=2),encoding='utf-8')
print('Passed original modal geometry and native UI scenarios:',len(results))
