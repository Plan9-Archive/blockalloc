#include <u.h>
#include <libc.h>
#include <blockalloc.h>

#define LIKE50MB 52428800

int createtestfile(void);
int sproc(char*,char*[]);
int crucify(int);

int disksimpid;

void
main(int argc, char *argv[])
{
	initinstru();
	Fn *f = infn("main");
	action(f,"creating virtual sd device sdTT");
	int fd = createtestfile();
	if(fd<0){
//		crucify(disksimpid);
		upanic(f,smprint("unable to open device /dev/sdTT/data. errstr = %r"));
	}
	action(f,"reaming with blocksize 1024");
	int retval = ream(fd,1024,0);
	if(retval == -2)
		upanic(f,smprint("ream short read errstr = %r"));
	action(f,smprint("ream retval = %d",retval));
	Disk *d;
	action(f,smprint("d(ptr) = %p",d));
	retval = fd2disk(fd,d,0);
	action(f,smprint("fd2disk retval = %d",retval));
	action(f,smprint("d(ptr) = %p",d));
	u64int blocksize = d->meta->blocksize;
	u64int ptrmapstart = d->meta->ptrmapstart;
	u64int totalblocks = d->meta->totalblocks;
	u64int bitmapstart = d->meta->bitmapstart;
	u64int dsize = d->size;
	action(f,smprint("blocksize = %d",d->meta->blocksize));
	action(f,smprint("ptrmapstart = %d",d->meta->ptrmapstart));
	action(f,smprint("totalblocks = %d",d->meta->totalblocks));
	action(f,smprint("bitmapstart = %d",d->meta->bitmapstart));
	action(f,smprint("disk size = %d",d->size));
	close(fd);
	crucify(disksimpid);
	action(f,"done");
	outfn(f);
	exits(nil);
}

int
createtestfile(void)
{
	Fn *f = infn("createtestfile");
	action(f,"running disksim");
//	char *dsargs[] = { "aux/disksim","sdTT",0 };
//	disksimpid = sproc("/bin/aux/disksim",dsargs);
//	sleep(2);
//	int fd = open("/dev/sdTT/ctl",OWRITE|OREAD);
//	if(fd<0)
//		upanic(f,"could not open /dev/sdTT/ctl");
//	fprint(fd,"geometry %d 512 0 0 0",LIKE50MB*2);
//	close(fd);
	outfn(f);
	return open("/n/ram/testdisk.img",OWRITE|OREAD);
}

int
sproc(char *execf, char *args[])
{
	Fn *f = infn("sproc");
	int pid;
	if((pid = fork()) == 0){
		Fn *f2 = infn(smprint("sproc [pid %d]",getpid()));
		action(f2,smprint("calling %s",execf));
		int x = exec(execf,args);
		char *s = malloc(sizeof(char)*1024);
		if(s == nil)
			upanic(f2,"unable to malloc");
		errstr(s,1024);
		upanic(f2,smprint("unable to exec. retval = %d, errstr = %s",x,s));
	}
	outfn(f);
	return pid;
}

int
crucify(int pid)
{
	Fn *f = infn("crucify");
	int fd = open(smprint("/proc/%d/ctl"),OWRITE);
	if(fd<0)
		return -1;
	fprint(fd,"kill\n");
	close(fd);
	action(f,smprint("the process %d met his destiny.",pid));
	outfn(f);
	return 0;
}
