from pathlib import Path
import hashlib,json,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
def sha(file):
    with file.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
snapshot=json.loads((root/'source-manifest.json').read_text(encoding='utf-8'))
changed=[name for name,digest in snapshot['files'].items() if not (root/'upstream'/name).is_file() or sha(root/'upstream'/name)!=digest]
sony=json.loads((root/'runtime/scripts/sony-walkman/assets.json').read_text(encoding='utf-8'))
sony_files={name:sha(root/'runtime/vendor/sony-walkman'/name)==digest for name,digest in sony['files'].items()}
audio=json.loads((root/'runtime/vendor/audio/windows-x64/assets.json').read_text(encoding='utf-8'))
audio_files={name:sha(root/'runtime/vendor/audio/windows-x64'/name)==digest for name,digest in audio['files'].items()}
sdk=json.loads((root/'vendor/sdk-sources.json').read_text('utf-8'))
controller=sdk['kenneyInputPrompts'];controller_icons={name:sha(root/'vendor/controller-prompts/source'/(name+'.svg'))==value['originalSha256'] and sha(root/'runtime/public/controller-prompts'/(name+'.svg'))==value['runtimeSha256'] for name,value in controller['icons'].items()}
controller_license=sha(root/'runtime/licenses/kenney-input-prompts/License.txt')==controller['licenseSha256']
files=[];suspicious=[];media=[]
for base in ['backend','shaders','runtime']:
    for file in (root/base).rglob('*'):
        if not file.is_file():continue
        rel=file.relative_to(root).as_posix()
        if any(part in {'.cache','__pycache__'} for part in file.parts):continue
        files.append({'path':rel,'bytes':file.stat().st_size})
        if file.name.lower() in {'preferences.json','config.json','index.json','.env','.ds_store'} or any(part in {'music-data','music-data-v3','cover-cache','sony-pcm-cache'} for part in file.parts):suspicious.append(rel)
        if file.suffix.lower() in {'.mp3','.wav','.flac','.m4a','.aac','.aif','.aiff','.dsf','.dff','.ape','.opus','.ogg','.wma'}:media.append(rel)
report={'sourceSnapshotFiles':len(snapshot['files']),'sourceSnapshotChanged':changed,'sonyRevision':sony.get('revision'),'sonyOriginalBytes':sony_files,'audioRuntimeHashes':audio_files,'suspiciousStateFiles':suspicious,'bundledAudioAssets':media,'programFileCount':len(files),'programBytes':sum(f['bytes'] for f in files),'largestFiles':sorted(files,key=lambda f:f['bytes'],reverse=True)[:12]}
report['ok']=not changed and all(sony_files.values()) and all(audio_files.values()) and not suspicious
report['controllerSvgSources']=controller_icons;report['controllerSvgLicense']=controller_license
report['ok']=report['ok'] and len(controller_icons)==59 and all(controller_icons.values()) and controller_license
(root/'build/package-input-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['ok']:raise RuntimeError('Package-input audit failed')
