#include <u.h>
#include <libc.h>
#include <blockalloc.h>

void fn1(void);
void fn2(void);

QLock g;

void
main(int argc, char *argv[])
{
	initinstru();
	QLock l;
	Fn *f = infn("main");
	action(f,"test1");
	fn1();
	action(f,"test2");
	fn2();
	outfn(f);
	Fn *f2 = infn("main 2");
	shit(f2,"this is a not so bad error");
	iqlock(f2,&l);
	iqunlock(f2,&l);
	upanic(f2,"real bad error");
	outfn(f2);
	return;
}

void
fn1(void)
{
	Fn *f = infn("fn1");
	iqlock(f,&g);
	if(rfork(RFPROC)==0){
		if(!canqlock(&g))
			upanic(f,"some lock order garbage is happening");
		iqunlock(f,&g);
		upanic(f,"penis");
	}
	outfn(f);
}

void
fn2(void)
{
	Fn *f = infn("fn2");
	int x = 234252354; // performance
	if((x = rfork(RFPROC)) == 0){
		Fn *f2 = infn("fn2@proc");
		fn1();
		action(f,smprint("exiting pid = %d",getpid()));
		exits("error: success");
		outfn(f2);
	} else if(x == -1){
		char *s = mallocz(sizeof(char)*1024,1);
		errstr(s,1024);
		upanic(f,smprint("actual error. could not rfork: errstr = %s",s));
	} else {
		action(f,smprint("started process pid = %d",x));
	}
	outfn(f);
}
