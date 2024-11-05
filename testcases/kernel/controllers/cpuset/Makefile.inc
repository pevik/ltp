#
#    kernel/controllers/cpuset testcase suite common definitions Makefile.
#
#    Copyright (C) 2009, Cisco Systems Inc.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Ngie Cooper, July 2009
#

LIBCONTROLLERS_DIR	:= ../../libcontrollers

LIBCONTROLLERS		:= $(LIBCONTROLLERS_DIR)/libcontrollers.a

LIBCPUSET_DIR       := ../../../../../libs/cpuset_lib

LIBCPUSET           := $(LIBCPUSET_DIR)/libcpu_set.a

$(LIBCONTROLLERS_DIR):
	mkdir -p "$@"

$(LIBCONTROLLERS): $(LIBCONTROLLERS_DIR)
	$(MAKE) -C $^ -f "$(abs_srcdir)/$^/Makefile" all

$(LIBCPUSET):
	$(MAKE) -C $(LIBCPUSET_DIR) -f "$(abs_srcdir)/$(LIBCPUSET_DIR)/Makefile" all

MAKE_DEPS       := $(LIBCONTROLLERS) $(LIBCPUSET)

LDFLAGS			+= -L$(abs_builddir)/$(LIBCONTROLLERS_DIR)

LDLIBS			:= -lcontrollers -lltp $(LDLIBS)

INSTALL_TARGETS		?= *.sh

# vim: syntax=make
