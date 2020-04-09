top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

MAKE_TARGETS		:= metadata.json

ifeq ($(WITH_METADATA_HTML),yes)
MAKE_TARGETS		+= metadata.html
endif

ifeq ($(WITH_METADATA_PDF),yes)
MAKE_TARGETS		+= metadata.pdf
endif

HOST_MAKE_TARGETS	:= docparse

INSTALL_DIR = metadata
INSTALL_TARGETS = *.css *.js

ifndef METADATA_GENERATOR
METADATA_GENERATOR := asciidoctor
endif

ifeq ($(METADATA_GENERATOR),asciidoctor)
METADATA_GENERATOR_CMD := asciidoctor
METADATA_GENERATOR_PARAMS := -d book metadata.txt
METADATA_GENERATOR_PARAMS_HTML := -b xhtml
METADATA_GENERATOR_PARAMS_PDF := -b pdf -r asciidoctor-pdf
else ifeq ($(METADATA_GENERATOR),asciidoc)
METADATA_GENERATOR_CMD := a2x
METADATA_GENERATOR_PARAMS := --xsltproc-opts "--stringparam toc.section.depth 1" -d book -L  --resource="$(PWD)" metadata.txt
METADATA_GENERATOR_PARAMS_HTML := -f xhtml
METADATA_GENERATOR_PARAMS_PDF := -f pdf
else
$(error '$(METADATA_GENERATOR)' not supported, only asciidoctor and asciidoc are supported)
endif

ifdef VERBOSE
METADATA_GENERATOR_PARAMS += -v
endif

.PHONY: metadata.json

metadata.json: docparse
	$(abs_srcdir)/parse.sh > metadata.json

validate:
	jsonschema metadata.schema.json -i metadata.json

txt: metadata.json
	./testinfo.pl metadata.json

ifeq ($(WITH_METADATA_HTML),yes)
metadata.html: txt
	$(METADATA_GENERATOR_CMD) $(METADATA_GENERATOR_PARAMS) $(METADATA_GENERATOR_PARAMS_HTML)
endif

ifeq ($(WITH_METADATA_PDF),yes)
metadata.pdf: txt
	$(METADATA_GENERATOR_CMD) $(METADATA_GENERATOR_PARAMS) $(METADATA_GENERATOR_PARAMS_PDF)
endif

include $(top_srcdir)/include/mk/generic_leaf_target.mk
