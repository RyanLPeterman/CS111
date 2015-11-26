# Comment this line to get more verbose messages from this Makefile
V=@

# Path to the Linux kernel, include files, etc.
#
# KERNELRELEASE is the target kernel's version.  If KERNELRELEASE
# is not set in the environment then it is taken from the running
# system.
#
# KERNELPATH is the path to the target kernel's build/source area.
# This is used to obtain the kernel configuration and include files.
# If KERNELPATH is not set in the environment then it is derived
# from KERNELRELEASE.
#
ifeq ($(KERNELRELEASE),)
KERNELRELEASE =	$(shell uname -r)
endif

ifeq ($(KERNELPATH),)
KERNELPATH=	/lib/modules/${KERNELRELEASE}/build
endif

ifeq ($(DESTDIR),)
DESTDIR=
endif

ifeq ($(MODULEPATH),)
MODULEPATH=	/lib/modules/${KERNELRELEASE}/fs
endif

obj-m		+= ospfs.o
ospfs-objs	:= ospfsmod.o fsimg.o
BASEFILES	:= $(shell find base 2>/dev/null | grep -v '[ 	]')

ospfs.ko all: fsimg.c truncate always
	$(MAKE) -C $(KERNELPATH) M=$(shell pwd) modules

install: ospfs.ko
	$(MAKE) -C $(KERNELPATH) M=$(shell pwd) modules_install

fsimg.c: fs.img fsimgtoc
	./fsimgtoc fs.img fsimg.c

fs.img: ospfsformat Makefile $(BASEFILES)
	./ospfsformat -l hello.txt:link -c $@ 4096 128 -r base

ospfsformat: ospfsformat.c md5.c ospfs.h md5.h
	$(CC) -g -c md5.c -o md5.o
	$(CC) -g -c ospfsformat.c -o ospfsformat.o
	$(CC) -g md5.o ospfsformat.o -o $@

fsimgtoc: fsimgtoc.c
	$(CC) $< -o $@

truncate: truncate.c
	$(CC) $< -o $@

DISTDIR := lab3-$(USER)
ifeq ($(SOL),1)
DISTDIR := sol3
endif

tarball: realclean
	@echo + mk $(DISTDIR).tar.gz
	$(V)mkdir $(DISTDIR)
	$(V)tar cf - `ls | grep -v '^$(DISTDIR)\|.*\.qvm$$\|.*\.qcow2$$\|.*\.tar\.gz$$\|.*\.tar\.bz2$$\|^\.svn\|^\.git\|^CVS\|.*\.iso$$\|^binary$$\|^cache$$\|^chroot$$\|^config$$\|^\.stage$$'` | (cd $(DISTDIR) && tar xf -)
	$(V)/bin/rm -rf `find $(DISTDIR) -name CVS -o -name .svn -print`
	$(V)date > $(DISTDIR)/tarballstamp
	$(V)/bin/bash ./check-lab.sh $(DISTDIR) || (rm -rf $(DISTDIR); false)
	$(V)tar cf $(DISTDIR).tar $(DISTDIR)
	$(V)gzip $(DISTDIR).tar
	$(V)/bin/rm -rf $(DISTDIR)

tarball-nocheck: realclean
	@echo + mk $(DISTDIR).tar.gz
	$(V)mkdir $(DISTDIR)
	$(V)tar cf - `ls | grep -v '^$(DISTDIR)\|.*\.qvm$$\|.*\.qcow2$$\|.*\.tar\.gz$$\|.*\.tar\.bz2$$\|^\.svn\|^\.git\|^CVS\|.*\.iso$$\|^binary$$\|^cache$$\|^chroot$$\|^config$$\|^\.stage$$'` | (cd $(DISTDIR) && tar xf -)
	$(V)/bin/rm -rf `find $(DISTDIR) -name CVS -o -name .svn -print`
	$(V)date > $(DISTDIR)/tarballstamp
	$(V)tar cf $(DISTDIR).tar $(DISTDIR)
	$(V)gzip $(DISTDIR).tar
	$(V)/bin/rm -rf $(DISTDIR)

clean:
	@echo + clean
	$(V)-rm -f fs.img fsimg.c fsimgtoc ospfsformat truncate *.o *.ko *.mod.c
	$(V)-rm -f .version .*.o.flags .*.o.d .*.o.cmd .*.ko.cmd
	$(V)-rm -rf .tmp_versions

realclean: clean
	@echo + realclean
	$(V)-rm -f write_clean
	$(V)-rm -rf $(DISTDIR) $(DISTDIR).tar.gz labstuff.tgz

.PHONY: all always clean distclean distdir dist tarball install
