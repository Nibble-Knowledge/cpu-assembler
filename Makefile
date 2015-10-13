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

#docs:
#	rm -f docs.html
#	doxygen
#	ln -s html/index.html docs.html

clean:
	$(MAKE) -C src clean
	rm -rf as4

phony: 
	true
