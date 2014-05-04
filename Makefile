all: efc doc

allreally: all alltest runalltest

alltest: efctest

runalltest: runefctest

efc efctest runefctest:
	cd src/efc && $(MAKE) $@

doc:
	cd doc && $(MAKE) $@

clean:
	cd src/efc && $(MAKE) $@
	cd doc && $(MAKE) $@

encrypt: clean
	tar -c doc src | gpg --batch --yes -c -o ef.tar.gpg -

decrypt:
	gpg --batch --yes -o - -d ef.tar.gpg | tar --overwrite -x

.PHONY: all allreally alltest runalltest clean efc efctest runefctest doc encrypt decrypt
