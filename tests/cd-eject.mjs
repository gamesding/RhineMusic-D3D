import test from 'node:test';
import assert from 'node:assert/strict';
import {cdAlbumForEject,performCdEject} from '../backend/cd-eject.mjs';
const album={id:'cd-'+ 'a'.repeat(32),cd:{drive:'F:',discId:'toc-identity'},tracks:[]};
test('only a current enumerated CD album can authorize ejection',()=>{
  assert.equal(cdAlbumForEject([album],album.id),album);
  for(const id of [null,{},'demo-sun','F:',album.id+'stale'])assert.throws(()=>cdAlbumForEject([album],id));
  assert.throws(()=>cdAlbumForEject([],album.id));
  for(const cd of [{drive:'C:/music',discId:'toc'},{drive:'f:',discId:'toc'},{drive:'F:',discId:''},{drive:'F:',discId:12}])assert.throws(()=>cdAlbumForEject([{...album,cd}],album.id));
});
function dependencies(current,failAt){
  const calls=[];const run=name=>async(...args)=>{calls.push([name,...args]);if(failAt===name)throw new Error(name+' failed');return name==='eject'?{ejected:true,drive:album.cd.drive}:undefined;};
  return {calls,current:()=>current,stop:run('stop'),release:run('release'),eject:run('eject'),removed:run('removed')};
}
test('active CD playback stops and releases its drive before the verified eject',async()=>{
  const d=dependencies({albumId:album.id});const result=await performCdEject(album,d);
  assert.deepEqual(d.calls.map(c=>c[0]),['stop','release','eject','removed']);
  assert.deepEqual(d.calls[2],['eject','F:','toc-identity']);assert.equal(result.albumId,album.id);
});
test('ejecting the selected CD preserves another album playback',async()=>{
  const d=dependencies({albumId:'local-other'});await performCdEject(album,d);
  assert.deepEqual(d.calls.map(c=>c[0]),['release','eject','removed']);assert.equal(d.calls[0][1],'F:');
});
test('idle CD selection can eject without inventing a playback session',async()=>{
  const d=dependencies(null);await performCdEject(album,d);assert.equal(d.calls.some(c=>c[0]==='stop'),false);
});
for(const point of ['stop','release','eject'])test(point+' failure retains the CD and stops the eject sequence',async()=>{
  const d=dependencies({albumId:album.id},point);await assert.rejects(()=>performCdEject(album,d),new RegExp(point+' failed'));
  assert.equal(d.calls.some(c=>c[0]==='removed'),false);assert.equal(d.calls.at(-1)[0],point);
});
test('a wrong-drive or unconfirmed hardware reply cannot remove the album',async()=>{
  for(const result of [{ejected:false,drive:'F:'},{ejected:true,drive:'G:'},undefined]){
    const d=dependencies(null);d.eject=async()=>result;
    await assert.rejects(()=>performCdEject(album,d),/没有确认/);assert.equal(d.calls.some(c=>c[0]==='removed'),false);
  }
});
