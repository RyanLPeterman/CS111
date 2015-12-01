CC	= gcc
CFLAGS	= -g -W -Wall -Werror -Wno-unused
V	= @
LIBS	= -lpthread

# Uncomment the following line to run on Solaris machines.
#LIBS	+= -lsocket -lnsl -lresolv

all: osppeer

%.o: %.c
	@echo + cc $<
	$(V)$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

run: osppeer
	@-/bin/rm -rf test
	@echo + mkdir test
	@mkdir test
	@echo + ./osppeer -dtest -t12997 cat1.jpg cat2.jpg cat3.jpg
	@./osppeer -dtest -t12997 cat1.jpg cat2.jpg cat3.jpg

run-good: osppeer
	@-/bin/rm -rf test
	@echo + mkdir test
	@mkdir test
	@echo + ./osppeer -dtest -t12998 cat1.jpg cat2.jpg cat3.jpg
	@./osppeer -dtest -t12998 cat1.jpg cat2.jpg cat3.jpg

run-slow: osppeer
	@-/bin/rm -rf test
	@echo + mkdir test
	@mkdir test
	@echo + ./osppeer -dtest -t12999 cat1.jpg cat2.jpg cat3.jpg
	@./osppeer -dtest -t12999 cat1.jpg cat2.jpg cat3.jpg

run-popular: osppeer
	@-/bin/rm -rf test
	@echo + mkdir test
	@mkdir test
	@echo + ./osppeer -dtest -t13000 cat1.jpg cat2.jpg cat3.jpg
	@./osppeer -dtest -t13000 cat1.jpg cat2.jpg cat3.jpg

run-bad: osppeer
	@-/bin/rm -rf test
	@echo + mkdir test
	@mkdir test
	@echo + ./osppeer -dtest -t13001 cat1.jpg cat2.jpg cat3.jpg
	@./osppeer -dtest -t13001 cat1.jpg cat2.jpg cat3.jpg
	
clean:
	-rm -f *.o *~ osptracker osptracker.cc osppeer

distclean: clean

DISTDIR := lab4-$(USER)

tarballdir-nocheck: clean always
	@echo + mk $(DISTDIR)
	$(V)/bin/rm -rf $(DISTDIR)
	$(V)mkdir $(DISTDIR)
	$(V)tar cf - `ls | grep -v '^$(DISTDIR)\|^test\|^lab4\|\.tar\.gz$$\|\.tgz$$\|\.qcow2$$\|~$$'` | (cd $(DISTDIR) && tar xf -)
	$(V)/bin/rm -rf `find $(DISTDIR) -name CVS -o -name .svn -print`
	$(V)date > $(DISTDIR)/tarballstamp

tarballdir: tarballdir-nocheck
	$(V)/bin/bash ./check-lab.sh $(DISTDIR) || (rm -rf $(DISTDIR); false)

tarball: tarballdir
	@echo + mk $(DISTDIR).tar.gz
	$(V)tar cf $(DISTDIR).tar $(DISTDIR)
	$(V)gzip $(DISTDIR).tar
	$(V)/bin/rm -rf $(DISTDIR)

osppeer: osppeer.o md5.o writescan.o
	@echo + ld osppeer
	$(V)$(CC) $(CFLAGS) -o $@ osppeer.o md5.o writescan.o $(LIBS)


.PHONY: all always clean distclean tarball tarballdir-nocheck tarballdir \
	dist distdir install run run-good run-slow run-bad
