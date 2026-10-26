CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Isrc
LDFLAGS ?=

CURL_LIBS := -lcurl
ifneq ($(shell command -v pkg-config >/dev/null 2>&1 && pkg-config --exists libcurl && echo yes),)
  CPPFLAGS += $(shell pkg-config --cflags libcurl)
  CURL_LIBS := $(shell pkg-config --libs libcurl)
endif

PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

LIB_SRC := \
	src/json.cpp \
	src/http.cpp \
	src/format.cpp \
	src/currency.cpp \
	src/markets.cpp \
	src/coingecko.cpp \
	src/coinpaprika.cpp \
	src/kraken.cpp \
	src/bitfinex.cpp \
	src/registry.cpp \
	src/cli.cpp

LIB_OBJ := $(patsubst src/%.cpp,build/%.o,$(LIB_SRC))

.PHONY: all test clean install

all: build/coinconvert

build/coinconvert: build/main.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ build/main.o $(LIB_OBJ) $(CURL_LIBS) $(LDFLAGS)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

build/tests: tests/test_coinconvert.cpp $(LIB_OBJ)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ tests/test_coinconvert.cpp $(LIB_OBJ) $(CURL_LIBS) $(LDFLAGS)

test: build/tests
	./build/tests

install: build/coinconvert
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 build/coinconvert $(DESTDIR)$(BINDIR)/coinconvert

clean:
	rm -rf build
