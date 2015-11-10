#pragma lib "libblockalloc.a"

#define BPTRSIZE 9  // this is bytes
#define SYNCSLEEP 300 // in seconds

enum {
	DISKCACHE = 1,
	DISKAUTOSYNC = 2,
	DISKCONSTSYNC = 4,
	DISKDEBUG = 8,
	DISKMAN = 3,
};

typedef struct Blockptr Blockptr;
typedef struct Metablock Metablock;
typedef struct Disk Disk;
typedef struct Fn Fn;

struct Fn {
	char *name;
	QLock l;
	uint active;
};

struct Blockptr {
	uchar used; // is the block in use?
	u64int offset; // where is it from the start of the bitmap?
	void *buffer; // buffer for sync if blocks are cached
};

struct Metablock { // information about the managed disk
	u64int blocksize; // how big is a block in the bitmap
	u64int ptrmapstart; // where are the pointers?
	u64int totalblocks; // how many blocks are there?
	u64int bitmapstart; // where are the blocks?
};
	
struct Disk {
	int fd; // rw channel
	Metablock *meta; // the metadata block
	uint blocks_cached; // are the blocks cached?
	int syncpid;
	int debug;
	QLock syncing;
	QLock synclock;
	Blockptr *ptrs; // blockpointers if they're cached
};

/* pointers.c */
u64int preadn(int,void*,u64int,u64int); // pread + readn
u64int pwriten(int, void*,u64int,u64int); // wrapper around pwrite to make it match preadn
int loadbptrs(Disk*);
int syncbptrs(Disk*);
u64int readbptr(Disk*, Blockptr*, u64int);
u64int writebptr(Disk*, Blockptr*);
u64int allocblock(Disk*,Blockptr*);
int freeblock(Disk*,Blockptr*);
u64int readblock(Disk*,Blockptr*,void*);
u64int writeblock(Disk*,Blockptr*,void*);

/* disks.c */
int opendisk(char*,Disk*,uchar);
int closedisk(Disk*);
int fd2disk(int,Disk*,uchar);
int disksyncer(Disk*);

/* instru.c */
void enableinstru(void);
Fn* infn(char*);
void action(Fn*,char*);
void outfn(Fn*);
void iqlock(Fn*,QLock*);
void iqunlock(Fn*,QLock*);
