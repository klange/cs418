.PHONY: all clean
all: mp3 mp3-rgba

clean:
	rm  -f mp3 mp3-rgba

mp3: mp3.c
	$(CC) -Wall -o mp3 mp3.c -lGLEW -lglut -lGLU -lGL -lm

mp3-rgba: mp3-rgba.c
	$(CC) -Wall -o mp3-rgba mp3-rgba.c -lX11 -lXi -lXrender -lGLEW -lglut -lGLU -lGL -lm
