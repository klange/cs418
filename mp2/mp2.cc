/*
 * CS 418 MP2 - Kevin Lange <lange7@illinois.edu>
 *
 * Based on mountains.c
 *
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

float sealevel; /* Water level for the ocean plane */
float polysize; /* Scale of the map; probably best not to be changing this (though there are keybindings to do so) */

/* Rotation constant. Higher means slower */
#define DEG 30
/* Tolerance for close-enough normalization on quaternions */
#define TOL 0.00001

/*
 * Simple 3d vector class with normalization
 */
class Vector {
public:
	double x, y, z; /* Elements of the vector */
	Vector(double a, double b, double c) {
		x = a; y = b; z = c;
	}
	Vector() { }
	/* Vector addition */
	Vector operator+(Vector v) {
		return Vector(x+v.x,y+v.y,z+v.z);
	}
	/* Turn the vector into a unit vector */
	void normalize() {
		double mag_squared = x * x + y * y + z * z;
		if (fabs(mag_squared) > TOL && fabs(mag_squared - 1.0) > TOL) {
			double mag = sqrt(mag_squared);
			x /= mag;
			y /= mag;
			z /= mag;
		}
	}
};

/*
 * Quick Quaternion implementation
 *
 * (based on definitions for a quaternion from multiple sources,
 *  esp. Wikipedia)
 */
class Quaternion {
public:
	double w, x, y, z;
	Quaternion(double a, double b, double c, double d) {
		w = a; x = b; y = c; z = d;
	}
	Quaternion() { }
	/* Unit quaternions are the only safe way to rotate */
	void normalise() {
		double mag_squared = w * w + x * x + y * y + z * z;
		if (fabs(mag_squared) > TOL && fabs(mag_squared - 1.0) > TOL) {
			double mag = sqrt(mag_squared);
			w /= mag;
			x /= mag;
			y /= mag;
			z /= mag;
		}
	};
	Quaternion conjugate() const {
		return Quaternion(w,-x,-y,-z);
	}
	/* Quaternion multiplication... as if life weren't annoying
	 * enough as it is; look at all those multiplications! */
	Quaternion operator*(const Quaternion &rq) const {
		return Quaternion(
			w * rq.w - x * rq.x - y * rq.y - z * rq.z,
			w * rq.x + x * rq.w + y * rq.z - z * rq.y,
			w * rq.y + y * rq.w + z * rq.x - x * rq.z,
			w * rq.z + z * rq.w + x * rq.y - y * rq.x
			);
	}
	/* Quaternion * Vector, rotates a vector from the
	 * base coordinate system to the quaternion system */
	Vector operator*(const Vector &vec) const {
		Vector vn(vec.x,vec.y,vec.z);
		vn.normalize();
		Quaternion vecQuat, resQuat;
		vecQuat.x = vn.x;
		vecQuat.y = vn.y;
		vecQuat.z = vn.z;
		vecQuat.w = 0.0;
		resQuat = vecQuat * conjugate();
		resQuat = *this * resQuat;
		return Vector(resQuat.x,resQuat.y,resQuat.z);
	}
};

/*
 * Camera class
 *
 * For tracking locations and directions without
 * the chance for gimbal lock and other
 * annoyances that occur from other rotation systems
 */
class Camera {
	public:
	Quaternion rotation;	/* Direction */
	Vector pos;				/* Position */
	double speed;			/* Forward speed */
	Camera() {
		/* A default camera for this world */
		pos = Vector(0,0.05,0);
		rotation = Quaternion(1.0,0.0,0.0,0.0);
		rotation.normalise();
		speed = 0.01;
	}
	void forward(double t) {
		/* Translate forward in the direction we are facing */
		pos = pos + rotation * Vector(t, 0.0, 0.0);
	}
	void up(double t) {
		/* Pan up and down without respect to our own direction */
		pos.y += t;
	}
	void yaw(double r) {
		/* Rotate around the true vertical axis */
		Quaternion nrot(r,0.0,1.0,0.0);
		rotation = nrot * rotation;
		rotation.normalise();
	}
	void roll(double r) {
		/* Rotate around our local 'forward' axis */
		Vector v = rotation * Vector(1.0,0.0,0.0);
		Quaternion nrot(r,v.x,v.y,v.z);
		nrot.normalise();
		rotation = nrot * rotation;
		rotation.normalise();
	}
	void pitch(double r) {
		/* Rotate around our local x (sideways) axis */
		Vector v = rotation * Vector(0.0,0.0,1.0);
		Quaternion nrot(r,v.x,v.y,v.z);
		nrot.normalise();
		rotation = nrot * rotation;
		rotation.normalise();
	}
	void tick(double seconds) {
		/* Tick -> Move forward by speed * time elapsed */
		forward(speed * seconds);
	}
};

