from pathlib import Path
import argparse,subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1]
commands=[['tools/validate-cd-controls.py'],['tools/validate-main-ui.py'],['tools/audit-package-inputs.py'],['tools/package.py'],['tools/validate-icons.py'],['tools/validate-standalone.py'],['tools/validate-controller.py','--packaged'],['tools/validate-cd-controls.py','--packaged'],['tools/validate-notifications.py','--packaged'],['tools/validate-haptics.py','--packaged']]
parser=argparse.ArgumentParser();parser.add_argument('--start-at',choices=[c[0] for c in commands]);options=parser.parse_args()
if options.start_at:commands=commands[next(i for i,c in enumerate(commands) if c[0]==options.start_at):]
for command in commands:
    print('Controller/CD release check: '+command[0],flush=True)
    result=subprocess.run([sys.executable,*command],cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,encoding='utf-8',errors='replace',creationflags=0x08000000)
    print(result.stdout[-6000:],flush=True)
    if result.returncode:raise RuntimeError('Release check failed: '+command[0])
print('Final package, original icons, controller transport and CD/footer controls validated.',flush=True)
