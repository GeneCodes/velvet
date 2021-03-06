CC = gcc
CFLAGS = -Wall
DEBUG = -g
LIBS = -lm
OPT = -O3
MAXKMERLENGTH=31
CATEGORIES=2
DEF = -D MAXKMERLENGTH=$(MAXKMERLENGTH) -D CATEGORIES=$(CATEGORIES)

SYSTEM = $(shell uname -s)

# Mac OS users: uncomment the following lines
# CFLAGS = -Wall -m64

# Sparc/Solaris users: uncomment the following line
# CFLAGS = -Wall -m64

ifdef BIGASSEMBLY
override DEF := $(DEF) -D BIGASSEMBLY
endif

ifdef VBIGASSEMBLY
override DEF := $(DEF) -D BIGASSEMBLY -D VBIGASSEMBLY
endif 	


ifdef LONGSEQUENCES
override DEF := $(DEF) -D LONGSEQUENCES
endif

# OpenMP 
ifdef OPENMP
override CFLAGS := $(CFLAGS) -fopenmp
endif

# Per library coverage
ifdef SINGLE_COV_CAT
override DEF := $(DEF) -D SINGLE_COV_CAT
endif

# For Windows compilation under mingw-w64
ifneq (,$(findstring MINGW,$(SYSTEM)))
WINOBJ = obj/windows/pagesize.o
override DEF := $(DEF) -D__USE_MINGW_ANSI_STDIO=1
endif

OBJ = obj/tightString.o obj/run.o obj/splay.o obj/splayTable.o obj/graph.o obj/run2.o obj/fibHeap.o obj/fib.o obj/concatenatedGraph.o obj/passageMarker.o obj/graphStats.o obj/correctedGraph.o obj/dfib.o obj/dfibHeap.o obj/recycleBin.o obj/readSet.o obj/binarySequences.o obj/shortReadPairs.o obj/locallyCorrectedGraph.o obj/graphReConstruction.o obj/roadMap.o obj/preGraph.o obj/preGraphConstruction.o obj/concatenatedPreGraph.o obj/readCoherentGraph.o obj/utility.o obj/kmer.o obj/scaffold.o obj/kmerOccurenceTable.o obj/allocArray.o obj/autoOpen.o $(WINOBJ)

VELVETH_OBJ = obj/tightString.o obj/run.o obj/recycleBin.o obj/splay.o obj/splayTable.o obj/readSet.o obj/binarySequences.o obj/utility.o obj/kmer.o obj/kmerOccurenceTable.o obj/autoOpen.o $(WINOBJ)
VELVETH_COLOR_OBJ = $(VELVETH_OBJ) obj/allocArray.o
VELVETG_OBJ = obj/tightString.o obj/graph.o obj/run2.o obj/fibHeap.o obj/fib.o obj/concatenatedGraph.o obj/passageMarker.o obj/graphStats.o obj/correctedGraph.o obj/dfib.o obj/dfibHeap.o obj/recycleBin.o obj/readSet.o obj/binarySequences.o obj/shortReadPairs.o obj/scaffold.o obj/locallyCorrectedGraph.o obj/graphReConstruction.o obj/roadMap.o obj/preGraph.o obj/preGraphConstruction.o obj/concatenatedPreGraph.o obj/readCoherentGraph.o obj/utility.o obj/kmer.o obj/kmerOccurenceTable.o obj/allocArray.o obj/autoOpen.o $(WINOBJ)

OBJDBG = $(subst obj,obj/dbg,$(OBJ))
VELVETH_OBJDBG = $(subst obj,obj/dbg,$(VELVETH_OBJ))
VELVETH_COLOR_OBJDBG = $(subst obj,obj/dbg,$(VELVETH_OBJ))
VELVETG_OBJDBG = $(subst obj,obj/dbg,$(VELVETG_OBJ))

default : cleanobj zlib obj velveth velvetg doc

