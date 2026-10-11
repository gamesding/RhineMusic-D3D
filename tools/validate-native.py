"""Native application regression and measured process-family performance."""
from pathlib import Path
import os,json,subprocess,time,sys,ctypes as c
from ctypes import wintypes as w
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parent.parent
if not (root/'CMakeLists.txt').is_file() or not (root/'native/main.cpp').is_file():raise RuntimeError('Expected the RhineMusic D3D project root')
out=root/'build/native-review';out.mkdir(parents=True,exist_ok=True)
class Entry(c.Structure):
    _fields_=[('size',w.DWORD),('usage',w.DWORD),('pid',w.DWORD),('heap',c.c_size_t),('module',w.DWORD),('threads',w.DWORD),('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('name',w.WCHAR*260)]
class Memory(c.Structure):
    _fields_=[('cb',w.DWORD),('faults',w.DWORD),*[(n,c.c_size_t) for n in ['peakWorking','working','peakPaged','paged','peakNonpaged','nonpaged','pagefile','peakPagefile','private']]]
k=c.WinDLL('kernel32',use_last_error=True);p=c.WinDLL('psapi',use_last_error=True)
k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE
k.Process32FirstW.argtypes=[w.HANDLE,c.POINTER(Entry)];k.Process32NextW.argtypes=[w.HANDLE,c.POINTER(Entry)]
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.CloseHandle.argtypes=[w.HANDLE];k.GetProcessTimes.argtypes=[w.HANDLE,*([c.POINTER(w.FILETIME)]*4)]
p.GetProcessMemoryInfo.argtypes=[w.HANDLE,c.POINTER(Memory),w.DWORD]
def family(pid):
    snapshot=k.CreateToolhelp32Snapshot(2,0);entry=Entry();entry.size=c.sizeof(Entry);parents={};names={}
    try:
        more=k.Process32FirstW(snapshot,c.byref(entry))
        while more:parents[entry.pid]=entry.parent;names[entry.pid]=entry.name;more=k.Process32NextW(snapshot,c.byref(entry))
    finally:k.CloseHandle(snapshot)
    ids={pid}
    while True:
        extra={i for i,parent in parents.items() if parent in ids};before=len(ids);ids|=extra
        if len(ids)==before:break
    processes=[]
    for i in ids:
        handle=k.OpenProcess(0x1010,False,i)
        if not handle:continue
        try:
            memory=Memory();memory.cb=c.sizeof(memory);times=[w.FILETIME() for _ in range(4)]
            if not p.GetProcessMemoryInfo(handle,c.byref(memory),c.sizeof(memory)):continue
            cpu=0
            if k.GetProcessTimes(handle,*[c.byref(t) for t in times]):cpu=sum((t.dwHighDateTime<<32)|t.dwLowDateTime for t in times[2:])/1e7
            processes.append({'pid':i,'name':names.get(i,''),'privateBytes':memory.private,'workingSetBytes':memory.working,'cpuSeconds':cpu})
        finally:k.CloseHandle(handle)
    return processes
cases=[
 {'name':'large-covers','scenario':'covers-stress','view':'browse','duration':60,'api':'dx11','upscaler':'native','fixture':'large'},
 {'name':'stress','scenario':'stress','view':'browse','duration':30,'api':'dx11','upscaler':'native'},
 {'name':'cd-playback','scenario':'cd-stress','view':'browse','duration':30,'api':'dx11','upscaler':'native','cd':True},
 {'name':'compact-night-detail','view':'detail','night':True},
 {'name':'portrait-day-detail','view':'detail','width':900,'height':1600},
 {'name':'ultrawide-night-detail','view':'detail','width':2100,'height':900,'night':True},
 {'name':'small-reduced-browse','view':'browse','width':800,'height':600,'reduced':True},
 {'name':'rtx-dlss-detail','view':'detail','width':1920,'height':1080,'api':'dx12','upscaler':'dlss','reflections':'rtx','night':True},
 {'name':'dx11-native-ssr-performance','view':'detail','scenario':'performance','duration':30},
 {'name':'dx11-native-off-performance','view':'detail','scenario':'performance','duration':30,'reflections':'off'},
 {'name':'dx11-fsr-performance','view':'detail','scenario':'performance','duration':30,'upscaler':'fsr'},
 {'name':'dx12-dlss-rtx-performance','view':'detail','scenario':'performance','duration':30,'api':'dx12','upscaler':'dlss','reflections':'rtx'},
 {'name':'dx12-fsr-rtx-performance','view':'detail','scenario':'performance','duration':30,'api':'dx12','upscaler':'fsr','reflections':'rtx'},
]
results=[]
for case in cases:
    name=case['name'];report=out/(name+'.json');screenshot=out/(name+'.png');performance=case.get('scenario')=='performance'
    args=[str(root/'RhineMusicD3D.exe'),'--self-test','--api',case.get('api','dx11'),'--upscaler',case.get('upscaler','native'),'--reflections',case.get('reflections','ssr'),'--view',case['view'],'--width',str(case.get('width',1280)),'--height',str(case.get('height',720)),'--duration',str(case.get('duration',9)),'--report',str(report),'--image',str(screenshot)]
    if not case.get('cd'):args+=['--demo','--fixture',case.get('fixture','parity')]
    for flag in ['night','reduced']:
        if case.get(flag):args+=['--'+flag]
    if case.get('scenario'):args+=['--scenario',case['scenario']]
    env=os.environ.copy();env.pop('RHINE_DEMO',None);env.pop('RHINE_DEMO_FIXTURE',None)
    if performance:env.pop('RHINE_D3D_DEBUG',None)
    else:env['RHINE_D3D_DEBUG']='1'
    print('Running '+name,flush=True);start=time.monotonic();child=subprocess.Popen(args,cwd=root,env=env,creationflags=0x08000000);samples=[]
    while child.poll() is None:
        processes=family(child.pid);samples.append({'time':time.monotonic()-start,'privateBytes':sum(x['privateBytes'] for x in processes),'workingSetBytes':sum(x['workingSetBytes'] for x in processes),'processes':processes});time.sleep(.5)
        if time.monotonic()-start>170:child.kill();raise RuntimeError('Native test deadline: '+name)
    data=json.loads(report.read_text(encoding='utf-8'));okay=child.returncode==0 and data.get('ok') is True
    result={'name':name,'ok':okay,'exitCode':child.returncode,'report':str(report.relative_to(root)),'screenshot':str(screenshot.relative_to(root)),'checks':len(data.get('checks',[])),'elapsedSeconds':time.monotonic()-start,'gpuTiming':data.get('graphics',{}).get('gpuTiming'),'debugErrors':data.get('graphics',{}).get('debugErrors'),'debugWarnings':data.get('graphics',{}).get('debugWarnings'),'processFamilySamples':samples}
    results.append(result);(root/'build/native-validation.json').write_text(json.dumps({'ok':all(x['ok'] for x in results),'completed':len(results),'planned':len(cases),'cases':results},indent=2)+'\n',encoding='utf-8')
    print(name+': '+('PASS' if okay else 'FAIL: '+data.get('error','unknown')),flush=True)
    if not okay:raise RuntimeError('Native regression failed: '+name)
print('Passed all '+str(len(results))+' native cases.',flush=True)
