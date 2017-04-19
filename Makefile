CC=g++
CPPFLAGS=-c -g -Wall -rdynamic -D_DEBUG -std=c++11
LDFLAGS=-g -rdynamic

SOURCES=  \
TinyJS_Functions.cpp \
TinyJS_MathFunctions.cpp \
jsLexer.cpp \
utils.cpp \
jsVars.cpp \
jsParser.cpp \
ast.cpp \
parserResults.cpp \
microVM.cpp \
mvmDisassembly.cpp \
scriptMain.cpp \
mvmFunctions.cpp \
semanticCheck.cpp \
asObjects.cpp \
asString.cpp \
jsArray.cpp \
ScriptPosition.cpp \
ScriptException.cpp \
mvmCodegen.cpp 
#executionScope.cpp \
#actorRuntime.cpp \
#asActors.cpp \

OBJECTS=$(SOURCES:.cpp=.o)

all: run_tests Script

run_tests: run_tests.o $(OBJECTS)
	$(CC) $(LDFLAGS) run_tests.o $(OBJECTS) -o $@

Script: Script.o $(OBJECTS)
	$(CC) $(LDFLAGS) Script.o $(OBJECTS) -o $@

%.o: %.cpp ascript_pch.hpp.gch
	$(CC) $(CPPFLAGS) $< -o $@

ascript_pch.hpp.gch: ascript_pch.hpp OS_support.h
	$(CC) $(CPPFLAGS) ascript_pch.hpp -o $@

clean:
	rm -f *.gch run_tests Script run_tests.o Script.o $(OBJECTS)
