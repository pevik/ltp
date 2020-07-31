%.o: %.c
ifdef VERBOSE
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
else
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
	@echo CC $@
endif

ifdef VERBOSE
COMPILE.c=$(CC) $(CPPFLAGS) $(CFLAGS) -c
else
COMPILE.c=@echo CC $@; $(CC) $(CPPFLAGS) $(CFLAGS) -c
endif

%: %.o
ifdef VERBOSE
	$(CC) $(LDFLAGS) $^ $(LTPLDLIBS) $(LDLIBS) -o $@
else
	@$(CC) $(LDFLAGS) $^ $(LTPLDLIBS) $(LDLIBS) -o $@
	@echo LD $@
endif

%: %.c
ifdef VERBOSE
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LTPLDLIBS) $(LDLIBS) -o $@
else
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LTPLDLIBS) $(LDLIBS) -o $@
	@echo CC $@
endif
