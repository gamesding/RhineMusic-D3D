"""Run the final private native release checks in a deterministic order."""
from pathlib import Path
import subprocess,sys
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[1]
commands=[
    ['tools/validate-native.py'],
    ['tools/validate-ui.py'],
    ['tools/validate-main-ui.py'],
    ['tools/validate-personal-ui.py'],
    ['tools/audit-package-inputs.py'],
    ['tools/package.py'],
    ['tools/validate-standalone.py'],
    ['tools/validate-packaged-audio.py'],
    ['tools/validate-controller.py','--packaged'],
    ['tools/write-validation-report.py'],
]
for command in commands:
    print('Release verification: '+command[0],flush=True)
    subprocess.run([sys.executable,*command],cwd=root,check=True,creationflags=0x08000000)
print('Final private EXE and all required release checks completed.',flush=True)
