
$(BUILDDIR)/%.o: %.c $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(THUMBCFLAGS) --std=c99 $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.cpp $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(CPPFLAGS) $(THUMBCFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

# to override thumb setting, mark the .o file as .Ao
$(BUILDDIR)/%.Ao: %.c $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) --std=c99 $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.Ao: %.cpp $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

# assembly is always compiled in ARM mode at the moment
$(BUILDDIR)/%.Ao: %.S $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(ASMFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.S $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(ASMFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

