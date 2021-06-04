# Rules to make clang-check tool(s) for inclusion in lib and testcases Makefiles

CLANG_CHECK_DIR:= $(abs_top_builddir)/tools/clang-check

$(CLANG_CHECK_DIR)/main: $(CLANG_CHECK_DIR)
	$(MAKE) -C "$^" -f "$(CLANG_CHECK_DIR)/Makefile" all

$(CLANG_CHECK_DIR): %:
	mkdir -p "$@"
