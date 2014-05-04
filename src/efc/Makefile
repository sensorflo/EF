CXXFLAGS += -O0 -Wall -Werror -ggdb3 -std=c++98 #-pedantic
CXX = clang++
LEX = flex
YACC = bison
GTEST_DIR = /home/sensorflo/src/gtest

all: out gensrc out/efc



out gensrc:
	mkdir -p $@


GEN_HEADERS:=gensrc/parser.hpp gensrc/stack.hh gensrc/position.hh gensrc/location.hh

efc: out/efc

gensrc/parser.cpp $(GEN_HEADERS): ef.yy *.h
	$(YACC) $(YFLAGS) -d -o gensrc/parser.cpp $<

gensrc/scanner.cpp: ef.l $(GEN_HEADERS) *.h
	$(LEX) $(LFLAGS) -o $@ $<

out/parser.o: gensrc/parser.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/scanner.o: gensrc/scanner.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/driver.o: driver.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/ast.o: ast.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/irbuilderast.o: irbuilderast.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efc.o: efc.cpp $(GEN_HEADERS) *.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efc: out/parser.o out/scanner.o out/driver.o out/ast.o out/irbuilderast.o out/efc.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)



test efctest: out/efctest

runtest runefctest: out/efctest
	$<

out/efctest.o: test/efctest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/scannertest.o: test/scannertest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/scannerandparsertest.o: test/scannerandparsertest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/irbuilderasttest.o: test/irbuilderasttest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/asttest.o: test/asttest.cpp $(GEN_HEADERS) *.h test/*.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efctest: out/scanner.o out/ast.o out/irbuilderast.o out/parser.o out/driver.o out/scannertest.o out/scannerandparsertest.o out/irbuilderasttest.o out/asttest.o out/efctest.o out/libgtest.a
	$(CXX)  $(LDFLAGS) -pthread -o $@ $^



out/libgtest.a: ${GTEST_DIR}/src/gtest-all.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c -o out/gtest-all.o $<
	$(AR) $(ARFLAGS) $@ out/gtest-all.o



clean:
	rm -rf out/*
	rm -rf gensrc/*

.PHONY: all efc test efctest runtest runefctest
