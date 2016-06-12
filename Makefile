# architecture
ARCH = $(shell uname -p)
BITS = $(shell uname -m)
OS = $(shell uname) 
OSFLAVOR =

USERMACROS =
CFLAGS =
DFLAGS =
LFLAGS =
INSTALL =
EXE_BITS =
EXE_OS =
THREADED =
GIT_VERSION =
# build info
SRC := hedwig.cpp threads.cpp globals.cpp magic.cpp uci.cpp move.cpp board.cpp bench.cpp material.cpp pawns.cpp evaluate.cpp search2.cpp hashtable.cpp moveselect.cpp zobrist.cpp book.cpp
OBJS =$(SRCS:.cpp=.o)


#====================================
#  makefile options
#  
# compiler
ifeq ($(COMP),)
   CC = g++
else
   CC = $(COMP)
endif

# mode
ifeq ($(MODE),)
   MODE = debug
   USERMACROS += -DDEBUG
   CFLAGS += -Wall
   CFLAGS += -g
   CFLAGS += -ggdb
else
   MODE = release
   CFLAGS += -Wall -O3 -fomit-frame-pointer -fstrict-aliasing -fno-rtti -save-temps -std=gnu++11
   #CFLAGS += -pthread
   USERMACROS += -DNDEBUG
endif

# threads
ifeq ($(THREADED),)
   USERMACROS += -DTHREADED
   CFLAGS += -pthread
   LFLAGS += -lpthread -lrt
endif

# arch
ifeq ($(ARCH),i386)
   #CFLAGS += -march=$(ARCH)
   CFLAGS += -sse
   CFLAGS += -O2
endif

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
   USERMACROS += -DOS=\"unix\"
endif

ifeq ($(OS),Linux )
   EXE_OS = nix
   USERMACROS += -DOS=\"unix\"
endif


# executable
EXE = hedwig-$(EXE_OS)-$(EXE_BITS)

# git version/date
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
USERMACROS += -DBUILD_DATE="\"$$(date)\""
USERMACROS += -DVERSION=\"$(GIT_VERSION)\"

# collect object files here
OBJ := $(patsubst %.cpp, %.o, $(filter %.cpp,$(SRC)))

.PHONY:all
all: information link

information:
	@echo ".............................."
	@echo "Build Information : "
	@echo "...Building "$(MODE)" mode"
	@echo "...ARCH    = "$(ARCH)
	@echo "...BITS    = "$(BITS)
	@echo "...CC      = "$(CC)
	@echo "...OS      = "$(OS)
	@echo "...CFLAGS  = "$(CFLAGS)
	@echo "...MACROS  = "$(USERMACROS)
	@echo "...EXE     = "$(EXE)
	@echo "..............................."
	@echo ""
	@echo ""
#linking the program
link: $(OBJ)
	$(CC) -o $(EXE) $(OBJ) $(LFLAGS)

%.o:%.cpp
	$(CC) -c $(CFLAGS) $(USERMACROS) $< -o $@

install: all
	if [ ! -d $(INSTALL) ]; then \
		mkdir -p $(INSTALL); \
	fi 
	if [ ! -d $(LOG) ]; then \
		mkdir -p $(LOG); \
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
