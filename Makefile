
TARGETS := serverd

all: $(TARGETS)
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET))

.PHONY: docs
docs:
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET) docs)

.PHONY: install
install: $(TARGETS)
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET) install)

.PHONY: uninstall
uninstall:
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET) uninstall)

.PHONY: clean
clean:
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET) clean)
