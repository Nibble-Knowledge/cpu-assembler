SRC=as4.c
EXE=as4
EXTRACFLAGS=
EXTRALDFLAGS=
export SRC
export EXE
export EXTRACFLAGS
export EXTRALDFLAGS

all: fast


fast: phony
	$(MAKE) -C src fast
	cp src/as4 .

debug: phony
	$(MAKE) -C src
	cp src/as4 .

clean:
	$(MAKE) -C src clean
	rm -rf as4 gmon.out

phony: 
	true
