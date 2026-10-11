from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parent.parent;vendor=root/'runtime/vendor/audio/windows-x64';p=vendor/'assets.json';manifest=json.loads(p.read_text('utf-8'))
manifest['files']['rhine-audio-host.exe']=hashlib.sha256((vendor/'rhine-audio-host.exe').read_bytes()).hexdigest()
p.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('Recorded the built native audio binary hash.')
