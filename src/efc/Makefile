CXXFLAGS += `llvm-config --cxxflags | sed 's/-fno-exceptions\|-DNDEBUG//g'` -O0 -Wall -Werror -std=c++11 -Wno-deprecated-register #-pedantic
LDFLAGS += `llvm-config --ldflags`
LIBS += `llvm-config --libs core mcjit native interpreter --system-libs | sed 's/-ledit//g'`
CXX = clang++
LEX = flex
YACC = bison
GTEST_DIR = /home/sensorflo/src/gtest
BISON_DATA_DIR := $(shell bison --print-datadir)

all: out/efc

out gensrc:
	mkdir -p $@


# efc
# ----------------------------------------------------------------------

GEN_HEADERS:=gensrc/parser.hpp gensrc/stack.hh gensrc/position.hh gensrc/location.hh

EFC_SRCS = driver.cpp parserext.cpp ast.cpp errorhandler.cpp objtype.cpp \
  astdefaultiterator.cpp env.cpp semanticanalizer.cpp irgen.cpp astprinter.cpp \
  efc.cpp executionengineadapter.cpp

EFC_OBJS = out/parser.o out/scanner.o $(patsubst %.cpp, out/%.o, $(EFC_SRCS))

efc: out/efc

gensrc/parser.cpp $(GEN_HEADERS): ef.yy ast.h objtype.h parserext.h driver.h access.h astforwards.h | gensrc
	$(YACC) $(YFLAGS) --verbose -x -Werror --warnings=all -d -o gensrc/parser.cpp $<;\
    yaccexitstatus=$$?;\
    xsltproc $(BISON_DATA_DIR)/xslt/xml2xhtml.xsl gensrc/parser.xml > gensrc/parser.xhtml;\
    exit $$yaccexitstatus

gensrc/scanner.cpp: ef.l driver.h gensrc/parser.hpp | gensrc
	$(LEX) $(LFLAGS) -o $@ $<

efc.h.pch: efc.h objtype.h access.h ast.h
	$(CXX) -x c++-header $(CXXFLAGS) $(CPPFLAGS) $< -o $@

out/%.o: gensrc/%.cpp efc.h.pch | out
	$(CXX) -include efc.h $(CXXFLAGS) $(CPPFLAGS) -c -MD -o $@ $<
	@cp out/$*.d out/$*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < out/$*.d >> out/$*.P;

out/%.o: %.cpp efc.h.pch | out
	$(CXX) -include efc.h $(CXXFLAGS) $(CPPFLAGS) -c -MD -o $@ $<
	@cp out/$*.d out/$*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < out/$*.d >> out/$*.P;

out/efc: $(EFC_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)



# efctest
# ----------------------------------------------------------------------

EFCTEST_SRCS = errorhandlertest.cpp objtypetest.cpp parserexttest.cpp	\
  scannertest.cpp scannerandparsertest.cpp astprintertest.cpp	\
  driversystemtest.cpp semanticanalizertest.cpp irgentest.cpp	\
  gtestprinter.cpp test.cpp tutorialstest.cpp efctest.cpp

EFCTEST_OBJS = $(patsubst %.cpp, out/%.o, $(EFCTEST_SRCS))

test efctest: out/efctest

runtest runefctest: out/efctest
	$<

test/efctest.h.pch: test/efctest.h efc.h.pch test/test.h
	$(CXX) -x c++-header $(CXXFLAGS) $(CPPFLAGS) $< -o $@

out/%.o: test/%.cpp test/efctest.h.pch | out
	$(CXX) -include test/efctest.h $(CXXFLAGS) $(CPPFLAGS) -c -MD -o $@ $<
	@cp out/$*.d out/$*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < out/$*.d >> out/$*.P;

out/libgtest.a: ${GTEST_DIR}/src/gtest-all.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c -o out/gtest-all.o $<
	$(AR) $(ARFLAGS) $@ out/gtest-all.o

out/efctest: $(filter-out out/efc.o, $(EFC_OBJS)) $(EFCTEST_OBJS) out/libgtest.a
	$(CXX)  $(LDFLAGS) -pthread -o $@ $^ $(LIBS)


# doc
# ----------------------------------------------------------------------

doc: out/specs.html

out/specs.html: test/*.cpp | out
	-if type testdox >/dev/null ; then\
		if type asciidoc >/dev/null ; then\
			testdox -f asciidoc $^ | asciidoc -a toc -o $@ -;\
		else\
			testdox $^ > $@;\
		fi ;\
	fi


# other
# ----------------------------------------------------------------------

clean:
	rm -rf out
	rm -rf gensrc
	rm -rf *.pch

.PHONY: all efc test efctest runtest runefctest

-include $(EFC_SRCS:%.cpp=out/%.P)
-include $(EFCTEST_SRCS:%.cpp=out/%.P)