clean : clean-zlib
	-rm obj/*.o obj/dbg/*.o ./velvet*
	-rm obj/windows/*.o obj/dbg/windows/*.o
	-rm -f doc/manual_src/Manual.toc doc/manual_src/Manual.aux doc/manual_src/Manual.out doc/manual_src/Manual.log
	-rm -f doc/manual_src/Columbus_manual.aux doc/manual_src/Columbus_manual.out doc/manual_src/Columbus_manual.log

cleanobj: 
	-rm obj/*.o obj/dbg/*.o
	-rm obj/windows/*.o obj/dbg/windows/*.o

ifdef BUNDLEDZLIB
Z_LIB_DIR=third-party/zlib-1.2.3
Z_LIB_FILES=$(Z_LIB_DIR)/*.o
override DEF := $(DEF) -D BUNDLEDZLIB

zlib: 
	cd $(Z_LIB_DIR); ./configure; make; rm minigzip.o; rm example.o

clean-zlib :
	cd $(Z_LIB_DIR) && make clean
else
Z_LIB_FILES=-lz

zlib :
clean-zlib :
endif

velveth : obj 
	$(CC) $(CFLAGS) $(OPT) $(LDFLAGS) -o velveth $(VELVETH_OBJ) $(Z_LIB_FILES) $(LIBS)


velvetg : obj
	$(CC) $(CFLAGS) $(OPT) $(LDFLAGS) -o velvetg $(VELVETG_OBJ) $(Z_LIB_FILES) $(LIBS)

debug : override DEF := $(DEF) -D DEBUG 
debug : cleanobj obj/dbg
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o velveth $(VELVETH_OBJDBG) $(Z_LIB_FILES) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o velvetg $(VELVETG_OBJDBG) $(Z_LIB_FILES) $(LIBS)

color : override DEF := $(DEF) -D COLOR
color : cleanobj obj_de
	$(CC) $(CFLAGS) $(OPT) $(LDFLAGS) -o velveth_de $(VELVETH_COLOR_OBJ) $(Z_LIB_FILES) $(LIBS)
	$(CC) $(CFLAGS) $(OPT) $(LDFLAGS) -o velvetg_de $(VELVETG_OBJ) $(Z_LIB_FILES) $(LIBS)

colordebug : override DEF := $(DEF) -D COLOR -D DEBUG
colordebug : cleanobj obj/dbg_de
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o velveth_de $(VELVETH_COLOR_OBJDBG) $(Z_LIB_FILES) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o velvetg_de $(VELVETG_OBJDBG) $(Z_LIB_FILES) $(LIBS)

objdir:
	mkdir -p obj
ifneq (,$(findstring MINGW,$(SYSTEM)))
	mkdir -p obj/windows
endif

obj: zlib cleanobj objdir $(OBJ)

obj_de: override DEF := $(DEF) -D COLOR
obj_de: zlib cleanobj objdir $(OBJ)

obj/dbgdir: 
	mkdir -p obj/dbg
ifneq (,$(findstring MINGW,$(SYSTEM)))
	mkdir -p obj/dbg/windows
endif

obj/dbg: override DEF := $(DEF) -D DEBUG 
obj/dbg: zlib cleanobj obj/dbgdir $(OBJDBG)

obj/dbg_de: override DEF := $(DEF) -D COLOR -D DEBUG
obj/dbg_de: zlib cleanobj obj/dbgdir $(OBJDBG)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) $(OPT) $(DEF) -c $? -o $@ 

obj/dbg/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEBUG) $(DEF) -c $? -o $@ 

doc: Manual.pdf

Manual.pdf: doc/manual_src/Manual.tex doc/manual_src/Columbus_manual.tex
	cd doc/manual_src; pdflatex Manual.tex; pdflatex Manual.tex; mv Manual.pdf ../..; pdflatex Columbus_manual.tex; mv Columbus_manual.pdf ../..

test: velvetg velveth
	cd tests && ./run-tests.sh
