"""Run upstream fixtures against the packaged native/Sony program components."""
from pathlib import Path
import shutil,subprocess,os,json,time,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
target=root/'build/validation';target.mkdir(parents=True,exist_ok=True)
scripts=target/'scripts';scripts.mkdir(exist_ok=True)
for file in (root/'upstream/scripts').iterdir():
    if file.is_file() and file.suffix=='.mjs':shutil.copy2(file,scripts/file.name)
for file in (root/'runtime/scripts').iterdir():
    if file.is_file() and file.suffix=='.mjs':shutil.copy2(file,scripts/file.name)
shutil.copytree(root/'runtime/scripts/sony-walkman',scripts/'sony-walkman',dirs_exist_ok=True)
shutil.copytree(root/'upstream/scripts/fixtures',scripts/'fixtures',dirs_exist_ok=True)
shutil.copytree(root/'upstream/src',target/'src',dirs_exist_ok=True)
shutil.copytree(root/'runtime/public/icons',target/'public/icons',dirs_exist_ok=True)
modules=target/'node_modules'
if not modules.exists():subprocess.run(['powershell','-NoProfile','-Command',"New-Item -ItemType Junction -Path '"+str(modules)+"' -Target '"+str(root/'runtime/node_modules')+"' | Out-Null"],check=True)
env={**os.environ,'RHINE_NATIVE_AUDIO':str(root/'runtime/vendor/audio/windows-x64/rhine-audio-host.exe'),'SONY_WALKMAN_ASSETS':str(root/'runtime/vendor/sony-walkman'),'SONY_WALKMAN_PYTHON':str(root/'runtime/python/python.exe'),'SONY_WALKMAN_X86_DLL':str(root/'runtime/vendor/sony-walkman-x86/sony-dsp-x86.dll'),'SONY_WALKMAN_NATIVE_DECODER':str(root/'runtime/vendor/audio/windows-x64/rhine-audio-host.exe')}
tests=['check-native-output.mjs','check-audio-rates.mjs','check-sony-integration.mjs','check-ftp-library.mjs','check-online-sources.mjs','check-music-library.mjs','check-album-introductions.mjs','check-music-player.mjs']
started=time.monotonic();command=[str(root/'runtime/node/node.exe'),'--test','--test-concurrency=1',*[str(scripts/name) for name in tests]]
with (root/'build/audio-validation.log').open('w',encoding='utf-8') as log:completed=subprocess.run(command,cwd=target,env=env,stdout=log,stderr=subprocess.STDOUT)
report={'exitCode':completed.returncode,'seconds':time.monotonic()-started,'tests':tests,'components':'packaged native decoder and Sony original runtimes','fixtures':'isolated synthetic / upstream fixtures; no personal music'}
(root/'build/audio-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2));print('\n'.join((root/'build/audio-validation.log').read_text(encoding='utf-8').splitlines()[-22:]));raise SystemExit(completed.returncode)
