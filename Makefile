EXE=as4
SRCDIR=src
all: fast


fast: phony
	$(MAKE) -C $(SRCDIR) fast
	cp $(SRCDIR)/$(EXE) .

debug: phony
	$(MAKE) -C src
	cp $(SRCDIR)/$(EXE) .

distw32: phony
	$(MAKE) -C $(SRCDIR) distw32
	cp $(SRCDIR)/$(EXE) .

clean:
	$(MAKE) -C $(SRCDIR) clean
	rm -rf $(EXE) gmon.out

phony: 
	true
