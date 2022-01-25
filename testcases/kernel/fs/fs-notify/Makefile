#
#    kernel/fs/fs-notify testcases Makefile.
#

top_srcdir			?= ../../../..

include $(top_srcdir)/include/mk/testcases.mk

BINARIES:=fanotify_flush_stress fanotify_init_stress \
fanotify_mark_stress fanotify_watch inotify_watch \
inotify_add_rm_stress inotify_init_stress inotify_inmodify_stress\
writefile freadfile fwritefile readfile

INSTALL_TARGETS			:= fsnotify-stress.sh $(BINARIES) rw_files.sh

MAKE_TARGETS			:= $(BINARIES)

include $(top_srcdir)/include/mk/generic_leaf_target.mk
