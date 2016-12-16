CC=g++
CFLAGS=-c -g -Wall -rdynamic -D_DEBUG -std=c++11
LDFLAGS=-g -rdynamic

SOURCES=  \
TinyJS_Functions.cpp \
TinyJS_MathFunctions.cpp \
jsLexer.cpp \
utils.cpp \
JsVars.cpp \
jsParser.cpp \
ast.cpp \
parserResults.cpp \
microVM.cpp \
mvmCodegen.cpp \
scriptMain.cpp \
mvmFunctions.cpp

OBJECTS=$(SOURCES:.cpp=.o)

all: run_tests Script

run_tests: run_tests.o $(OBJECTS)
	$(CC) $(LDFLAGS) run_tests.o $(OBJECTS) -o $@

Script: Script.o $(OBJECTS)
	$(CC) $(LDFLAGS) Script.o $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f run_tests Script run_tests.o Script.o $(OBJECTS)
