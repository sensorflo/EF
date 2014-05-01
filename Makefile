all: efc doc

allreally: all alltest

alltest: efctest

efc efctest:
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

.PHONY: all allreally alltest clean efc efctest doc encrypt decrypt
