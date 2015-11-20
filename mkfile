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
	tests.log \

TESTS=\
	ftrace-test \

all:V: $LIB

ftrace-test:
	$CC tests/ftrace-test.c
	$LD -o ftrace-test ftrace-test.$O

installheaders:
	for(i in $HFILES){
		cp $i /sys/include
	}

run-tests:EVQ: ftrace-test
	echo 'running tests...'
	{
		echo -n 'tests started at '
		date
		echo '-----------------------------------------------------'
		for(i in $TESTS){
			echo 'running test: '^$i
			$i >[2=1]
			echo '-----------------------------------------------------'
		}
		echo 'tests done.'
		} > tests.log

</sys/src/cmd/mksyslib

nuke:V:
	rm -f *.[$OS] [$OS].out $CLEANFILES $LIB