/* The scene's camera */
Camera cam;

/* Camera direction as a vector for call to gluLookAt */
double camera_dir[3] = {1.0,0.0,0.0};
/* Similarly, the up direction of the camera */
double camera_up[3]  = {0.0,0.0,1.0};

void updateVectors() {
	/* Tick forward in time */
	cam.tick(0.05);

	/* Camera Direction */
	Vector v = cam.rotation * Vector(1.0,0.0,0.0);
	camera_dir[0] = v.x;
	camera_dir[1] = v.z;
	camera_dir[2] = v.y;

	/* Camera roll */
	v = cam.rotation * Vector(0.0,1.0,0.0);
	camera_up[0] = v.x;
	camera_up[1] = v.z;
	camera_up[2] = v.y;
}

/* <<< Everything here is undocumented because it's not mine
 *     and I don't care to figure out exactly how it works.
 */
int seed(float x, float y) {
	static int a = 1588635695, b = 1117695901;
	int xi = *(int *)&x;
	int yi = *(int *)&y;
	return ((xi * a) % b) - ((yi * b) % a);
}

void mountain(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float s)
{
	float x01,y01,z01,x12,y12,z12,x20,y20,z20;

	glShadeModel(GL_SMOOTH);
	if (s < polysize) {
		x01 = x1 - x0;
		y01 = y1 - y0;
		z01 = z1 - z0;

		x12 = x2 - x1;
		y12 = y2 - y1;
		z12 = z2 - z1;

		x20 = x0 - x2;
		y20 = y0 - y2;
		z20 = z0 - z2;

		float nx = y01*(-z20) - (-y20)*z01;
		float ny = z01*(-x20) - (-z20)*x01;
		float nz = x01*(-y20) - (-x20)*y01;

		float den = sqrt(nx*nx + ny*ny + nz*nz);

		if (den > 0.0) {
			nx /= den;
			ny /= den;
			nz /= den;
		}

		glNormal3f(nx,ny,nz);
		glBegin(GL_TRIANGLES);
		glVertex3f(x0,y0,z0);
		glVertex3f(x1,y1,z1);
		glVertex3f(x2,y2,z2);
		glEnd();

		return;
	}

	x01 = 0.5*(x0 + x1);
	y01 = 0.5*(y0 + y1);
	z01 = 0.5*(z0 + z1);

	x12 = 0.5*(x1 + x2);
	y12 = 0.5*(y1 + y2);
	z12 = 0.5*(z1 + z2);

	x20 = 0.5*(x2 + x0);
	y20 = 0.5*(y2 + y0);
	z20 = 0.5*(z2 + z0);

	s *= 0.5;

	srand(seed(x01,y01));
	z01 += 0.3*s*(2.0*((float)rand()/(float)RAND_MAX) - 1.0);
	srand(seed(x12,y12));
	z12 += 0.3*s*(2.0*((float)rand()/(float)RAND_MAX) - 1.0);
	srand(seed(x20,y20));
	z20 += 0.3*s*(2.0*((float)rand()/(float)RAND_MAX) - 1.0);

	mountain(x0,y0,z0,x01,y01,z01,x20,y20,z20,s);
	mountain(x1,y1,z1,x12,y12,z12,x01,y01,z01,s);
	mountain(x2,y2,z2,x20,y20,z20,x12,y12,z12,s);
	mountain(x01,y01,z01,x12,y12,z12,x20,y20,z20,s);
}

