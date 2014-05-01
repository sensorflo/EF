CXXFLAGS += -O0 -Wall -Werror -ggdb3 -std=c++98 #-pedantic 
CXX = clang++
GTEST_DIR = /home/sensorflo/src/gtest

all: out out/efc



out:
	mkdir -p out


efc: out/efc

out/efc.o: efc.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efc: out/efc.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)



efctest: out/efctest
	out/efctest

out/efctest.o: test/efctest.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

out/efctest: out/efctest.o out/libgtest.a 
	$(CXX)  $(LDFLAGS) -pthread -o $@ $^ 



out/libgtest.a: ${GTEST_DIR}/src/gtest-all.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c -o out/gtest-all.o $<
	$(AR) -rv $@ out/gtest-all.o
#$(ARFLAGS)

clean:
	rm -rf out/*

.PHONY: all efc efctest
