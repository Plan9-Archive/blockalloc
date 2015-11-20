</$objtype/mkfile

LIB=/$objtype/lib/libblockalloc.a

OFILES=\
	disk.$O \
	pointers.$O \
	instru.$O \
	io.$O \
	ream.$O \

CFILES=\
	disk.c \
	pointers.c \
	instru.c \
	io.c \
	ream.c \

HFILES=\
	blockalloc.h \

UPDATE=\
	mkfile \
	$LIB \
	$CFILES \
	$HFILES \

CLEANFILES=\
	ftrace-test \

ftrace-test: all
	$CC tests/ftrace-test.c
	$LD -o ftrace-test ftrace-test.$O

installheaders:
	for(i in $HFILES){
		cp $i /sys/include
	}

</sys/src/cmd/mksyslib

nuke:V:
	rm -f *.[$OS] [$OS].out $CLEANFILES $LIB

all:V: $LIB
