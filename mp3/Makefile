.PHONY: all clean
all: mp3 mp3-rgba

clean:
	rm  -f mp3

mp3: mp3.c
	clang -Wall -lGLEW -lglut -lGLU -lGL -o mp3 mp3.c

mp3-rgba: mp3-rgba.c
	clang -Wall -lXi -lXrender -lGLEW -lglut -lGLU -lGL -o mp3-rgba mp3-rgba.c
