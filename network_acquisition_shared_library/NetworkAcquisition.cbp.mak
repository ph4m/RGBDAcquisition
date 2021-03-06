#------------------------------------------------------------------------------#
# This makefile was generated by 'cbp2make' tool rev.147                       #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CC = gcc
CXX = g++
AR = ar
LD = g++
WINDRES = windres

INC = 
CFLAGS = -Wall -fPIC
RESINC = 
LIBDIR = 
LIB = 
LDFLAGS = -pthread -ljpeg -lpng

INC_DEBUG = $(INC)
CFLAGS_DEBUG = $(CFLAGS) -g
RESINC_DEBUG = $(RESINC)
RCFLAGS_DEBUG = $(RCFLAGS)
LIBDIR_DEBUG = $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG = $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG = 
OUT_DEBUG = libNetworkAcquisition.so

INC_RELEASE = $(INC)
CFLAGS_RELEASE = $(CFLAGS) -O2
RESINC_RELEASE = $(RESINC)
RCFLAGS_RELEASE = $(RCFLAGS)
LIBDIR_RELEASE = $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE = $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE = 
OUT_RELEASE = libNetworkAcquisition.so

OBJ_DEBUG = $(OBJDIR_DEBUG)/__/tools/Codecs/codecs.o $(OBJDIR_DEBUG)/__/tools/Codecs/jpgInput.o $(OBJDIR_DEBUG)/daemon.o $(OBJDIR_DEBUG)/main.o $(OBJDIR_DEBUG)/networkFramework.o

OBJ_RELEASE = $(OBJDIR_RELEASE)/__/tools/Codecs/codecs.o $(OBJDIR_RELEASE)/__/tools/Codecs/jpgInput.o $(OBJDIR_RELEASE)/daemon.o $(OBJDIR_RELEASE)/main.o $(OBJDIR_RELEASE)/networkFramework.o

all: debug release

clean: clean_debug clean_release

before_debug: 
	test -d $(OBJDIR_DEBUG)/__/tools/Codecs || mkdir -p $(OBJDIR_DEBUG)/__/tools/Codecs
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)

after_debug: 

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) -shared $(LIBDIR_DEBUG) $(OBJ_DEBUG)  -o $(OUT_DEBUG) $(LDFLAGS_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/__/tools/Codecs/codecs.o: ../tools/Codecs/codecs.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c ../tools/Codecs/codecs.c -o $(OBJDIR_DEBUG)/__/tools/Codecs/codecs.o

$(OBJDIR_DEBUG)/__/tools/Codecs/jpgInput.o: ../tools/Codecs/jpgInput.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c ../tools/Codecs/jpgInput.c -o $(OBJDIR_DEBUG)/__/tools/Codecs/jpgInput.o

$(OBJDIR_DEBUG)/daemon.o: daemon.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c daemon.c -o $(OBJDIR_DEBUG)/daemon.o

$(OBJDIR_DEBUG)/main.o: main.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c main.c -o $(OBJDIR_DEBUG)/main.o

$(OBJDIR_DEBUG)/networkFramework.o: networkFramework.c
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c networkFramework.c -o $(OBJDIR_DEBUG)/networkFramework.o

clean_debug: 
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf $(OBJDIR_DEBUG)/__/tools/Codecs
	rm -rf $(OBJDIR_DEBUG)

before_release: 
	test -d $(OBJDIR_RELEASE)/__/tools/Codecs || mkdir -p $(OBJDIR_RELEASE)/__/tools/Codecs
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)

after_release: 

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) -shared $(LIBDIR_RELEASE) $(OBJ_RELEASE)  -o $(OUT_RELEASE) $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/__/tools/Codecs/codecs.o: ../tools/Codecs/codecs.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c ../tools/Codecs/codecs.c -o $(OBJDIR_RELEASE)/__/tools/Codecs/codecs.o

$(OBJDIR_RELEASE)/__/tools/Codecs/jpgInput.o: ../tools/Codecs/jpgInput.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c ../tools/Codecs/jpgInput.c -o $(OBJDIR_RELEASE)/__/tools/Codecs/jpgInput.o

$(OBJDIR_RELEASE)/daemon.o: daemon.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c daemon.c -o $(OBJDIR_RELEASE)/daemon.o

$(OBJDIR_RELEASE)/main.o: main.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c main.c -o $(OBJDIR_RELEASE)/main.o

$(OBJDIR_RELEASE)/networkFramework.o: networkFramework.c
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c networkFramework.c -o $(OBJDIR_RELEASE)/networkFramework.o

clean_release: 
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf $(OBJDIR_RELEASE)/__/tools/Codecs
	rm -rf $(OBJDIR_RELEASE)

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release

