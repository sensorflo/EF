all: efc doc

allreally: all alltest runalltest

alltest: efctest

runalltest: runefctest

efc efctest runefctest:
	cd src/efc && $(MAKE) $@

doc:
	cd src/efc && $(MAKE) $@
	cd doc && $(MAKE) $@

clean:
	cd src/efc && $(MAKE) $@
	cd doc && $(MAKE) $@

.PHONY: all allreally alltest runalltest clean efc efctest runefctest doc
