"""Pin Kenney's official CC0 SVG input prompts, with byte-for-byte sources."""
from pathlib import Path
from prepare import download,sha
import hashlib,json,zipfile
root=Path(__file__).resolve().parents[1]
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
archive=root/'vendor/downloads/kenney_input-prompts_1.5.zip'
manifest_file=root/'vendor/sdk-sources.json';manifest=json.loads(manifest_file.read_text('utf-8'))
source=manifest.get('kenneyInputPrompts')
if not source:
    source={'website':'https://kenney.nl/assets/input-prompts','license':'CC0 1.0','archive':download('https://kenney.nl/media/pages/assets/input-prompts/8de120163f-1783763952/kenney_input-prompts_1.5.zip',archive)}
    manifest['kenneyInputPrompts']=source;manifest_file.write_text(json.dumps(manifest,indent=2)+'\n','utf-8')
assert archive.is_file() and sha(archive)==source['archive']['sha256']
with zipfile.ZipFile(archive) as z:
    ps=['playstation_button_'+n+'_outline' for n in ['cross','circle','square','triangle']]
    ps+=['playstation_trigger_'+n+'_outline' for n in ['l1','r1']]
    xb=['xbox_button_'+n+'_outline' for n in ['a','b','x','y','menu']]+['xbox_lb_outline','xbox_rb_outline']
    for family,names in [('playstation',ps),('xbox',xb)]:
        names+= [family+'_dpad_'+n for n in ['all','up','down','left','right','vertical','horizontal']]
        names+= [family+'_stick_'+side+('_'+n if n else '') for side in ['l','r'] for n in ['','up','down','left','right','press']]
    ps+=['playstation4_button_options_outline','playstation5_button_options_outline']
    ps+=[model+'_touchpad_press_'+n for model in ['playstation4','playstation5'] for n in ['left','center','right']]
    original=root/'vendor/controller-prompts/source';native=root/'runtime/public/controller-prompts'
    original.mkdir(parents=True,exist_ok=True);native.mkdir(parents=True,exist_ok=True)
    hashes={}
    for directory,names in [('PlayStation Series',ps),('Xbox Series',xb)]:
        for name in names:
            relative=directory+'/Vector/'+name+'.svg';data=z.read(relative)
            target=original/(name+'.svg')
            if not target.exists() or target.read_bytes()!=data:target.write_bytes(data)
            text=data.decode('utf-8').replace('fill="#FFFFFF"','fill="currentColor"').replace('fill="#ffffff"','fill="currentColor"')
            text=text.replace('width="64"','width="100%"',1).replace('height="64"','height="100%"',1)
            if 'viewBox=' not in text:text=text.replace('<svg ','<svg viewBox="0 0 64 64" ',1)
            output=native/(name+'.svg');encoded=text.encode('utf-8')
            if not output.exists() or output.read_bytes()!=encoded:output.write_bytes(encoded)
            hashes[name]={'archivePath':relative,'originalSha256':hashlib.sha256(data).hexdigest(),'runtimeSha256':hashlib.sha256(encoded).hexdigest()}
    license=root/'runtime/licenses/kenney-input-prompts/License.txt';license.parent.mkdir(parents=True,exist_ok=True)
    data=z.read('License.txt')
    if not license.exists() or license.read_bytes()!=data:license.write_bytes(data)
    source['licenseSha256']=sha(license);source['icons']=hashes
    metadata=(json.dumps(manifest,indent=2)+'\n').encode('utf-8')
    if manifest_file.read_bytes()!=metadata:manifest_file.write_bytes(metadata)
    print('Kenney official SVG prompts prepared:',len(hashes),'source and theme variants checked.')
