
SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .
topdir = /home/nick/.rvm/rubies/ruby-1.8.7-p358/lib/ruby/1.8/i686-linux
hdrdir = $(topdir)
VPATH = $(srcdir):$(topdir):$(hdrdir)
exec_prefix = $(prefix)
prefix = $(DESTDIR)/home/nick/.rvm/rubies/ruby-1.8.7-p358
vendordir = $(libdir)/ruby/vendor_ruby
datarootdir = $(prefix)/share
docdir = $(datarootdir)/doc/$(PACKAGE)
sitelibdir = $(sitedir)/$(ruby_version)
localstatedir = $(prefix)/var
mandir = $(datarootdir)/man
htmldir = $(docdir)
archdir = $(rubylibdir)/$(arch)
sharedstatedir = $(prefix)/com
sysconfdir = $(prefix)/etc
sitearchdir = $(sitelibdir)/$(sitearch)
pdfdir = $(docdir)
bindir = $(exec_prefix)/bin
vendorlibdir = $(vendordir)/$(ruby_version)
datadir = $(datarootdir)
vendorarchdir = $(vendorlibdir)/$(sitearch)
libexecdir = $(exec_prefix)/libexec
localedir = $(datarootdir)/locale
includedir = $(prefix)/include
dvidir = $(docdir)
psdir = $(docdir)
oldincludedir = $(DESTDIR)/usr/include
sbindir = $(exec_prefix)/sbin
infodir = $(datarootdir)/info
rubylibdir = $(libdir)/ruby/$(ruby_version)
libdir = $(exec_prefix)/lib
sitedir = $(libdir)/ruby/site_ruby

CC = gcc
LIBRUBY = $(LIBRUBY_SO)
LIBRUBY_A = lib$(RUBY_SO_NAME)-static.a
LIBRUBYARG_SHARED = -Wl,-R -Wl,$(libdir) -L$(libdir) -l$(RUBY_SO_NAME)
LIBRUBYARG_STATIC = -l$(RUBY_SO_NAME)-static

RUBY_EXTCONF_H = 
CFLAGS   =  -fPIC -g -std=gnu99 -D_GNU_SOURCE -fno-strict-aliasing -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith 
INCFLAGS = -I. -I. -I/home/nick/.rvm/rubies/ruby-1.8.7-p358/lib/ruby/1.8/i686-linux -I.
DEFS     = -D_FILE_OFFSET_BITS=64
CPPFLAGS =  -I/home/nick/trema_apps_combined/src/lib -I/home/nick/trema_apps_combined/objects/openflow -I/home/nick/trema_apps_combined/apps/topology  -D_FILE_OFFSET_BITS=64 
CXXFLAGS = $(CFLAGS) 
ldflags  = -Wl,-Bsymbolic
dldflags = 
archflag = 
DLDFLAGS = $(ldflags) $(dldflags) $(archflag)
LDSHARED = $(CC) -shared
AR = ar
EXEEXT = 

RUBY_INSTALL_NAME = ruby
RUBY_SO_NAME = ruby
arch = i686-linux
sitearch = i686-linux
ruby_version = 1.8
ruby = /home/nick/.rvm/rubies/ruby-1.8.7-p358/bin/ruby
RUBY = $(ruby)
RM = rm -f
MAKEDIRS = mkdir -p
INSTALL = /usr/bin/install -c
INSTALL_PROG = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 644
COPY = cp

#### End of system configuration section. ####

preload = 

libpath = . $(libdir) /home/nick/trema_apps_combined/objects/lib
LIBPATH =  -L. -L$(libdir) -Wl,-R$(libdir) -L/home/nick/trema_apps_combined/objects/lib -Wl,-R/home/nick/trema_apps_combined/objects/lib
DEFFILE = 

CLEANFILES = mkmf.log
DISTCLEANFILES = $(SRCS)

extout = 
extout_prefix = 
target_prefix = 
LOCAL_LIBS = 
LIBS = $(LIBRUBYARG_SHARED) -ltrema -lsqlite3 -lrt -lpthread  -lrt -ldl -lcrypt -lm   -lc
SRCS = libtopology.c topology_service_interface_option_parser.c topology-client.c
OBJS = libtopology.o topology_service_interface_option_parser.o topology-client.o
TARGET = topology_client
DLLIB = $(TARGET).so
EXTSTATIC = 
STATIC_LIB = 

BINDIR        = $(bindir)
RUBYCOMMONDIR = $(sitedir)$(target_prefix)
RUBYLIBDIR    = $(sitelibdir)$(target_prefix)
RUBYARCHDIR   = $(sitearchdir)$(target_prefix)

TARGET_SO     = $(DLLIB)
CLEANLIBS     = $(TARGET).so $(TARGET).il? $(TARGET).tds $(TARGET).map
CLEANOBJS     = *.o *.a *.s[ol] *.pdb *.exp *.bak

all:		$(DLLIB)
static:		$(STATIC_LIB)

clean:
		@-$(RM) $(CLEANLIBS) $(CLEANOBJS) $(CLEANFILES)

distclean:	clean
		@-$(RM) Makefile $(RUBY_EXTCONF_H) conftest.* mkmf.log
		@-$(RM) core ruby$(EXEEXT) *~ $(DISTCLEANFILES)

realclean:	distclean
install: install-so install-rb

install-so: $(RUBYARCHDIR)
install-so: $(RUBYARCHDIR)/$(DLLIB)
$(RUBYARCHDIR)/$(DLLIB): $(DLLIB)
	$(INSTALL_PROG) $(DLLIB) $(RUBYARCHDIR)
install-rb: pre-install-rb install-rb-default
install-rb-default: pre-install-rb-default
pre-install-rb: Makefile
pre-install-rb-default: Makefile
$(RUBYARCHDIR):
	$(MAKEDIRS) $@

site-install: site-install-so site-install-rb
site-install-so: install-so
site-install-rb: install-rb

.SUFFIXES: .c .m .cc .cxx .cpp .C .o

.cc.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.cxx.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.cpp.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.C.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.c.o:
	$(CC) $(INCFLAGS) $(CPPFLAGS) $(CFLAGS) -c $<

$(DLLIB): $(OBJS) Makefile
	@-$(RM) $@
	$(LDSHARED) -o $@ $(OBJS) $(LIBPATH) $(DLDFLAGS) $(LOCAL_LIBS) $(LIBS)



$(OBJS): ruby.h defines.h