void init(void) 
{

	glClearColor (0.5, 0.5, 1.0, 0.0);
	/* glShadeModel (GL_FLAT); */
	glEnable(GL_DEPTH_TEST);

	gluLookAt(0.0,0.0,0.0,
			  1.0,0.0,0.0,
			  0.0,0.0,1.0);
	sealevel = 0.0;
	polysize = 0.01;
}

/* >>> And now we're back to some of my code. */
void lights(void) {
	GLfloat white[] = {1.0,1.0,1.0,1.0};
	GLfloat lpos[] = {2.0,2.0,2.0,0.0};

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
}

void display(void)
{
	/* Camera Rotation Transformation */
	glLoadIdentity ();
	/* Update the camera */
	updateVectors();
	/* Point in the right direction, from the right point */
	gluLookAt (cam.pos.x,cam.pos.z,cam.pos.y,
			   cam.pos.x + camera_dir[0],
			   cam.pos.z + camera_dir[1],
			   cam.pos.y + camera_dir[2],
			   camera_up [0],camera_up [1],camera_up [2]);
	/* End Camera transformation */
	lights();

	/* <<< And not my code again */

	GLfloat tanamb[] = {0.2,0.15,0.1,1.0};
	GLfloat tandiff[] = {0.4,0.3,0.2,1.0};

	GLfloat seaamb[] = {0.0,0.0,0.2,1.0};
	GLfloat seadiff[] = {0.0,0.0,0.8,1.0};
	GLfloat seaspec[] = {0.5,0.5,1.0,1.0};


	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f (1.0, 1.0, 1.0);
	glTranslatef (-0.5, -0.5, 0.0);      /* modeling transformation */ 
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tanamb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tandiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tandiff);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0);

	mountain(0.0,0.0,0.0, 1.0,0.0,0.0, 0.0,1.0,0.0, 1.0);
	mountain(1.0,1.0,0.0, 0.0,1.0,0.0, 1.0,0.0,0.0, 1.0);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, seaamb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, seadiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, seaspec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0);

	glNormal3f(0.0,0.0,1.0);
	glBegin(GL_QUADS);
	glVertex3f(0.0,0.0,sealevel);
	glVertex3f(1.0,0.0,sealevel);
	glVertex3f(1.0,1.0,sealevel);
	glVertex3f(0.0,1.0,sealevel);
	glEnd();

	glutSwapBuffers();
	glFlush ();


	glutPostRedisplay();
}

void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	/* How about we actually adjust the aspect ratio to our display, hm? */
	gluPerspective(90.0,(double)w / (double)h,0.0001,10.0);
	glMatrixMode (GL_MODELVIEW);
}

/* >>> And welcome back once more */

/* Handle the arrow keys and (potentially) other special keys */
void arrowkeys(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_LEFT:
			/* Roll left */
			cam.roll(DEG);
			break;
		case GLUT_KEY_RIGHT:
			/* Roll right */
			cam.roll(-DEG);
			break;
		case GLUT_KEY_UP:
			/* Pitch "up" */
			cam.pitch(-DEG);
			break;
		case GLUT_KEY_DOWN:
			/* Pitch "down" */
			cam.pitch(DEG);
			break;
	}
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case '-':
			sealevel -= 0.01;
			break;
		case '+':
		case '=':
			sealevel += 0.01;
			break;
		case 'f':
			polysize *= 0.5;
			break;
		case 'c':
			polysize *= 2.0;
			break;
	/* New key bindings */
		case 'e':
			/* Move down */
			cam.up(-0.005);
			break;
		case 'r':
			/* Move up */
			cam.up(0.005);
			break;
		case 'w':
			/* Speed up */
			cam.speed += 0.005;
			break;
		case 's':
			/* Slow down */
			cam.speed -= 0.005;
			break;
		case 'a':
			/* Yaw left */
			cam.yaw(-DEG);
			break;
		case 'd':
			/* Yaw right */
			cam.yaw(DEG);
			break;
	/* End new keybindings */
		case 27:
			exit(0);
			break;
	}
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize (500, 500); 
	glutInitWindowPosition (100, 100);
	glutCreateWindow (argv[0]);
	init ();
	glutDisplayFunc(display); 
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(arrowkeys);
	glutMainLoop();
	return 0;
}
