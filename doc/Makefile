all: out doc

out:
	mkdir -p out

doc: out/ef.html

out/ef.html: ef.txt ../README
	asciidoc -a toc -a toclevels=3 -o $@ $<

clean:
	rm -rf out/*

.PHONY: all doc clean
