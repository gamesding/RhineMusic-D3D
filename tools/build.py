"""Build the complete native source distribution without downloading SDKs."""
from pathlib import Path
import argparse, json, os, shutil, subprocess, sys, zipfile
from prepare import sha

ROOT=Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding='utf-8')
os.environ['PYTHONUTF8']='1'

def run(command):
    print('> '+subprocess.list2cmdline([str(x) for x in command]),flush=True)
    child=subprocess.Popen([str(x) for x in command],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=0x08000000)
    for line in iter(child.stdout.readline,b''):
        print(line.decode('utf-8',errors='replace').rstrip(),flush=True)
    code=child.wait()
    if code:raise subprocess.CalledProcessError(code,command)

def restore_dependencies():
    archive=ROOT/'vendor/build-dependencies.zip'
    stamp=json.loads((ROOT/'vendor/build-dependencies.json').read_text('utf-8'))
    if sha(archive)!=stamp['sha256']:raise RuntimeError('Offline build dependency archive failed its SHA-256 check')
    destination=ROOT/'build-tools/node_modules'
    needs_restore=any(not (destination/name).is_file() or sha(destination/name)!=digest for name,digest in stamp['files'].items())
    if needs_restore:
        print('Restoring verified offline Node build dependencies...',flush=True)
        destination.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(archive) as source:
            for item in source.infolist():
                relative=Path(item.filename)
                target=(destination/relative).resolve()
                if relative.is_absolute() or '..' in relative.parts or not target.is_relative_to(destination.resolve()):raise RuntimeError('Unsafe dependency archive entry')
                if item.is_dir():target.mkdir(parents=True,exist_ok=True)
                else:
                    target.parent.mkdir(parents=True,exist_ok=True)
                    target.write_bytes(source.read(item))
    if any(sha(destination/name)!=digest for name,digest in stamp['files'].items()):raise RuntimeError('Restored build dependency hash mismatch')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true',help='Run CTest and isolated software regression after packaging')
    parser.add_argument('--configure-only',action='store_true')
    parser.add_argument('--jobs',type=int,default=4)
    args=parser.parse_args()
    if sys.platform!='win32' or sys.version_info<(3,11):raise RuntimeError('Windows x64 and Python >= 3.11 are required')
    if not shutil.which('cmake'):raise RuntimeError('Install CMake >= 3.24 and add it to PATH')
    run([sys.executable,ROOT/'tools/verify-source.py'])
    restore_dependencies()
    audio_assets=ROOT/'runtime/vendor/audio/windows-x64/assets.json'
    if not audio_assets.exists():shutil.copy2(audio_assets.with_name('audio-assets.template.json'),audio_assets)
    for name in ['prepare-controls.py','prepare-opencc.py','prepare-search.py','prepare-ui.py','prepare-controller-icons.py','prepare-audio.py']:
        run([sys.executable,ROOT/'tools'/name])
    run([ROOT/'runtime/node/node.exe',ROOT/'tools/build-backend.mjs'])
    run(['cmake','-S',ROOT,'-B',ROOT/'build','-G','Visual Studio 17 2022','-A','x64','-DPython3_EXECUTABLE='+sys.executable])
    if args.configure_only:return
    run(['cmake','--build',ROOT/'build','--config','Release','--parallel',str(max(1,args.jobs))])
    run([sys.executable,ROOT/'tools/stamp-audio.py'])
    run([sys.executable,ROOT/'tools/precompile.py'])
    run([sys.executable,ROOT/'tools/package.py','--no-build'])
    run([sys.executable,ROOT/'tools/audit-package-inputs.py'])
    if args.verify:
        run(['ctest','--test-dir',ROOT/'build','-C','Release','--output-on-failure'])
        run([sys.executable,ROOT/'tools/validate-final-audio.py'])
        run([sys.executable,ROOT/'tools/validate-standalone.py','--software-only'])
        run([sys.executable,ROOT/'tools/validate-configuration.py'])
        run([sys.executable,ROOT/'tools/validate-icons.py'])
    print('Build complete: '+str(ROOT/'Release/RhineMusic.exe'),flush=True)

if __name__=='__main__':main()
