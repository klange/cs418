#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>

void disp();                                /* Render function */
void tick(int v);                           /* Timer tick */
void keyb(unsigned char key, int x, int y); /* Keboard handler */

static int win;            /* Window identifier */
static float time;         /* Current time */
static int wired_only = 0; /* Render wireframe only? */
static int paused = 0;     /* Pause timer ticks */

int main(int argc, char * argv[]) {
	/* Initialize GLUT */
	glutInit(&argc, argv);
	/* RGBA, Double-buffered, with depth buffer */
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	/* Initialize the window to be 500x500 at (100,100) */
	glutInitWindowSize(500,500);
	glutInitWindowPosition(100,100);
	/* Create the window; titled 'JIGGLE!' because it perfectly describes what the I is doing */
	win = glutCreateWindow("JIGGLE!");
	/* Register the display, keyboard, and timer functions */
	glutDisplayFunc(disp);
	glutKeyboardFunc(keyb);
	glutTimerFunc(17,tick,0); /* Tick every 17msecs */
	/* Set the clear color to blue */
	glClearColor(0.0,0.2,0.6,0.0);
	/* Set orthographic view */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-3.0,3.0,-3.0,3.0,-3.0,3.0);
	glMatrixMode(GL_MODELVIEW);
	/* Enable line antialiasing */
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND); /* (which requires blend mode to be enabled */
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2.0); /* Set the line width to 2 for thicker lines */
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST); /* And use the best available line hinting */
	glutMainLoop(); /* Start the loop */

	return 0;
}

void tick(int v) {
	/* tick the clock */
	if (!paused) {
		time += 0.2;
	}
	/* Re-render */
	glutPostRedisplay();
	/* And tick again in 17msecs */
	glutTimerFunc(17,tick,v);
}

void drawI(int how) {
	/* Perturbation levels */
	float sin_a = 0.2 * sin(time - 1.5);
	float sin_b = 0.2 * sin(time - 0.5);
	float sin_c = 0.2 * sin(time + 0.5);
	float sin_d = 0.2 * sin(time + 1.5);
	/* === */
	glBegin(how);
		glVertex3f(-1.5,2.5 + sin_a,0.0);
		glVertex3f(1.5,2.5 + sin_d,0.0);
		glVertex3f(1.5,1.5 + sin_d,0.0);
		glVertex3f(0.5,1.5 + sin_c,0.0);
		glVertex3f(-0.5,1.5 + sin_b,0.0);
		glVertex3f(-1.5,1.5 + sin_a,0.0);
	glEnd();
	/*  |  */
	glBegin(how);
		glVertex3f(-0.5,1.5 + sin_b,0.0);
		glVertex3f(0.5,1.5 + sin_c,0.0);
		glVertex3f(0.5,-1.5 + sin_c,0.0);
		glVertex3f(-0.5,-1.5 + sin_b,0.0);
	glEnd();
	/* === */
	glBegin(how);
		glVertex3f(-1.5,-2.5 + sin_a,0.0);
		glVertex3f(1.5,-2.5 + sin_d,0.0);
		glVertex3f(1.5,-1.5 + sin_d,0.0);
		glVertex3f(0.5,-1.5 + sin_c,0.0);
		glVertex3f(-0.5,-1.5 + sin_b,0.0);
		glVertex3f(-1.5,-1.5 + sin_a,0.0);
	glEnd();
}

void disp() {
	/* Clear the color buffer */
	glClear(GL_COLOR_BUFFER_BIT);
	if (!wired_only) {
		/* Render the filled-in polygons */
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glColor3f(1.0,0.5,0);
		drawI(GL_TRIANGLE_FAN);
		/* Render the lines to make the edges smooth */
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glColor3f(1.0,0.5,0);
		drawI(GL_LINE_LOOP);
	} else {
		/* Render the wireframe */
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glColor3f(1.0,0.5,0);
		drawI(GL_TRIANGLE_FAN);
	}
	/* Flip the back buffer */
	glutSwapBuffers();
}

void keyb(unsigned char key, int x, int y) {
	if (key == 'q') {
		glutDestroyWindow(win);
		exit(0);
	} else if (key == 'w') {
		wired_only = 1;
	} else if (key == 's') {
		wired_only = 0;
	} else if (key == 'p') {
		paused = !paused;
	}
}
