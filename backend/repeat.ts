export type RepeatMode = 'off' | 'track' | 'album';
export function repeatSuccessor<T extends {id:string;albumId?:string}>(queue:readonly T[],index:number,mode:RepeatMode):T|undefined {
    const current=queue[index];if(!current)return undefined;
    if(mode==='track')return current;
    if(mode==='album'){
        const album=queue.filter(track=>track.albumId===current.albumId);
        const position=album.findIndex(track=>track.id===current.id);
        return album[(position+1)%album.length];
    }
    return queue[index+1];
}
