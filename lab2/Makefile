# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y

# Comment this line to get more verbose messages from this Makefile
V=@

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DOSPRD_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += -I..

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m	:= osprd.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

default: osprdaccess
	$(MAKE) osprdaccess
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif



clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions osprdaccess

check:
	perl lab2-tester.pl

depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif

DISTDIR = lab2-$(USER)

tarball: realclean
	@echo + mk $(DISTDIR).tar.gz
	$(V)mkdir $(DISTDIR)
	$(V)tar cf - `ls | grep -v '^$(DISTDIR)\|.*\.qvm\|.*\.tar\.gz\|^\.svn\|^CVS\|.*\.iso\|^binary$$\|^cache$$\|^chroot$$\|^config$$\|^\.stage$$'` | (cd $(DISTDIR) && tar xf -)
	$(V)/bin/rm -rf `find $(DISTDIR) -name CVS -o -name .svn -print`
	$(V)date > $(DISTDIR)/tarballstamp
	$(V)tar cf $(DISTDIR).tar $(DISTDIR)
	$(V)gzip $(DISTDIR).tar
	$(V)/bin/rm -rf $(DISTDIR)

realclean: clean
	@echo + realclean
	$(V)rm -f write_clean
	$(V)rm -rf $(DISTDIR) $(DISTDIR).tar.gz

.PHONY: clean realclean tarball export dep depend default check
