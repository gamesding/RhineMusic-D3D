// Keep ejection on the trusted native IPC route. The helper verifies both
// optical drive type and current TOC identity before the Windows operation.
export function cdAlbumForEject(albums,id){
  if(typeof id!=='string')throw new Error('请选择 CD 专辑');
  const album=albums.find(album=>album.id===id);
  if(!album||!album.cd||!/^cd-[a-f0-9]{32}$/.test(album.id)||!/^[A-Z]:$/.test(album.cd.drive)||typeof album.cd.discId!=='string'||!album.cd.discId)throw new Error('当前 CD 已移除或不在专辑架中，请刷新光驱');
  return album;
}
export async function performCdEject(album,{current,stop,release,eject,removed}){
  if(current()?.albumId===album.id)await stop();
  await release(album.cd.drive);
  const result=await eject(album.cd.drive,album.cd.discId);
  if(result?.ejected!==true||result.drive!==album.cd.drive)throw new Error('光驱没有确认 CD 弹出');
  await removed(album);
  return {...result,albumId:album.id};
}
