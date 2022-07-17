CC=gcc
CC_WIN64=x86_64-w64-mingw32-gcc
DLLTOOL_WIN64=x86_64-w64-mingw32-dlltool
IDIR=../ClassiCube/src
CFLAGS=-shared -ldl -fPIC -I$(IDIR)

C_SOURCES:=$(wildcard *.c)
BIN_DIR=../bin

GAME_CFLAGS_WIN64=-mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32
CFLAGS_WIN64=-s -shared -L . -lClassiCube -I$(IDIR)

linux: fakeUnicodeSupport.c
	$(CC) fakeUnicodeSupport.c -o $(BIN_DIR)/fakeUnicodeSupport.so $(CFLAGS)
win64: fakeUnicodeSupport.c
	$(CC_WIN64) $(IDIR)/*.c -o $(BIN_DIR)/ClassiCube.exe $(GAME_CFLAGS_WIN64)
	gendef $(BIN_DIR)/ClassiCube.exe 
	$(DLLTOOL_WIN64) -d ClassiCube.def -l libClassiCube.a -D $(BIN_DIR)/ClassiCube.exe
	$(CC_WIN64) fakeUnicodeSupport.c -o $(BIN_DIR)/fakeUnicodeSupport.dll $(CFLAGS_WIN64)
	rm libClassiCube.a ClassiCube.def