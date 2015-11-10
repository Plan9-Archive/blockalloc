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

void
initinstru(void)
{
	enable_instru = 1;
}

Fn* 
infn(char* name)
{
	Fn *bf = malloc(sizeof(Fn));
	if(enable_instru){
		bf->active = 1;
		qlock(&bf->l);
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
	} else if (f->active == 1 && !canqlock(&f->l)){
		fprint(2,"instru: %s: %s\n",f->name,c);
	} else if (canqlock(&f->l)) {
		fprint(2,"instru: error: action %s done outside of function\n",c);
	}
}

void
outfn(Fn* f)
{
	if(f == nil || enable_instru == 0){
		return;
	} else if (f->active == 1 && !canqlock(&f->l)){
		fprint(2,"instru: outfn: %s\n",f->name);
		qunlock(&f->l);
		free(f->name);
		free(f);
		return;
	} else {
		if(f != nil){
			fprint(2,"instru: error: outfn done outside of function\n");
			if(!canqlock(&f->l))
				qunlock(&f->l);
			free(f->name);
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
	} else if (f->active == 1 && !canqlock(&f->l)){
		qlock(l);
		fprint(2,"instru: %s: lock: %p\n",f->name,l);
	}
}

void
iqunlock(Fn* f,QLock* l)
{
	if(f == nil || enable_instru == 0){
		qlock(l);
	} else if (f->active == 1 && !canqlock(&f->l)){
		qlock(l);
		fprint(2,"instru: %s: unlock: %p\n",f->name,l);
	}
}
