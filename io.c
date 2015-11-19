#include <u.h>
#include <libc.h>
#include <blockalloc.h>

u64int
offset2block(Disk *d, u64int offset)
{
	return offset/(d->meta->blocksize);
}

u64int
dread(Disk* d ,void *buf, u64int len, u64int offset)
{
	Blockptr *block;
	void *bbuf;
	u64int ll = len;
	u64int start = offset2block(d,offset);
	u64int tblocks = len/(d->meta->blocksize);
	u64int block0offset = offset - (start*d->meta->blocksize);

	buf = mallocz(len,0); // for performance reasons
	if(len%(d->meta->blocksize) != 0)
		tblocks++;
	if(tblocks == 1){
		readbptr(d,block,start);
		readblock(d,block,bbuf);
		memcpy(buf,(void*)((uchar*)bbuf+block0offset),(ulong)len);
		free(bbuf);
		return len;
	}
	for(u64int i = 0; i < tblocks; i++){
		readbptr(d,block,start+i);
		readblock(d,block,bbuf);
		if(i == 0){
			memcpy(buf,(void*)((uchar*)bbuf+block0offset),(ulong)(d->meta->blocksize-block0offset));
			len = len - (d->meta->blocksize-block0offset);
		} else {
			if(len >= d->meta->blocksize){
				memcpy((void*)((uchar*)buf+d->meta->blocksize*i),bbuf,d->meta->blocksize);
			} else {
				memcpy((void*)((uchar*)buf+d->meta->blocksize*i),bbuf,len);
			}
		}
		free(bbuf);
	}
	return ll;
}

u64int
dwrite(Disk* d , void *buf, u64int len, u64int offset)
{
	Blockptr *block;
	void *bbuf;
	u64int ll = len;
	u64int start = offset2block(d,offset);
	u64int tblocks = len/(d->meta->blocksize);
	u64int block0offset = offset - (start*d->meta->blocksize);

	if(len%(d->meta->blocksize) != 0)
		tblocks++;
	if(tblocks == 1){
		readbptr(d,block,start);
		readblock(d,block,bbuf);
		memcpy((void*)((uchar*)bbuf+block0offset),buf,(ulong)len);
		free(bbuf);
		return len;
	}
	for(u64int i = 0; i < tblocks; i++){
	start:
		readbptr(d,block,start+i);
		readblock(d,block,bbuf);
		if(i == 0){
			memcpy((void*)((uchar*)bbuf+block0offset),buf,(ulong)(d->meta->blocksize-block0offset));
			len = len - (d->meta->blocksize-block0offset);
		} else {
			if(len >= d->meta->blocksize)
				memcpy(bbuf,(void*)((uchar*)buf+d->meta->blocksize*i),d->meta->blocksize);
			else
				memcpy(bbuf,(void*)((uchar*)buf+d->meta->blocksize*i),len);
		}
		if(writeblock(d,block,bbuf) < d->meta->blocksize){
			len = ll;
			goto start;
		}
		free(bbuf);
	}
	return ll;
}
