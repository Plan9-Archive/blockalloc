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
typedef struct BlockptrODF BlockptrODF;
typedef struct Metablock Metablock;
typedef struct Disk Disk;
typedef struct Fn Fn;

struct Fn {
	char *name;
	Lock l;
	uint active;
};

struct Blockptr {
	uchar used; // is the block in use?
	uchar notcurrent; // is the block current?
	u64int offset; // where is it from the start of the bitmap?
	u64int next; // if the block is not current, this points to the next possible current block
	void *buffer; // buffer for sync if blocks are cached
};

struct BlockptrODF {  // on-disk format of a block pointer. contains currently unused fields.
	uchar used;
	uchar notcurrent;
	u64int offset;
	u64int next;
};

struct Metablock { // information about the managed disk
	u64int blocksize; // how big is a block in the bitmap
	u64int ptrmapstart; // where are the pointers?
	u64int totalblocks; // how many blocks are there?
	u64int bitmapstart; // where are the blocks?
};
	
struct Disk {
	int fd; // rw channel
	u64int size; // size of the disk
	Metablock *meta; // the metadata block
	uint blocks_cached; // are the blocks cached?
	int syncpid;
	int debug;
	QLock syncing;
	QLock synclock;
	QLock writelock;
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
void initinstru(void);
Fn* infn(char*);
void action(Fn*,char*);
void shit(Fn*,char*);
void upanic(Fn*,char*);
void outfn(Fn*);
void iqlock(Fn*,QLock*);
void iqunlock(Fn*,QLock*);

/* io.c */
// device, buffer, length, offset
u64int dread(Disk*,void*,u64int,u64int);
u64int dwrite(Disk*,void*,u64int,u64int);

/* ream.c */
int ream(int,u64int);
int ream1(int,u64int,u64int,u64int,u64int);
