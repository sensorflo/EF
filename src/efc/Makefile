CXXFLAGS += `llvm-config --cxxflags | sed 's/-fno-exceptions\|-DNDEBUG//g'` -O0 -Wall -Werror -ggdb3 -std=c++98 #-pedantic
LDFLAGS += `llvm-config --ldflags`
LIBS += `llvm-config --libs core jit native interpreter`
CXX = clang++
LEX = flex
YACC = bison
GTEST_DIR = /home/sensorflo/src/gtest
BISON_DATA_DIR := $(shell bison --print-datadir)

all: out gensrc out/efc



out gensrc:
	mkdir -p $@


GEN_HEADERS:=gensrc/parser.hpp gensrc/stack.hh gensrc/position.hh gensrc/location.hh

efc: out/efc

gensrc/parser.cpp $(GEN_HEADERS): ef.yy *.h
	$(YACC) $(YFLAGS) --verbose -x -Werror --warnings=all -d -o gensrc/parser.cpp $<;\
    yaccexitstatus=$$?;\
    xsltproc $(BISON_DATA_DIR)/xslt/xml2xhtml.xsl gensrc/parser.xml > gensrc/parser.xhtml;\
    exit $$yaccexitstatus

gensrc/scanner.cpp: ef.l $(GEN_HEADERS) *.h
	$(LEX) $(LFLAGS) -o $@ $<

out/parser.o: gensrc/parser.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/scanner.o: gensrc/scanner.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/parserext.o: parserext.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/driver.o: driver.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ast.o: ast.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/env.o: env.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/objtype.o: objtype.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/irbuilderast.o: irbuilderast.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efc.o: efc.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efc: out/parser.o out/scanner.o out/parserext.o out/driver.o out/ast.o out/objtype.o out/env.o out/irbuilderast.o out/efc.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)



test efctest: out/efctest

runtest runefctest: out/efctest
	$<

out/efctest.o: test/efctest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/test.o: test/test.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/gtestprinter.o: test/gtestprinter.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/scannertest.o: test/scannertest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/parserexttest.o: test/parserexttest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/scannerandparsertest.o: test/scannerandparsertest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/irbuilderasttest.o: test/irbuilderasttest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/asttest.o: test/asttest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/envtest.o: test/envtest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/objtypetest.o: test/objtypetest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efctest: out/scanner.o out/objtype.o out/ast.o out/env.o out/irbuilderast.o out/parser.o out/parserext.o out/driver.o out/objtypetest.o out/envtest.o out/asttest.o out/parserexttest.o out/scannertest.o out/scannerandparsertest.o out/irbuilderasttest.o out/gtestprinter.o out/test.o out/efctest.o out/libgtest.a
	$(CXX)  $(LDFLAGS) -pthread -o $@ $^ $(LIBS)



out/libgtest.a: ${GTEST_DIR}/src/gtest-all.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c -o out/gtest-all.o $<
	$(AR) $(ARFLAGS) $@ out/gtest-all.o


doc: out/specs.html

out/specs.html: test/*.cpp
	-if type testdox ; then\
		if type asciidoc ; then\
			testdox -f asciidoc $^ | asciidoc -a toc -o $@ -;\
		else\
			testdox $^ > $@;\
		fi ;\
	fi


clean:
	rm -rf out/*
	rm -rf gensrc/*

.PHONY: all efc test efctest runtest runefctest
