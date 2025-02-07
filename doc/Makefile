# Copyright (c) Linux Test Project, 2024-2025
# SPDX-License-Identifier: GPL-2.0-or-later

top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk

PYTHON := python3

# install sphinx only if needed
INSTALL_SPHINX := $(shell $(PYTHON) -c "import sphinx" 2>/dev/null && echo "true" || echo "pip install sphinx")

.venv:
	$(PYTHON) -m virtualenv .venv
	. .venv/bin/activate && pip install -r requirements.txt && $(INSTALL_SPHINX)

${abs_top_builddir}/metadata/ltp.json:
	$(MAKE) -C ${abs_top_builddir}/metadata

all: .venv ${abs_top_builddir}/metadata/ltp.json
	. .venv/bin/activate && sphinx-build -b html . html

spelling:
	. .venv/bin/activate && sphinx-build -b spelling -d build/doctree . build/spelling

clean:
	rm -rf html/ build/ _static/syscalls.rst _static/tests.rst syscalls.tbl

distclean: clean
	rm -rf .venv/
