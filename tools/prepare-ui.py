"""Adapt original SVG stylesheets into Direct2D presentation attributes.
Original files, geometry and Hi-Res colors/gradient stops remain intact.
The derived monochrome logos use currentColor, matching the original CSS masks.
"""
from pathlib import Path
import re, json, hashlib, shutil, xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
original_icon=root/'upstream/windows/app.ico'
(root/'windows').mkdir(parents=True,exist_ok=True)
shutil.copy2(original_icon,root/'windows/app.ico')
svg='http://www.w3.org/2000/svg'
ET.register_namespace('',svg);ET.register_namespace('xlink','http://www.w3.org/1999/xlink')
directory=root/'runtime/public/audio-tech';directory.mkdir(parents=True,exist_ok=True)
manifest=[]
for original in (root/'upstream/public/audio-tech').glob('*.svg'):
    data=original.read_bytes();document=ET.fromstring(data)
    styles={}
    for node in document.iter('{'+svg+'}style'):
        for cls,body in re.findall(r'\.([\w-]+)\s*\{([^}]+)\}',node.text or ''):
            styles[cls]=dict(item.strip().split(':',1) for item in body.split(';') if ':' in item)
    for node in document.iter():
        attrs={}
        for cls in node.get('class','').split():attrs.update(styles.get(cls,{}))
        attrs.update(dict(item.strip().split(':',1) for item in node.get('style','').split(';') if ':' in item))
        node.attrib.pop('style',None);node.attrib.pop('class',None)
        for key,value in attrs.items():
            if key in {'display','fill','fill-opacity','fill-rule','clip-path','clip-rule','stroke','stroke-width','stroke-opacity','stop-color','stop-opacity','opacity','visibility'}:node.set(key,value)
    for parent in document.iter():
        for child in list(parent):
            if child.tag=='{'+svg+'}style' or not child.tag.startswith('{'+svg+'}'):parent.remove(child)
    if 'viewBox' not in document.attrib:
        document.set('viewBox','0 0 '+document.get('width','1')+' '+document.get('height','1'))
    document.attrib.pop('width',None);document.attrib.pop('height',None)
    if original.stem in {'dsd','dsee','s-master','vinyl'}:
        for node in document.iter():
            if node.tag in {'{'+svg+'}path','{'+svg+'}polygon','{'+svg+'}rect'}:node.set('fill','currentColor')
    target=directory/('native-'+original.name);ET.ElementTree(document).write(target,encoding='utf-8',xml_declaration=True)
    manifest.append({'source':original.name,'sourceSha256':hashlib.sha256(data).hexdigest(),'native':target.name,'nativeSha256':hashlib.sha256(target.read_bytes()).hexdigest()})
(root/'build').mkdir(parents=True,exist_ok=True)
(root/'build/ui-assets.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('Prepared original SVG presentation attributes and view boxes:',len(manifest))
