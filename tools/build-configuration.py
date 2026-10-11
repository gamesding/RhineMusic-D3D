"""Compile the standalone CPU configuration UI with Windows Framework tools."""
from pathlib import Path
import argparse,os,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    probe=args.probe.resolve();output=args.output.resolve()
    if not probe.is_file():raise RuntimeError('Build RhineConfigurationProbe first')
    output.parent.mkdir(parents=True,exist_ok=True)
    compiler=Path(os.environ.get('SystemRoot','C:/Windows'))/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
    # pathlib emits native Windows separators. The legacy Framework compiler
    # treats forward slashes in source arguments as option separators.
    command=[str(compiler),'/nologo','/utf8output','/codepage:65001','/target:winexe','/platform:x64','/optimize+',
        '/win32manifest:'+str(ROOT/'windows/configuration.manifest'),'/win32icon:'+str(ROOT/'windows/app.ico'),
        '/out:'+str(output),'/r:System.Windows.Forms.dll','/r:System.Drawing.dll','/r:System.Web.Extensions.dll',
        '/resource:'+str(probe)+',Rhine.Configuration.Probe']
    command.extend(str(ROOT/'windows'/name) for name in ['ConfigurationModel.cs','Configuration.cs','ConfigurationTests.cs'])
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=0x08000000)
    sys.stdout.reconfigure(encoding='utf-8');print(result.stdout.decode('utf-8',errors='replace'),end='')
    if result.returncode:raise subprocess.CalledProcessError(result.returncode,command)
    print('Standalone configuration built:',output)
if __name__=='__main__':main()
