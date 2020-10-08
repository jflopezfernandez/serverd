
TARGETS := serverd

all: $(TARGETS)
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET))

.PHONY: docs
docs:
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET) docs)

.PHONY: clean
clean:
	$(foreach TARGET,$(TARGETS),$(MAKE) -C $(TARGET) clean)
