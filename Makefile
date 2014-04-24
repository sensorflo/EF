YFLAGS += --debug --verbose
CXXFLAGS += `llvm-config --cxxflags | sed 's/-fno-exceptions//'` -O0 -Wall -Werror -ggdb3 #-std=c++98 -pedantic 
LDFLAGS += `llvm-config --ldflags`
LIBS += `llvm-config --libs core`
ENCRYPTED_FILES = ef.txt ef.yy ef.l ef_driver.cc ef_driver.hh ef.cc ast.cc ast.hh
CXX = clang++
LEX = flex
YACC = bison


all: out out/ef doc

out:
	mkdir -p out

out/ef_parser.cc out/ef_parser.hh: ef.yy
	$(YACC) $(YFLAGS) -d -o out/ef_parser.cc $^

out/ef_scanner.cc: ef.l out/ef_parser.hh
	$(LEX) $(LFLAGS) -o $@ $<

out/ef_parser.o: out/ef_parser.cc out/ef_parser.hh ast.hh ef_driver.hh
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ef_scanner.o: out/ef_scanner.cc out/ef_parser.hh ef_driver.hh
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ef_driver.o: ef_driver.cc ef_driver.hh out/ef_parser.hh ast.hh
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ast.o: ast.cc ast.hh out/ef_parser.hh
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ef.o: ef.cc ef_driver.hh out/ef_parser.hh
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ef: out/ef.o out/ast.o out/ef_driver.o out/ef_parser.o out/ef_scanner.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

doc: out/ef.html

out/ef.html: ef.txt README
	asciidoc -a toc -a toclevels=3 -o $@ $<

clean:
	-rm -f out/*

encrypt:
	tar -c $(ENCRYPTED_FILES) | gpg --batch --yes -c -o ef.tar.gpg -  

decrypt:
	gpg --batch --yes -o - -d ef.tar.gpg | tar --overwrite -x

.PHONY: all encrypt decrypt doc clean
