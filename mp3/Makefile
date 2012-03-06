.PHONY: all clean
all: mp3 mp3-rgba

clean:
	rm  -f mp3 mp3-rgba

mp3: mp3.c
	clang -Wall -o mp3 mp3.c -lGLEW -lglut -lGLU -lGL

mp3-rgba: mp3-rgba.c
	clang -Wall -o mp3-rgba mp3-rgba.c -lXi -lXrender -lGLEW -lglut -lGLU -lGL
