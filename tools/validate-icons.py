from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve().parents[1]
original=(root/'upstream/windows/app.ico').read_bytes();assert (root/'windows/app.ico').read_bytes()==original
reserved,kind,count=struct.unpack_from('<HHH',original);assert reserved==0 and kind==1 and count>0
images=[]
for i in range(count):
    entry=struct.unpack_from('<BBBBHHII',original,6+i*16);images.append((entry[:6],original[entry[7]:entry[7]+entry[6]]))
def resources(file):
    source=file.read_bytes();pe=struct.unpack_from('<I',source,60)[0];assert source[pe:pe+4]==b'PE\0\0'
    sections=struct.unpack_from('<H',source,pe+6)[0];optional_size=struct.unpack_from('<H',source,pe+20)[0];optional=pe+24
    magic=struct.unpack_from('<H',source,optional)[0];directory=optional+(112 if magic==0x20b else 96)
    resource_rva=struct.unpack_from('<I',source,directory+16)[0];section_headers=optional+optional_size
    table=[]
    for i in range(sections):
        _,virtual_size,rva,raw_size,offset=struct.unpack_from('<8sIIII',source,section_headers+i*40);table.append((rva,max(virtual_size,raw_size),offset))
    def physical(address):
        for rva,size,offset in table:
            if rva<=address<rva+size:return offset+address-rva
        raise ValueError('Resource RVA outside sections')
    base=physical(resource_rva);values={}
    def walk(offset,keys=()):
        named,ids=struct.unpack_from('<HH',source,base+offset+12)
        for i in range(named+ids):
            key,target=struct.unpack_from('<II',source,base+offset+16+i*8);next_keys=keys+(key,)
            if target&0x80000000:walk(target&0x7fffffff,next_keys)
            else:
                rva,size=struct.unpack_from('<II',source,base+target);values[next_keys]=source[physical(rva):physical(rva)+size]
    walk(0);return values
checks=[]
for name in ['RhineMusicD3D.exe','Release/RhineMusic.exe','Release/RhineConfiguration.exe']:
    values=resources(root/name);groups=[(keys,data) for keys,data in values.items() if keys[0]==14];assert groups,name+' missing icon group'
    matched=[]
    for keys,group in groups:
        if struct.unpack_from('<HHH',group)!=(0,1,count):continue
        for i,(dimensions,pixels) in enumerate(images):
            entry=struct.unpack_from('<BBBBHHIH',group,6+i*14)
            if entry[:6]!=dimensions or entry[6]!=len(pixels):break
            data=values.get((3,entry[7],keys[2]));
            if data!=pixels:break
        else:matched.append(keys[1])
    assert matched,name+' has a different icon'
    if name=='RhineMusicD3D.exe':assert 101 in matched
    checks.append({'exe':name,'originalIconImages':count,'matchingResourceIds':matched,'sha256':hashlib.sha256((root/name).read_bytes()).hexdigest()})
report={'ok':True,'originalIcoSha256':hashlib.sha256(original).hexdigest(),'unchangedIcoBytes':len(original),'checks':checks}
(root/'build/icon-validation.json').write_text(json.dumps(report,indent=2),'utf-8');print('Original program icon retained byte for byte in all three EXEs ('+str(count)+' resolutions).')
