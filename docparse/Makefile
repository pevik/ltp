top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

MAKE_TARGETS		:=  *.html metadata.json docparse
INSTALL_DIR = metadata
INSTALL_TARGETS = *.css *.html *.js

.PHONY: all %.html metadata.json

docparse: data_storage.h

metadata.json: docparse
	./parse.sh > metadata.json

validate:
	jsonschema metadata.schema.json -i metadata.json

%.html: metadata.json
	./testinfo.pl metadata.json

all: %.html

include $(top_srcdir)/include/mk/generic_leaf_target.mk
