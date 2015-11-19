#include <u.h>
#include <libc.h>
#include <blockalloc.h>

int
ream(int fd, u64int blocksize)
{
	Fn *f = infn("ream");
	u64int disksize = seek(fd,0,2); seek(fd,0,0);
	u64int ptrmapstart = blocksize*2;
	u64int bmapsize = (blocksize*(disksize-blocksize))/(sizeof(BlockptrODF)+blocksize);
	u64int ptrmaplen = (bmapsize/blocksize)*sizeof(BlockptrODF);
	u64int bmapstart = ptrmaplen+sizeof(BlockptrODF);
	u64int totalblocks = bmapsize/blocksize;
	int retval = ream1(fd,blocksize,ptrmapstart,bmapstart,totalblocks);
	action(f,smprint("retval = %d",retval));
	if(retval == 0){
		action(f,smprint("disksize = %d bytes",disksize));
		action(f,smprint("ptrmapstart = %x",ptrmapstart));
		action(f,smprint("ptrmaplen = %d bytes",ptrmaplen));
		action(f,smprint("bmapstart = %x",bmapstart));
		action(f,smprint("bmapsize = %d bytes",bmapsize));
		action(f,smprint("totalblocks = %d of %d byte blocks",totalblocks,blocksize));
	} else {
		switch(retval){
		case -1:
			shit(f,"non-fatal error reaming. (code 1)");
			break;
		default:
			shit(f,smprint("fatal error reaming."));
			char *err = mallocz(1024*sizeof(char),0);
			errstr(err,1024);
			upanic(f,smprint("retval = %d, errstr = %s",retval,err));
			break;
		}
	}
	outfn(f);
	return retval;
}

int
ream1(int fd, u64int blocksize, u64int ptrmapstart, u64int bmapstart, u64int totalblocks)
{
	Fn *f = infn("ream1");
	Metablock *m = mallocz(sizeof(Metablock),0);
	BlockptrODF *blocks;
	if(m == nil){
		shit(f,"malloc failed");
		outfn(f);
		return -2;
	}
	m->blocksize = blocksize;
	m->ptrmapstart = ptrmapstart;
	m->totalblocks = totalblocks;
	m->bitmapstart = bmapstart;
	// sanity checking
	if(((bmapstart-ptrmapstart)/sizeof(BlockptrODF)) >= totalblocks){
		shit(f,smprint("ptrmap not large enough (failed: %x >= %x)",
						((bmapstart-ptrmapstart)/sizeof(BlockptrODF)),
						totalblocks));
		outfn(f);
		return -3;
	} else if (blocksize <= sizeof(BlockptrODF)){
		shit(f,smprint("blocksize too small (blockptr)"));
		outfn(f);
		return -4;
	} else if (blocksize <= sizeof(Metablock)){
		shit(f,smprint("blocksize too small (metablock)"));
		outfn(f);
		return -5;
	}
	u64int written = pwriten(fd,m,0,sizeof(Metablock));
	if(written < sizeof(Metablock)){
		shit(f,smprint("whole metablock not written"));
		outfn(f);
		return -6;
	}
	blocks = mallocz(totalblocks*sizeof(BlockptrODF),0);
	if(blocks == nil){
		upanic(f,"malloc");
		outfn(f);
		return -7;
	}
	action(f,"formatting blocks");
	for(u64int i = 0; i<totalblocks; i++){
		blocks[i].used = 0;
		blocks[i].offset = blocksize*i;
	}
	written = pwriten(fd,blocks,ptrmapstart,sizeof(BlockptrODF)*totalblocks);
	if(written < (sizeof(BlockptrODF)*totalblocks)){
		shit(f,"all blocksptrs not written");
		outfn(f);
		return -8;
	}
	action(f,"ream done. cleaning up");
	free(m);
	free(blocks);
	return 0;
}
