# Copyright (c) Linux Test Project, 2024-2025
# SPDX-License-Identifier: GPL-2.0-or-later

PYTHON := python3

.venv:
	$(PYTHON) -m virtualenv .venv
	. .venv/bin/activate && pip install -r requirements.txt

all: .venv
	. .venv/bin/activate && sphinx-build -b html . html

spelling:
	. .venv/bin/activate && sphinx-build -b spelling -d build/doctree . build/spelling

clean:
	rm -rf html/ _static/syscalls.rst

distclean: clean
	rm -rf .venv/
