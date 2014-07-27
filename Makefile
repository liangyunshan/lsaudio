TARGET  = lsaudio
DEFS  = -DMP3INFOLIB
CCOPTS =  -O2 -g -Wall
LIBS =  -lvorbis -lvorbisfile
CC = gcc

#-------------------------------------------------------------


$(TARGET): *.c
	$(CC) $(CCOPTS) $(DEFS)   -O3 $^ $(LIBS) -o $@

clean:
	@rm -rf *.o core
