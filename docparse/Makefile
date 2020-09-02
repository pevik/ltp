top_srcdir		?= ..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

MAKE_TARGETS		:= metadata.json
HOST_MAKE_TARGETS	:= docparse

INSTALL_DIR = metadata

.PHONY: metadata.json

metadata.json: docparse
	$(abs_srcdir)/parse.sh > metadata.json

validate:
	jsonschema metadata.schema.json -i metadata.json

include $(top_srcdir)/include/mk/generic_leaf_target.mk
