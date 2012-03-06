.PHONY: all clean
all: mp4 mp4-nonrgba

clean:
	rm  -f mp4 mp4-nonrgba

mp4: mp4.c
	clang -Wall -lXi -lXrender -lGLEW -lglut -lGLU -lGL -g -o mp4 mp4.c

mp4-nonrgba: mp4-nonrgba.c
	clang -Wall -lXi -lXrender -lGLEW -lglut -lGLU -lGL -g -o mp4-nonrgba mp4-nonrgba.c
