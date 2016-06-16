SOURCES=test_surrogate.cpp test_segmenter.cpp test_decoder.cpp
EXEC=test_surrogate test_segmenter test_decoder

#CFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
CFLAGS=-Wall -g
CXXFLAGS=-std=c++11
CPPFLAGS=$(CFLAGS) $(FLAGS)
PACKAGES=

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

test_surrogate: test_surrogate.o utfsurrogate.tcc
	$(CXX) -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

test_segmenter: test_segmenter.o utf8segmenter.tcc
	$(CXX) -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

test_decoder: test_decoder.o utf8.tcc
	$(CXX) -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

