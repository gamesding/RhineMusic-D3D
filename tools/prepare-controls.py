"""Pin official repeat icons and the official WASAPI add-on; retain notices."""
from pathlib import Path
from prepare import api,download,sha
import argparse,json,shutil,zipfile

p=argparse.ArgumentParser();p.add_argument('--download',action='store_true');args=p.parse_args()
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
manifestFile=root/'vendor/sdk-sources.json';manifest=json.loads(manifestFile.read_text('utf-8'))
archive=root/'vendor/downloads/basswasapi24.zip'
if args.download or not archive.exists():
    manifest['basswasapi']={'archive':download('https://www.un4seen.com/files/basswasapi24.zip',archive),'documentation':'https://www.un4seen.com/doc/basswasapi/BASS_WASAPI_Init.html'}
assert sha(archive)==manifest['basswasapi']['archive']['sha256']
destination=root/'vendor/basswasapi';destination.mkdir(parents=True,exist_ok=True)
def write_if_changed(file,data):
    if not file.exists() or file.read_bytes()!=data:file.write_bytes(data)
def copy_if_changed(source,target):
    if not target.exists() or sha(source)!=sha(target):shutil.copy2(source,target)
with zipfile.ZipFile(archive) as source:
    for name in ['c/basswasapi.h','basswasapi.txt','x64/basswasapi.dll']:
        file=destination/name;file.parent.mkdir(parents=True,exist_ok=True);write_if_changed(file,source.read(name))
audio=root/'runtime/vendor/audio/windows-x64'
copy_if_changed(destination/'c/basswasapi.h',audio/'include/basswasapi.h')
copy_if_changed(destination/'x64/basswasapi.dll',audio/'basswasapi.dll')
copy_if_changed(destination/'basswasapi.txt',audio/'licenses/basswasapi.txt')
manifest['basswasapi']['dllSha256']=sha(audio/'basswasapi.dll')
tabler=manifest.get('tabler')
if args.download or not tabler:
    revision=api('https://api.github.com/repos/tabler/tabler-icons/commits/main')['sha']
    tabler={'repository':'tabler/tabler-icons','revision':revision,'files':{}}
icons=root/'runtime/public/native-icons';icons.mkdir(parents=True,exist_ok=True)
for name in ['repeat','repeat-once','repeat-off','player-eject','disc']:
    relative='icons/outline/'+name+'.svg';file=icons/(name+'.svg')
    if args.download or not file.exists():tabler['files'][relative]=download('https://raw.githubusercontent.com/tabler/tabler-icons/'+tabler['revision']+'/'+relative,file)
    assert sha(file)==tabler['files'][relative]['sha256']
license=root/'runtime/licenses/tabler-icons/LICENSE';license.parent.mkdir(parents=True,exist_ok=True)
if args.download or not license.exists():tabler['files']['LICENSE']=download('https://raw.githubusercontent.com/tabler/tabler-icons/'+tabler['revision']+'/LICENSE',license)
assert sha(license)==tabler['files']['LICENSE']['sha256']
manifest['tabler']=tabler
write_if_changed(manifestFile,(json.dumps(manifest,indent=2)+'\n').encode('utf-8'))
print(json.dumps({'basswasapiSha256':manifest['basswasapi']['dllSha256'],'tablerRevision':tabler['revision'],'repeatIcons':3,'ejectIcons':1},indent=2))
