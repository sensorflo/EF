CXXFLAGS += `llvm-config --cxxflags | sed 's/-fno-exceptions\|-DNDEBUG//g'` -O0 -Wall -Werror -std=c++14 -Wno-deprecated-register #-pedantic
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


# Expands to commands which build a .cpp file
# $1: file name of header file which has a precompiled version
# $2: additional CXXFLAGS / CPPFLAGS
define build_cpp
	$(CXX) -include $1 $(CXXFLAGS) $(CPPFLAGS) $2 -c -MD -o $@ $<
	@cp out/$*.d out/$*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	  -e '/^$$/ d' -e 's/$$/ :/' <out/$*.d >>out/$*.P;
endef

# efc
# ----------------------------------------------------------------------

EFC_SRCS = driver.cpp parserext.cpp ast.cpp errorhandler.cpp objtype.cpp \
  astdefaultiterator.cpp env.cpp semanticanalizer.cpp irgen.cpp astprinter.cpp \
  efc.cpp executionengineadapter.cpp parserapiext.cpp tokenstreamlookahead.cpp \
  tokenfilter.cpp storageduration.cpp object.cpp envinserter.cpp \
	signatureaugmentor.cpp irgenforwarddeclarator.cpp envnode.cpp \
	freefromastobject.cpp

EFC_GEN_SRCS = parser.cpp scanner.cpp

EFC_OBJS = $(patsubst %.cpp, out/%.o, $(EFC_SRCS) $(EFC_GEN_SRCS))

efc: out/efc

# gensrc/scanner.hpp dependency problem: For unknown reasons, make did execute
# rules which depend on gensrc/scanner.hpp before gensrc/scanner.hpp was
# finished being built, altough actually the auto generated dependencies
# (out/*.P) should prevent that. The current workaround is to make these rules
# depend on out/parser.o.
gensrc/%.cpp gensrc/%.hpp: %.yy parser.hpp.patch | gensrc
	$(YACC) $(YFLAGS) --verbose -x -Werror --warnings=all -d -o gensrc/parser.cpp $<;\
    yaccexitstatus=$$?;\
    xsltproc $(BISON_DATA_DIR)/xslt/xml2xhtml.xsl gensrc/parser.xml > gensrc/parser.xhtml;\
    patch gensrc/parser.hpp < parser.hpp.patch;\
    exit $$yaccexitstatus

gensrc/scanner.cpp: scanner.l | gensrc
	$(LEX) $(LFLAGS) -o $@ $<

# The author sadly doesn't know how to automatically make the dependencies,
# that's why they are written explicitely and redundantely.
efc.h.pch: efc.h objtype.h access.h ast.h generalvalue.h storageduration.h \
					 envnode.h astforwards.h declutils.h object.h
	$(CXX) -x c++-header $(CXXFLAGS) $(CPPFLAGS) $< -o $@

out/%.o: gensrc/%.cpp efc.h.pch | out
	$(call build_cpp, efc.h)

# regarding the dependency to out/parser.o, see comment of bison's rule
out/%.o: %.cpp efc.h.pch out/parser.o | out
	$(call build_cpp, efc.h)

out/efc: $(EFC_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)



# efctest
# ----------------------------------------------------------------------

EFCTEST_ROOT_SRCS = gtestprinter.cpp test.cpp efctest.cpp

EFCTEST_TESTS_SRCS = errorhandlertest.cpp objtypetest.cpp parserexttest.cpp	\
  scannertest.cpp scannerandparsertest.cpp astprintertest.cpp	\
	driversystemtest.cpp semanticanalizertest.cpp irgentest.cpp	\
  tutorialstest.cpp tokenstreamlookaheadtest.cpp tokenfiltertest.cpp \
	envtest.cpp asttest.cpp

EFCTEST_TESTHELPERS_SRCS = literaltokenstream.cpp tokentesthelper.cpp

EFCTEST_TESTS_TESTHELPERS_SRCS = literaltokenstreamtest.cpp

EFCTEST_OBJS = $(patsubst %.cpp, out/%.o, \
  $(EFCTEST_ROOT_SRCS) $(EFCTEST_TESTS_SRCS) $(EFCTEST_TESTHELPERS_SRCS) \
  $(EFCTEST_TESTS_TESTHELPERS_SRCS))

test efctest: out/efctest

runtest runefctest: out/efctest
	$<

# The author sadly doesn't know how to automatically make the dependencies,
# that's why they are written explicitely and redundantely.
test/efctest.h.pch: test/efctest.h efc.h.pch test/test.h
	$(CXX) -x c++-header $(CXXFLAGS) $(CPPFLAGS) $< -o $@

# regarding the dependency to out/parser.o, see comment of bison's rule
out/%.o: test/%.cpp test/efctest.h.pch out/parser.o | out
	$(call build_cpp, test/efctest.h, -I test -I test/testhelpers)

out/%.o: test/tests/%.cpp test/efctest.h.pch out/parser.o | out
	$(call build_cpp, test/efctest.h, -I test -I test/testhelpers)

out/%.o: test/testhelpers/%.cpp test/efctest.h.pch out/parser.o | out
	$(call build_cpp, test/efctest.h, -I test -I test/testhelpers)

out/%.o: test/tests/testhelpers/%.cpp test/efctest.h.pch out/parser.o | out
	$(call build_cpp, test/efctest.h, -I test -I test/testhelpers)


out/libgtest.a: ${GTEST_DIR}/src/gtest-all.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c -o out/gtest-all.o $<
	$(AR) $(ARFLAGS) $@ out/gtest-all.o

out/efctest: $(filter-out out/efc.o, $(EFC_OBJS)) $(EFCTEST_OBJS) out/libgtest.a
	$(CXX)  $(LDFLAGS) -pthread -o $@ $^ $(LIBS)


# doc
# ----------------------------------------------------------------------

doc: out/specs.html

out/specs.html: $(patsubst %, test/tests/%, $(EFCTEST_TESTS_SRCS)) \
  $(patsubst %, test/tests/testhelpers/%, $(EFCTEST_TESTS_TESTHELPERS_SRCS)) | out
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
-include $(EFC_GEN_SRCS:%.cpp=out/%.P)
-include $(EFCTEST_ROOT_SRCS:%.cpp=out/%.P)
-include $(EFCTEST_TESTS_SRCS:%.cpp=out/%.P)
-include $(EFCTEST_TESTHELPERS_SRCS:%.cpp=out/%.P)
-include $(EFCTEST_TESTS_TESTHELPERS_SRCS:%.cpp=out/%.P)
