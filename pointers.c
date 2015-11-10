#include <u.h>
#include <libc.h>
#include <blockalloc.h>

u64int
preadn(int fd, void *buf, u64int offset, u64int len)
{
	int nfd = dup(fd,-1);
	seek(nfd, (vlong) offset, 0);
	u64int retval = (u64int) readn(nfd, buf, (vlong) len);
	close(nfd);
	return retval;
}

u64int
pwriten(int fd, void *buf, u64int offset, u64int len)
{
	return (u64int)pwrite(fd,buf,(long) len, (vlong) offset);
}

int
loadbptrs(Disk *d)
{
	Fn *f = infn("loadbptrs");
	int fd = d->fd;
	Metablock *m = d->meta;
	u64int ptrbitmapsize = m->totalblocks*BPTRSIZE;
	action(f,"preadn 1");
	if(preadn(fd, d->ptrs, m->ptrmapstart, ptrbitmapsize) < ptrbitmapsize){
		action(f,"failure");
		d->blocks_cached = 0;
		outfn(f);
		return -1;
	}
	d->blocks_cached = 1;
	outfn(f);
	return 0;
}

int
syncbptrs(Disk *d)
{
	Fn *f = infn("syncbptrs");
	iqlock(f,&d->synclock);
	if(d->blocks_cached){
		for(u64int i = 0; i < d->meta->totalblocks; i++){
			writebptr(d,&d->ptrs[i]);
			if(&d->ptrs[i].buffer != nil){
				writeblock(d,&d->ptrs[i],&d->ptrs[i].buffer);
				free(&d->ptrs[i].buffer);
			}
		}
		iqunlock(f,&d->synclock);
		outfn(f);
		return 0;
	}
	iqunlock(f,&d->synclock);
	outfn(f);
	return -1;
}

u64int
readbptr(Disk *d, Blockptr *b, u64int block)
{
	Fn *f = infn("readbptr");
	if(d->blocks_cached){
		iqlock(f,&d->synclock);
		b = &d->ptrs[block];
		iqunlock(f,&d->synclock);
		outfn(f);
		return BPTRSIZE;
	} else {
		action(f,"preadn 1");
		u64int retval = preadn(d->fd, b, d->meta->ptrmapstart+(BPTRSIZE*block),BPTRSIZE);
		b->buffer = nil;
		outfn(f);
		return retval;
	}
}

u64int
writebptr(Disk *d, Blockptr *b)
{
	Fn *f = infn("writebptr");
	Metablock *m = d->meta;
	u64int blocksize = m->blocksize;
	u64int blockaddr = b->offset/blocksize;
	action(f,"pwriten 1 (retval)");
	outfn(f);
	return pwriten(d->fd,b,m->ptrmapstart+(BPTRSIZE*blockaddr),BPTRSIZE);
}

u64int
allocblock(Disk *d, Blockptr *b)
{
	Fn *f = infn("allocblock");
	if(d->blocks_cached){
		iqlock(f,&d->synclock);
		for(u64int i = 0; i < d->meta->totalblocks; i++){
			if(d->ptrs[i].used == 0){
				b = &d->ptrs[i];
				iqunlock(f,&d->synclock);
				outfn(f);
				return i+1;
			}
		}
		iqunlock(f,&d->synclock);
		action(f,"retval -1");
		outfn(f);
		return -1;
	} else {
		Blockptr *bf = malloc(sizeof(Blockptr));
		action(f,smprint("allocated %p",bf));
		for(u64int i = 0 ; i < d->meta->totalblocks; i++){
			preadn(d->fd,bf,d->meta->ptrmapstart+(i*BPTRSIZE),BPTRSIZE);
			if(bf->used == 0){
				b = bf;
				outfn(f);
				return i;
			}
		}
		action(f,smprint("freeing %p",bf));
		free(bf);
		b = nil;
		outfn(f);
		return 0;
	}
	action(f,"retval -1");
	outfn(f);
	return -1;
}

int
freeblock(Disk *d, Blockptr *b)
{
	Fn *f = infn("freeblock");
	if(d->blocks_cached){
		iqlock(f,&d->synclock);
		b->used = 0;
		iqunlock(f,&d->synclock);
		outfn(f);
		return 0;
	} else {
		b->used = 0;
		for(int i = 0; i < 10 ; i++){
			if(writebptr(d,b) >= BPTRSIZE){
				outfn(f);
				return 0;
			}
		}
	}
	action(f,"retval -1");
	outfn(f);
	return -1;		
}

u64int
readblock(Disk *d, Blockptr *b, void *buf)
{
	Fn *f = infn("readblock");
	int fd = d->fd;
	u64int bmapstart = d->meta->bitmapstart;
	u64int blocksize = d->meta->blocksize;
	u64int offset = b->offset+d->meta->blocksize;
	iqlock(f,&d->synclock);
	if(d->blocks_cached && b->buffer != nil){
		buf = b->buffer;
		iqunlock(f,&d->synclock);
		outfn(f);
		return blocksize;
	} else if (d->blocks_cached && b->buffer == nil){
		u64int retval = preadn(fd,b->buffer,offset,blocksize);
		buf = b->buffer;
		iqunlock(f,&d->synclock);
		outfn(f);
		return retval;
	} else {
		iqunlock(f,&d->synclock);
		outfn(f);
		return preadn(fd,buf,offset,blocksize);
	}
	outfn(f);
	return 0;
}

u64int
writeblock(Disk *d, Blockptr *b, void *buf)
{
	Fn *f = infn("writeblock");
	int fd = d->fd;
	int syncing = 0;
	if(canqlock(&d->synclock)){
		syncing = 1;
		iqlock(f,&d->synclock);
	}
	u64int bmapstart = d->meta->bitmapstart;
	u64int blocksize = d->meta->blocksize;
	u64int offset = b->offset+d->meta->blocksize;
	if(d->blocks_cached && !syncing){
		memcpy(buf,b->buffer,blocksize);
		iqunlock(f,&d->synclock);
		action(f,"return (not syncing)");
		outfn(f);
		return blocksize;
	}
	action(f,"return (syncing)");
	outfn(f);
	return pwriten(fd,buf,offset,blocksize);
}
