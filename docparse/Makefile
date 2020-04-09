top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

MAKE_TARGETS		:=  metadata.json docparse
INSTALL_DIR = metadata
INSTALL_TARGETS = *.css *.html *.js

.PHONY: all metadata.json

docparse: data_storage.h

metadata.json: docparse
	./parse.sh > metadata.json

validate:
	jsonschema metadata.schema.json -i metadata.json

all: metadata.json
	./testinfo.pl metadata.json

include $(top_srcdir)/include/mk/generic_leaf_target.mk
