#include <u.h>
#include <libc.h>
#include <blockalloc.h>

void
disksyncer1(Disk *d)
{
	Fn *f = infn("disk syncer");
	for(;;){
		sleep(SYNCSLEEP*1000);
		iqlock(f,&d->syncing);
		if(!syncbptrs(d))
			if(d->debug)
				print("blockalloc: error syncing block pointers\n");
		iqunlock(f,&d->syncing);
	}
}

int
disksyncer(Disk* d)
{
	int pid;
	if((pid = rfork(RFPROC|RFMEM|RFNOWAIT)) == 0){
		disksyncer1(d);
	} else if (pid > 0) {
		d->syncpid = pid;
		return 0;
	} else {
		return -1;
	}
	return -1;
}

int
opendisk(char* fname, Disk* d, uchar opts)
{
	int fd;
	if((fd = open(fname,OREAD|OWRITE)) < 0){
		d = nil;
		return -1;
	}
	return fd2disk(fd,d,opts);
}

int
closedisk(Disk* d)
{
	if(d->blocks_cached){
		qlock(&d->syncing);
		syncbptrs(d);
		if(postnote(PNPROC,d->syncpid,"fucking die you cunt") == 0){
			free(d->ptrs);
			free(d->meta);
			close(d->fd);
			qunlock(&d->syncing);
			free(d);
			return 0;
		} else {
			print("blockalloc: unable to kill syncer process %d\n",d->syncpid);
			free(d->ptrs);
			free(d->meta);
			close(d->fd);
			return -1;
		}
	} else {
		if(d->ptrs != nil)
			free(d->ptrs);
		free(d->meta);
		close(d->fd);
		free(d);
		return 0;
	}
	return -2;
}

int
fd2disk(int fd, Disk* d, uchar opts)
{
	Fn *f = infn("fd2disk");
	Disk *bf = malloc(sizeof(Disk));
	Metablock *m = malloc(sizeof(Metablock));
	bf->fd = fd;
	action(f,"preadn 1");
	if((preadn(bf->fd,m,2,sizeof(Metablock))) < sizeof(Metablock)){
		action(f,"short read");
		free(m);
		close(bf->fd);
		free(bf);
		d = nil;
		outfn(f);
		return -2;
	}
	bf->meta = m;
	if(opts == 1 || opts%2 == 1){
		action(f,"enabling disk caching");
		bf->blocks_cached = 1;
		if(loadbptrs(bf) == 0){
			if(disksyncer(bf) == 0){
				d = bf;
				outfn(f);
				return 0;
			}
			action(f,"syncer did not start (rfork error)");
			free(d->ptrs);
			bf->blocks_cached = 0;
			d = bf;
			outfn(f);
			return -3;
		}
		action(f,"failure to enable caching");
	}
	d = bf;
	outfn(f);
	return 0;
}

