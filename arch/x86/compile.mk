
$(BUILDDIR)/%.o: %.c $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) --std=c99 $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.cpp $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o: %.S $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(ASMFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

