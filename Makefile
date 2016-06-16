SOURCES=jsonstate.cpp jsonparser.cpp jsonwriter.cpp  main.cpp parse.cpp
EXEC=tst_jsonstate parse test_unescape

#CFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
CFLAGS=-Wall -g
CXXFLAGS=-std=c++11
CPPFLAGS=$(CFLAGS) $(FLAGS)
PACKAGES=

CPPFLAGS+=-Idouble-conversion/include
LDFLAGS+=-Ldouble-conversion -ldouble-conversion -Wl,-rpath=double-conversion

#CPPFLAGS+=$(shell pkg-config --cflags $(PACKAGES))
#LDFLAGS+=$(shell pkg-config --libs $(PACKAGES))

OBJECTS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).o,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).o,\
$(SOURCES)))
DEPENDS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).d,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).d,\
        $(filter-out %.o,""\
$(SOURCES))))

all: $(EXEC)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif

clean:
	rm -f $(EXEC) $(OBJECTS) $(DEPENDS)

%.d: %.c
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

%.d: %.cpp
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

tst_jsonstate: main.o jsonstate.o
	$(CXX) -o $@ $^ $(LDFLAGS)

parse: parse.o jsonstate.o jsonparser.o jsonwriter.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test_unescape: test_unescape.cpp jsunescape.tcc
	$(CXX) -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

