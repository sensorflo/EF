YFLAGS += #--debug --verbose
ENCRYPTED_FILES = ef.txt ef.y ef.l
LEX = flex
YACC = bison


all: out out/ef doc

out:
	mkdir -p out

out/ef_parser.c out/ef_parser.h: ef.y
	$(YACC) $(YFLAGS) -d -o out/ef_parser.c $^

out/ef_scanner.c: ef.l out/ef_parser.h
	$(LEX) $(LFLAGS) -o $@ $<

out/ef: out/ef_parser.c out/ef_scanner.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

doc: out/ef.html

out/ef.html: ef.txt
	asciidoc -a toc -o $@ $^

clean:
	-rm -f out/*

encrypt:
	tar -c $(ENCRYPTED_FILES) | gpg --batch --yes -c -o ef.tar.gpg -  

decrypt:
	gpg --batch --yes -o - -d ef.tar.gpg | tar --overwrite -x

.PHONY: all encrypt decrypt doc clean
