# Copyright (c) Linux Test Project, 2024-2025
# SPDX-License-Identifier: GPL-2.0-or-later

top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk

PYTHON := python3
VENV_DIR := .venv
VENV_CMD := . $(VENV_DIR)/bin/activate
RUN_VENV := if [ -d $(VENV_DIR) ]; then $(VENV_CMD); fi

# install sphinx only if needed
INSTALL_SPHINX := $(shell $(PYTHON) -c "import sphinx" 2>/dev/null && echo ":" || echo "pip install sphinx")

$(VENV_DIR):
	$(PYTHON) -m virtualenv $(VENV_DIR)
	$(VENV_CMD) && pip install -r requirements.txt && $(INSTALL_SPHINX)

${abs_top_builddir}/metadata/ltp.json:
	$(MAKE) -C ${abs_top_builddir}/metadata

all: ${abs_top_builddir}/metadata/ltp.json
	$(RUN_VENV); sphinx-build -b html . html

spelling:
	$(RUN_VENV); sphinx-build -b spelling -d build/doctree . build/spelling

clean:
	rm -rf html/ build/ _static/syscalls.rst _static/tests.rst syscalls.tbl \
		${abs_top_builddir}/metadata/ltp.json

distclean: clean
	rm -rf $(VENV_DIR)
