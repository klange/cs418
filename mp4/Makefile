.PHONY: all clean
all: mp4 mp4-nonrgba

clean:
	rm  -f mp4 mp4-nonrgba

mp4: mp4.c
	$(CC) -Wall -g -o mp4 mp4.c -lX11 -lXi -lXrender -lGLEW -lglut -lGLU -lGL -lm

mp4-nonrgba: mp4-nonrgba.c
	$(CC) -Wall -g -o mp4-nonrgba mp4-nonrgba.c -lX11 -lXi -lXrender -lGLEW -lglut -lGLU -lGL -lm
