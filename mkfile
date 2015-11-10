</$objtype/mkfile

LIB=/$objtype/lib/libblockalloc.a

OFILES=\
	disk.$O \
	pointers.$O \
	instru.$O \

CFILES=\
	disk.c \
	pointers.c \
	instru.c \

HFILES=\
	blockalloc.h \

UPDATE=\
	mkfile \
	$LIB \
	$CFILES \
	$HFILES \

all:V: $LIB

</sys/src/cmd/mksyslib
