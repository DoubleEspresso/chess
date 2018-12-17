# architecture
ARCH = $(shell uname -p)
BITS = $(shell uname -m)
OS = $(shell uname) 
OSFLAVOR =

USERMACROS = -DNDEBUG
CFLAGS = -Wall -pedantic -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -std=c++11
DFLAGS =
LFLAGS =
INSTALL = /usr/local/bin
GIT_VERSION =

# build info
SRC := main.cpp magics.cpp bitboards.cpp position.cpp uci.cpp

# compiler
ifeq ($(COMP),)
   CC = g++
else
   CC = $(COMP)
endif

# arch

# bits
ifeq ($(BITS),x86_64)
   EXE_BITS = 64
   CFLAGS += -m64
   USERMACROS += -DBIT_64
else
   EXE_BITS = 32
   CFLAGS += -m32
   USERMACROS += -DBIT_32
endif

# prefetch
ifeq ($(USE_PREFETCH),true)
   USERMACROS += -DUSE_PREFETCH
endif

# os
ifeq ($(OS),Darwin )
   EXE_OS = osx
   USERMACROS += -DOSX
endif

ifeq ($(OS),Linux )
   EXE_OS = nix
   USERMACROS += -DUnix
endif

# executable
EXE = chess.exe

# git version info
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
USERMACROS += -DBUILD_DATE="\"$$(date)\""
USERMACROS += -DVERSION=\"$(GIT_VERSION)\"

OBJ := $(patsubst %.cpp, %.o, $(filter %.cpp,$(SRC)))

.PHONY:all
all: information link

debug: CFLAGS += -g -ggdb
debug: USERMACROS:=$(filter-out -DNDEBUG, $(USERMACROS))
debug: USERMACROS += -DDEBUG
debug: all

information:
	@echo ".............................."
	@echo "...ARCH    = "$(ARCH)
	@echo "...BITS    = "$(BITS)
	@echo "...CC      = "$(CC)
	@echo "...OS      = "$(OS)
	@echo "...CFLAGS  = "$(CFLAGS)
	@echo "...MACROS  = "$(USERMACROS)
	@echo "...EXE     = "$(EXE)
	@echo "..............................."
	@echo ""

link: $(OBJ)
	$(CC) -o $(EXE) $(OBJ) $(LFLAGS)

%.o:%.cpp
	$(CC) -c $(CFLAGS) $(USERMACROS) $< -o $@

install: all
	if [ ! -d $(INSTALL) ]; then \
		mkdir -p $(INSTALL); \
	fi 
	mv $(EXE) $(INSTALL)
	find . -name "*.o" | xargs rm -vf 

TAGS:   $(SRC)
	etags $(SRC)

depend:
	makedepend -- $(DFLAGS) -- $(SRC)

.PHONY:clean
clean:
	find . -name "*.o" | xargs rm -vf
	find . -name "*.ii" | xargs rm -vf
	find . -name "*.s" | xargs rm -vf
	rm -vf *~ *#
