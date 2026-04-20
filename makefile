# ==============================================================================
# Toolchain
# ==============================================================================

CXX      = g++
CXXFLAGS = -std=c++20
AR       = ar
ARFLAGS  = rcs

# ==============================================================================
# Paths
# ==============================================================================

ANTLR4_INC = /usr/include/antlr4-runtime
ANTLR4_LIB = /usr/lib/x86_64-linux-gnu
ANTLR4_JAR = tools/antlr4-4.10.1-complete.jar
ANTLR4_URL = https://www.antlr.org/download/antlr-4.10.1-complete.jar

CLI11_INC = CLI11/include

GIT_TAG := $(shell git describe --tags --always --dirty)

# ==============================================================================
# Output names  (all under build/)
# ==============================================================================

CORE_LIB            = build/libatlas_core.a
CORE_DEBUG_LIB      = build/libatlas_core_debug.a
CORE_DEBUG_FAST_LIB = build/libatlas_core_debug_fast.a

PARSER_LIB            = build/libatlas_parser.a
PARSER_DEBUG_LIB      = build/libatlas_parser_debug.a
PARSER_DEBUG_FAST_LIB = build/libatlas_parser_debug_fast.a

CLI_LIB            = build/libatlas_cli.a
CLI_DEBUG_LIB      = build/libatlas_cli_debug.a
CLI_DEBUG_FAST_LIB = build/libatlas_cli_debug_fast.a

CORE_DEBUG_BIN      = build/core_debug
CORE_DEBUG_FAST_BIN = build/core_debug_fast

PARSER_DEBUG_BIN      = build/parser_debug
PARSER_DEBUG_FAST_BIN = build/parser_debug_fast

CLI_DEBUG_BIN      = build/cli_debug
CLI_DEBUG_FAST_BIN = build/cli_debug_fast

ATLAS_BIN = build/atlas

# ==============================================================================
# Object lists  (object files live in build/obj/<variant>/)
# ==============================================================================

