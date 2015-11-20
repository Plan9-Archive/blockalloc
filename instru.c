#include <u.h>
#include <libc.h>
#include <blockalloc.h>

/*
simple instrumentation library
This is basically the ftrace-lite library. It doesn't have
all the cool features or anything like that, but it kinda
does the same thing.
*/

int enable_instru = 0;

/*
if using ftrace, run this *very* first.  infn calls started before
this will not start logging until the next time the function gets
called.
*/

void
initinstru(void)
{
	enable_instru = 1;
}

Fn* 
infn(char* name)
{
	Fn *bf = mallocz(sizeof(Fn),1);
	if(enable_instru){
		bf->active = 1;
		lock(&bf->l);
		bf->name = name;
		fprint(2,"instru: infn: %s\n",name);
		return bf;
	}
	free(bf);
	return nil;
}

void
action(Fn* f, char* c)
{
	if(f == nil || enable_instru == 0){
		return;
	} else if (f->active == 1 && !canlock(&f->l)){
		fprint(2,"instru: %s: %s\n",f->name,c);
	} else if (canlock(&f->l)) {
		fprint(2,"instru: error: action %s done outside of function\n",c);
	}
}

void
shit(Fn* f, char *c)
{
	if(f == nil || enable_instru == 0){
		return;
	} else if (f->active == 1 && !canlock(&f->l)){
		fprint(2,"instru: error: %s: %s (explicit)\n",f->name,c);
	} else if (canlock(&f->l)) {
		fprint(2,"instru: error: action %s done outside of function (automatic)\n",c);
	}
}

void
upanic(Fn* f, char *c)
{
	char *msg;
	if(f == nil || enable_instru == 0)
		return;
	msg = smprint("instru: PANIC: %s: %s [pid %d]",f->name,c,getpid());
	if (f->active == 1 && !canlock(&f->l)){
		fprint(2,"%s\n",msg);
		exits(msg);
		return;
	} else {
		fprint(2,"%s (panic outside function calling)\n",msg);
		exits(smprint("%s (panic outside function calling)",msg));
		return;
	}
}

void
outfn(Fn* f)
{
	if(f == nil || enable_instru == 0){
		return;
	} else if (f->active == 1 && !canlock(&f->l)){
		fprint(2,"instru: outfn: %s\n",f->name);
		unlock(&f->l);
		free(f);
		return;
	} else {
		if(f != nil){
			fprint(2,"instru: error: outfn done outside of function\n");
			if(!canlock(&f->l))
				unlock(&f->l);
			free(f);
			return;
		}
		fprint(2,"instru: error: something bad happened\n");
	}
}

void
iqlock(Fn* f, QLock* l)
{
	if(f == nil || enable_instru == 0){
		qlock(l);
	} else if (f->active == 1 && !canlock(&f->l)){
		qlock(l);
		fprint(2,"instru: %s: lock: %p\n",f->name,l);
	}
}

void
iqunlock(Fn* f,QLock* l)
{
	if(f == nil || enable_instru == 0){
		qlock(l);
	} else if (f->active == 1 && !canlock(&f->l)){
		qlock(l);
		fprint(2,"instru: %s: unlock: %p\n",f->name,l);
	}
}
