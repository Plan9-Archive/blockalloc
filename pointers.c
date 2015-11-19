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
	iqlock(f,&d->writelock);
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
		/* TODO: make a Blockptr odf type and use that instead of the Blockptr type directly.
				 In theory, using the Blockptr type directly without a mallocz could possibly
				 cause some issues if the user doesn't know whether caching is on or not.
				 Using mallocz has performance implications and I should probably avoid it.
		*/
		u64int retval = preadn(d->fd, b, d->meta->ptrmapstart+(BPTRSIZE*block),BPTRSIZE);
		b->buffer = nil;
		outfn(f);
		return retval;
	}
}

/*
writebptr should basically only be used by sync or in cases when
you're using the block pointer rewriting capabilities of blockalloc to
do copy-on-write of data blocks or some shit.  writebptr *always* does
a write(2) to the disk where as readbptr uses the in-memory structures
instead of the on-disk structures.  Also if you're modifying
readbptr'd Blockptrs and the cache is available then you *are*
modifying the on-disk Blockptrs.  Remember, all of these functions are
probably dangerous; think carefully and don't do stupid things with
other people's data.
*/
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
	u64int blocksize = d->meta->blocksize;
	u64int offset = b->offset+d->meta->blocksize+d->meta->bitmapstart;
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
}

u64int
writeblock(Disk *d, Blockptr *b, void *buf)
{
	Fn *f= infn("writeblock");
	int fd = d->fd;
	if(canqlock(&d->synclock) && d->blocks_cached){
		iqlock(f,&d->writelock);
		memcpy(b->buffer,buf,d->meta->blocksize);
		action(f,"return (not writing, blocks cached, no sync)");
		iqunlock(f,&d->writelock);
		outfn(f);
		return d->meta->blocksize;
	} else if (!canqlock(&d->synclock) && d->blocks_cached) {
		action(f,"return (writing, blocks cached, syncing)");
		outfn(f);
		return pwriten(fd,buf,b->offset+d->meta->bitmapstart,d->meta->blocksize);
	} else {
		action(f,"return (writing, blocks not cached, not syncing)");
		outfn(f);
		return pwriten(fd,buf,b->offset+d->meta->bitmapstart,d->meta->blocksize);
	}
}