# Core: discovered at parse time (source files are always present).
CORE_SRC = $(wildcard core/cpp/*.cpp)

CORE_OBJ            = $(patsubst core/cpp/%.cpp, build/obj/core/%.o,            $(CORE_SRC))
CORE_DEBUG_OBJ      = $(patsubst core/cpp/%.cpp, build/obj/core_debug/%.o,      $(CORE_SRC))
CORE_DEBUG_FAST_OBJ = $(patsubst core/cpp/%.cpp, build/obj/core_debug_fast/%.o, $(CORE_SRC))

# Parser generated: hardcoded because parser/generated/ may not exist at parse
# time, so $(wildcard ...) would expand to nothing.  ANTLR4 always emits
# exactly these four files from CHC.g4.
PARSER_GENERATED_STEMS = CHCLexer CHCParser CHCBaseVisitor CHCVisitor

# Parser hand-written: safe to wildcard — parser/cpp/ always exists.
PARSER_SRC = $(wildcard parser/cpp/*.cpp)

PARSER_OBJ = \
    $(patsubst %,                build/obj/parser/%.o,            $(PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser/%.o,            $(PARSER_SRC))
PARSER_DEBUG_OBJ = \
    $(patsubst %,                build/obj/parser_debug/%.o,      $(PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser_debug/%.o,      $(PARSER_SRC))
PARSER_DEBUG_FAST_OBJ = \
    $(patsubst %,                build/obj/parser_debug_fast/%.o, $(PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser_debug_fast/%.o, $(PARSER_SRC))

# CLI: source files are always present; no codegen needed for compilation.
# cli/cpp/ contains the library sources; cli/entry/ contains the entrypoint.
CLI_SRC = $(wildcard cli/cpp/*.cpp)

CLI_OBJ            = $(patsubst cli/cpp/%.cpp, build/obj/cli/%.o,            $(CLI_SRC))
CLI_DEBUG_OBJ      = $(patsubst cli/cpp/%.cpp, build/obj/cli_debug/%.o,      $(CLI_SRC))
CLI_DEBUG_FAST_OBJ = $(patsubst cli/cpp/%.cpp, build/obj/cli_debug_fast/%.o, $(CLI_SRC))

# ==============================================================================
# User-facing targets
# ==============================================================================

.PHONY: all core core_debug core_debug_fast parser parser_debug parser_debug_fast \
        cli cli_debug cli_debug_fast atlas clean

all: core core_debug core_debug_fast parser parser_debug parser_debug_fast \
     cli cli_debug cli_debug_fast atlas

core: $(CORE_LIB)

core_debug: $(CORE_DEBUG_LIB)
	$(CXX) $(CXXFLAGS) -DDEBUG -g core/test/main.cpp -Lbuild -latlas_core_debug -o $(CORE_DEBUG_BIN)

core_debug_fast: $(CORE_DEBUG_FAST_LIB)
	$(CXX) $(CXXFLAGS) -DDEBUG -g -O3 core/test/main.cpp -Lbuild -latlas_core_debug_fast -o $(CORE_DEBUG_FAST_BIN)

# Parser targets use recursive make: the dependency graph is resolved statically
# at startup, before codegen has produced the .cpp files.  Phase 1 runs ANTLR4;
# phase 2 re-invokes make so the pattern rules can find the generated sources.
parser:
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_LIB)

parser_debug: $(CORE_DEBUG_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_LIB)
	$(CXX) $(CXXFLAGS) -DDEBUG -g \
	    -I$(ANTLR4_INC) \
	    parser/test/main.cpp \
	    -Lbuild -latlas_parser_debug -latlas_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -o $(PARSER_DEBUG_BIN)

parser_debug_fast: $(CORE_DEBUG_FAST_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_FAST_LIB)
	$(CXX) $(CXXFLAGS) -DDEBUG -g -O3 \
	    -I$(ANTLR4_INC) \
	    parser/test/main.cpp \
	    -Lbuild -latlas_parser_debug_fast -latlas_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -o $(PARSER_DEBUG_FAST_BIN)

# CLI lib: sources never require codegen, so no recursive make needed.
cli: $(CLI_LIB)

cli_debug: $(CORE_DEBUG_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_LIB)
	$(MAKE) $(CLI_DEBUG_LIB)
	$(CXX) $(CXXFLAGS) -DDEBUG -g \
	    cli/test/main.cpp \
	    -Lbuild -latlas_cli_debug -latlas_parser_debug -latlas_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -o $(CLI_DEBUG_BIN)

cli_debug_fast: $(CORE_DEBUG_FAST_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_FAST_LIB)
	$(MAKE) $(CLI_DEBUG_FAST_LIB)
	$(CXX) $(CXXFLAGS) -DDEBUG -g -O3 \
	    cli/test/main.cpp \
	    -Lbuild -latlas_cli_debug_fast -latlas_parser_debug_fast -latlas_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -o $(CLI_DEBUG_FAST_BIN)

# Release entrypoint: links cli/entry/main.cpp against the three release libs.
# CLI11 headers are only needed here (the library sources don't use CLI11).
atlas: $(CORE_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_LIB)
	$(MAKE) $(CLI_LIB)
	$(CXX) $(CXXFLAGS) -O3 \
	    -I$(CLI11_INC) \
	    -DATLAS_GIT_TAG=\"$(GIT_TAG)\" \
	    cli/entry/main.cpp \
	    -Lbuild -latlas_cli -latlas_parser -latlas_core \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -o $(ATLAS_BIN)

clean:
	rm -rf build
	rm -rf parser/generated

# ==============================================================================
# Library archive rules
# ==============================================================================

$(CORE_LIB): $(CORE_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CORE_DEBUG_LIB): $(CORE_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CORE_DEBUG_FAST_LIB): $(CORE_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(PARSER_LIB): $(PARSER_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(PARSER_DEBUG_LIB): $(PARSER_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(PARSER_DEBUG_FAST_LIB): $(PARSER_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_LIB): $(CLI_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_DEBUG_LIB): $(CLI_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_DEBUG_FAST_LIB): $(CLI_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

# ==============================================================================
# Compilation pattern rules
# ==============================================================================

build/obj/core/%.o: core/cpp/%.cpp | build/obj/core
	$(CXX) $(CXXFLAGS) -O3 -c $< -o $@

build/obj/core_debug/%.o: core/cpp/%.cpp | build/obj/core_debug
	$(CXX) $(CXXFLAGS) -DDEBUG -g -c $< -o $@

build/obj/core_debug_fast/%.o: core/cpp/%.cpp | build/obj/core_debug_fast
	$(CXX) $(CXXFLAGS) -DDEBUG -g -O3 -c $< -o $@

build/obj/parser/%.o: parser/generated/%.cpp | parser/generated build/obj/parser
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) -O3 -c $< -o $@

build/obj/parser_debug/%.o: parser/generated/%.cpp | parser/generated build/obj/parser_debug
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) -DDEBUG -g -c $< -o $@

build/obj/parser_debug_fast/%.o: parser/generated/%.cpp | parser/generated build/obj/parser_debug_fast
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) -DDEBUG -g -O3 -c $< -o $@

build/obj/parser/%.o: parser/cpp/%.cpp | build/obj/parser
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) -O3 -c $< -o $@

build/obj/parser_debug/%.o: parser/cpp/%.cpp | build/obj/parser_debug
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) -DDEBUG -g -c $< -o $@

build/obj/parser_debug_fast/%.o: parser/cpp/%.cpp | build/obj/parser_debug_fast
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) -DDEBUG -g -O3 -c $< -o $@

build/obj/cli/%.o: cli/cpp/%.cpp | build/obj/cli
	$(CXX) $(CXXFLAGS) -O3 -c $< -o $@

build/obj/cli_debug/%.o: cli/cpp/%.cpp | build/obj/cli_debug
	$(CXX) $(CXXFLAGS) -DDEBUG -g -c $< -o $@

build/obj/cli_debug_fast/%.o: cli/cpp/%.cpp | build/obj/cli_debug_fast
	$(CXX) $(CXXFLAGS) -DDEBUG -g -O3 -c $< -o $@

# ==============================================================================
# Build directory creation
# ==============================================================================

build build/obj/core build/obj/core_debug build/obj/core_debug_fast \
build/obj/parser build/obj/parser_debug build/obj/parser_debug_fast \
build/obj/cli build/obj/cli_debug build/obj/cli_debug_fast:
	mkdir -p $@

# ==============================================================================
# ANTLR4 codegen
# ==============================================================================

$(ANTLR4_JAR):
	mkdir -p tools
	curl -fsSL -o $@ $(ANTLR4_URL)

parser/generated: $(ANTLR4_JAR)
	mkdir -p parser/generated
	cd parser/grammar && java -jar ../../$(ANTLR4_JAR) -Dlanguage=Cpp -visitor -no-listener \
	    -o ../../parser/generated/ CHC.g4
	sed -i 's|#include "antlr4-runtime.h"|#include <antlr4-runtime/antlr4-runtime.h>|g' \
	    parser/generated/*.h parser/generated/*.cpp
