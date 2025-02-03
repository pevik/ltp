PYTHON := python3

# install sphinx only if needed
INSTALL_SPHINX := $(shell $(PYTHON) -c "import sphinx" 2>/dev/null && echo "true" || echo "pip install sphinx")

.venv:
	$(PYTHON) -m virtualenv .venv
	. .venv/bin/activate && pip install -r requirements.txt && $(INSTALL_SPHINX)

all: .venv
	. .venv/bin/activate && sphinx-build -b html . html

spelling:
	. .venv/bin/activate && sphinx-build -b spelling -d build/doctree . build/spelling

clean:
	rm -rf html/ _static/syscalls.rst

distclean: clean
	rm -rf .venv/
