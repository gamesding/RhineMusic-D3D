from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,str(root/'tools/verify-source.py')],cwd=root,check=True)
print('Bundled fixed licenses verified; no license downloads needed.')
