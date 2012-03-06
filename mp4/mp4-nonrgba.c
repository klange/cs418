/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * CS 418 MP4 - Kevin Lange <lange7@illinois.edu>
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/extensions/Xrender.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glx.h>

#define PI  3.141592654
#define TAO 6.28318531
#define GRAVITY 0.000007
#define GROUND_LEVEL -2.0

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints, MwmHints;

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define WIN_WIDTH 450
#define WIN_HEIGHT 450

int win_width;
int win_height;
float x_light;
float y_light;

/* GLX double buffering attributes */
int doubleBufferAttributes[] = {
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE,   GLX_RGBA_BIT,
	GLX_DOUBLEBUFFER,  True,
	GLX_RED_SIZE,      1,
	GLX_GREEN_SIZE,    1,
	GLX_BLUE_SIZE,     1,
	GLX_ALPHA_SIZE,    1,
	GLX_DEPTH_SIZE,    1,
	None
};

/* Wait until X notify event */
Bool wait_for_notify (Display* pDisplay,
					  XEvent* pEvent,
					  XPointer arg)
{
	return (pEvent->type == MapNotify) && (pEvent->xmap.window == (Window) arg);
}

/* Xorg stuff */
Display* pDisplay = NULL;
Window window;
GLXWindow glxWindow;

/* Find and apply the RGBA GLX visual */
int findRGBAVisual() {
	XEvent event;
	XVisualInfo* pVisInfo;
	XSetWindowAttributes windowAttribs;
	GLXFBConfig* pFBConfigs;
	GLXFBConfig renderFBConfig;
	GLXContext glxContext;
	int windowAttribsMask;
	int iNumOfFBConfigs;
	int iRenderEventBase;
	int iRenderErrorBase;
	int i;
	XRenderPictFormat* pPictFormat = NULL;
	Bool bKeepGoing = False;
	Atom hintsAtom = None;
	unsigned char* pucData;
	MotifWmHints newHints;
	Atom typeAtom;
	int iFormat;
	unsigned long ulItems;
	unsigned long ulBytesAfter;

	bzero ((void*) &newHints, sizeof (MotifWmHints));

	/* open a connection to the X server */
	pDisplay = XOpenDisplay (NULL);
	if (pDisplay == NULL)
	{
		fprintf (stderr, "Unable to open a connection to the X server!\n");
		return 1;
	}

	/* test for the RENDER extension */
	if (!XRenderQueryExtension (pDisplay, &iRenderEventBase, &iRenderErrorBase))
	{
		fprintf (stderr, "Damn, no RENDER extension found!\n");
		return 2;
	}

	/* look for a framebuffer configuration, only try to get a double
	** buffered config */
	pFBConfigs = glXChooseFBConfig (pDisplay,
									DefaultScreen (pDisplay),
									doubleBufferAttributes,
									&iNumOfFBConfigs);

	if (pFBConfigs == NULL)
	{
		fprintf (stderr, "Argl, we could not get an ARGB-visual!\n");
		return 3;
	}

	/* try to find a GLX visual with alpha from the list of fb-configs */
	for (i = 0; i < iNumOfFBConfigs; i++)
	{
		pVisInfo = glXGetVisualFromFBConfig (pDisplay, pFBConfigs[i]);
        if (!pVisInfo)
			continue;

        pPictFormat = XRenderFindVisualFormat (pDisplay, pVisInfo->visual);
        if (!pPictFormat)
			continue;

		if (pPictFormat->direct.alphaMask > 0 || 1)
		{
			fprintf (stderr, "Strike, found a GLX visual with alpha-support!\n");
			pVisInfo = glXGetVisualFromFBConfig (pDisplay, pFBConfigs[i]);
			renderFBConfig = pFBConfigs[i];
			bKeepGoing = True;
			break;
		}

		XFree (pVisInfo);
    }

	if (!bKeepGoing)
	{
		fprintf (stderr, "Crap, was not able to acquire a Render-compatible GLX-visual!\n");
		return 4;
	}

	/* set some window-attributes before... */
	windowAttribs.border_pixel = 0;
	windowAttribs.event_mask = StructureNotifyMask |
							   ButtonPressMask |
							   ButtonReleaseMask |
							   KeyPressMask |
							   PointerMotionMask;
	windowAttribs.colormap = XCreateColormap (pDisplay,
											  RootWindow (pDisplay,
														  pVisInfo->screen),
											  pVisInfo->visual,
											  AllocNone);

	windowAttribsMask = CWBorderPixel | CWColormap | CWEventMask;

	/* ... creating our application window */
	window = XCreateWindow (pDisplay,
							RootWindow (pDisplay, pVisInfo->screen),
							0,
							0,
							WIN_WIDTH,
							WIN_HEIGHT,
							0,
							pVisInfo->depth,
							InputOutput,
							pVisInfo->visual,
							windowAttribsMask,
							&windowAttribs);

	/* create a GLX context for OpenGL rendering */
	glxContext = glXCreateNewContext (pDisplay,
									  renderFBConfig,
									  GLX_RGBA_TYPE,
									  NULL,
									  True);

	/* make sure to get a GLX window with the fb-config with render-support */
	glxWindow = glXCreateWindow (pDisplay, renderFBConfig, window, NULL);

	/* get rid of any window decorations */
	hintsAtom = XInternAtom (pDisplay, "_MOTIF_WM_HINTS", True);
	XGetWindowProperty (pDisplay,
						window,
						hintsAtom,
						0,
						sizeof (MotifWmHints) / sizeof (long),
						False,
						AnyPropertyType,
						&typeAtom,
						&iFormat,
						&ulItems,
						&ulBytesAfter,
						&pucData);

	newHints.flags = MWM_HINTS_DECORATIONS;
	newHints.decorations = 0;

	XChangeProperty (pDisplay,
					 window,
					 hintsAtom,
					 hintsAtom,
					 32,
					 PropModeReplace,
					 (unsigned char *) &newHints,
					 sizeof (MotifWmHints) / sizeof (long));

	/* show the window on the display... */
	XMapWindow (pDisplay, window);

	/* ... and wait for it to appear */
	XIfEvent (pDisplay, &event, wait_for_notify, (XPointer) window);

	/* ensure that our GL-context is the one we'll draw to */
	glXMakeContextCurrent (pDisplay, glxWindow, glxWindow, glxContext);

	return 0;
}

/* Scene scale */
float scale = 1.0;
/* Camera position */
float cam_x = 1.0;
float cam_y = 1.0;
float cam_z = 1.0;
float time  = 0.0;
/* Where to point the camera */
float cam_offset = 1.0;

char rotation_paused = 0;

/* Normal vector definition */
typedef struct {
	float x;
	float y;
	float z;
} normal_t ;

/* Vertex object */
typedef struct {
	float x; /* Coordinates */
	float y;
	float z;
	float u; /* Texture coordinates */
	float v;
	float v_x;
	float v_y;
	float v_z;
	float a_x;
	float a_y;
	float a_z;
	normal_t normal;
} vertex_t ;

/* Resizable array of vertices */
typedef struct {
	uint32_t len;
	uint32_t capacity;
	vertex_t ** nodes;
} vertices_t;

/* Face definition (3 vertices and a normal vector) */
typedef struct {
	vertex_t * a;
	vertex_t * b;
	vertex_t * c;
	normal_t normal;
} face_t;

/* Resizable array of faces */
typedef struct {
	uint32_t len;
	uint32_t capacity;
	face_t ** nodes;
} faces_t;

/* Spring structure */
typedef struct {
	vertex_t * start;
	vertex_t * end;
	double rest_length;
	double k;
} spring_t;

/* Resizable array of springs */
typedef struct {
	uint32_t len;
	uint32_t capacity;
	spring_t ** nodes;
} springs_t;

void add_spring(int a, int b, double k);

/* Model vertices */
vertices_t vertices = {.len = 0, .capacity = 0};
/* Model triangles */
faces_t    faces    = {.len = 0, .capacity = 0};
/* Model springs */
springs_t  springs  = {.len = 0, .capacity = 0};

/* Initialize the model objects */
void init_model() {
	/* We give an initial capacity of 16 for each */
	vertices.capacity = 16;
	vertices.nodes    = (vertex_t **)malloc(sizeof(vertex_t *) * vertices.capacity);
	faces.capacity    = 16;
	faces.nodes       = (face_t **)malloc(sizeof(face_t *) * faces.capacity);
	springs.capacity  = 16;
	springs.nodes     = (spring_t **)malloc(sizeof(spring_t *) * springs.capacity);
}

float offset_x = 0.0f, offset_y = 0.0f, offset_z = 0.0f;

/* Add the given coordinates to the vertex list */
int add_vertex(float x, float y, float z) {
	if (vertices.len == vertices.capacity) {
		/* When we run out of space, increase by two */
		vertices.capacity *= 2;
		vertices.nodes = (vertex_t **)realloc(vertices.nodes, sizeof(vertex_t *) * vertices.capacity);
	}
	/* Create a new vertex in the list */
	vertices.nodes[vertices.len] = calloc(sizeof(vertex_t),1);
	vertices.nodes[vertices.len]->x = x * scale + offset_x;
	vertices.nodes[vertices.len]->y = y * scale + offset_y;
	vertices.nodes[vertices.len]->z = z * scale + offset_z;
	/* Set texture coordinates by cylindrical mapping */
	float theta = atan2(z,x);
	vertices.nodes[vertices.len]->u = (theta + PI) / (TAO);
	vertices.nodes[vertices.len]->v = (y / 2.0);
	/* Initialize normals to 0,0,0 */
	vertices.nodes[vertices.len]->normal.x = 0.0f;
	vertices.nodes[vertices.len]->normal.y = 0.0f;
	vertices.nodes[vertices.len]->normal.z = 0.0f;
	vertices.len++;
	return vertices.len;
}

/* Add a face with the given vertices */
void add_face(int a, int b, int c) {
	if (faces.len == faces.capacity) {
		/* Double size when we run out... */
		faces.capacity *= 2;
		faces.nodes = (face_t **)realloc(faces.nodes, sizeof(face_t *) * faces.capacity);
	}
	if (vertices.len < a || vertices.len < b || vertices.len < c) {
		/* Invalid vertex indexes... */
		fprintf(stderr, "ERROR: Haven't yet collected enough vertices for the face %d %d %d (have %d)!\n", a, b, c, vertices.len);
		exit(1);
	}
	/* Create a new triangle */
	faces.nodes[faces.len] = malloc(sizeof(face_t));
	faces.nodes[faces.len]->a = vertices.nodes[a-1];
	faces.nodes[faces.len]->b = vertices.nodes[b-1];
	faces.nodes[faces.len]->c = vertices.nodes[c-1];
	faces.len++;
}

/* Calculate the distance between two points in 3D */
double distance(vertex_t * s, vertex_t * e) {
	return sqrt((s->x - e->x) * (s->x - e->x)
			   +(s->y - e->y) * (s->y - e->y)
			   +(s->z - e->z) * (s->z - e->z));
}

/* Add a spring between the given (1-indexed) vertex numbers with the given spring constant */
void add_spring(int a, int b, double k) {
	if (springs.len == springs.capacity) {
		/* Double size when we need more room... */
		springs.capacity *= 2;
		springs.nodes = (spring_t **)realloc(springs.nodes, sizeof(spring_t *) * springs.capacity);
	}
	if (vertices.len < a || vertices.len < b) {
		/* Invalid vertex indexes */
		fprintf(stderr, "ERROR: Haven't yet collected enough vertices for the spring %d~%d [%f] (have %d)!\n", a, b, k, vertices.len);
		exit(2);
	}
	/* Create a new spring */
	springs.nodes[springs.len] = malloc(sizeof(spring_t));
	springs.nodes[springs.len]->start = vertices.nodes[a-1];
	springs.nodes[springs.len]->end   = vertices.nodes[b-1];
	vertex_t * s = springs.nodes[springs.len]->start;
	vertex_t * e = springs.nodes[springs.len]->end;
	/* The rest-length of a spring is the distance between its start
	 * and end points when it is initialized */
	springs.nodes[springs.len]->rest_length = distance(s,e);
	/* K is the spring constant for this spring */
	springs.nodes[springs.len]->k = k;
	springs.len++;
}

void finish_normals() {
	/* Loop through vertices and accumulate normals for them */
	for (uint32_t i = 0; i < faces.len; ++i) {
		/* Calculate some normals */
		vertex_t u = {.x = faces.nodes[i]->b->x - faces.nodes[i]->a->x,
					  .y = faces.nodes[i]->b->y - faces.nodes[i]->a->y,
					  .z = faces.nodes[i]->b->z - faces.nodes[i]->a->z};
		vertex_t v = {.x = faces.nodes[i]->c->x - faces.nodes[i]->a->x,
					  .y = faces.nodes[i]->c->y - faces.nodes[i]->a->y,
					  .z = faces.nodes[i]->c->z - faces.nodes[i]->a->z};
		/* Set the face normals */
		faces.nodes[i]->normal.x = ((u.y * v.z) - (u.z * v.y));
		faces.nodes[i]->normal.y = -((u.z * v.x) - (u.x * v.z));
		faces.nodes[i]->normal.z = ((u.x * v.y) - (u.y * v.x));
	}
	for (uint32_t i = 0; i < vertices.len; ++i) {
		/* Reset the normals */
		vertices.nodes[i]->normal.x = 0.0;
		vertices.nodes[i]->normal.y = 0.0;
		vertices.nodes[i]->normal.z = 0.0;
	}
	for (uint32_t i = 0; i < faces.len; ++i) {
		/* vertex a */
		faces.nodes[i]->a->normal.x += faces.nodes[i]->normal.x;
		faces.nodes[i]->a->normal.y += faces.nodes[i]->normal.y;
		faces.nodes[i]->a->normal.z += faces.nodes[i]->normal.z;
		/* vertex b */
		faces.nodes[i]->b->normal.x += faces.nodes[i]->normal.x;
		faces.nodes[i]->b->normal.y += faces.nodes[i]->normal.y;
		faces.nodes[i]->b->normal.z += faces.nodes[i]->normal.z;
		/* vertex c */
		faces.nodes[i]->c->normal.x += faces.nodes[i]->normal.x;
		faces.nodes[i]->c->normal.y += faces.nodes[i]->normal.y;
		faces.nodes[i]->c->normal.z += faces.nodes[i]->normal.z;
	}
}

/* Discard the rest of this line */
void toss(FILE * f) {
	while (fgetc(f) != '\n');
}

/* Load a Wavefront Obj model */
void load_wavefront(char * filename) {
	/* Open the file */
	FILE * obj = fopen(filename, "r");
	int collected = 0;
	char d = ' ';
	int f = 0;
	while (!feof(obj)) {
		/* Scan in a line */
		collected = fscanf(obj, "%c ", &d);
		if (collected == 0) continue;
		switch (d) {
			case 'v':
				{
					/* Vertex */
					float x, y, z;
					collected = fscanf(obj, "%f %f %f\n", &x, &y, &z);
					if (collected < 3) fprintf(stderr, "ERROR: Only collected %d points!\n", collected);
					int r = add_vertex(x, y, z);
					if (!f) { f = r; }
				}
				break;
			case 'f':
				{
					/* Face */
					int a, b, c;
					collected = fscanf(obj, "%d %d %d\n", &a, &b, &c);
					if (collected < 3) fprintf(stderr, "ERROR: Only collected %d vertices!\n", collected);
					add_face(f + a - 1,f + b - 1,f + c - 1);
					add_spring(f + a - 1, f + b - 1, 0.2);
					add_spring(f + c - 1, f + b - 1, 0.2);
					add_spring(f + a - 1, f + c - 1, 0.2);
				}
				break;
			default:
				/* Something else that we don't care about */
				toss(obj);
				break;
		}
	}
	/* Finalize the vertex normals */
	finish_normals();
	fclose(obj);
}

void generateI() {
	/* 
	  1"""""2
	  2 1 4 3
	    | |
	  9 0 5 6
	  8_____7
	
	  I am not going to explain each line here. We are generating a
	  block I and applying springs to keep it together.
	*/
	int f01 = add_vertex(-1.5,  2.0, -0.5); /* 1 */
	int f02 = add_vertex( 1.5,  2.0, -0.5);
	int f03 = add_vertex( 1.5,  1.0, -0.5);
	int f04 = add_vertex( 0.5,  1.0, -0.5);
	int f05 = add_vertex( 0.5, -1.0, -0.5); /* 5 */
	int f06 = add_vertex( 1.5, -1.0, -0.5);
	int f07 = add_vertex( 1.5, -2.0, -0.5);
	int f08 = add_vertex(-1.5, -2.0, -0.5);
	int f09 = add_vertex(-1.5, -1.0, -0.5);
	int f10 = add_vertex(-0.5, -1.0, -0.5); /* 10 */
	int f11 = add_vertex(-0.5,  1.0, -0.5);
	int f12 = add_vertex(-1.5,  1.0, -0.5);

	add_face(f01,f02,f12);
	add_face(f02,f11,f12);
	add_face(f02,f03,f04);
	add_face(f11,f02,f04);
	add_face(f11,f04,f05);
	add_face(f11,f05,f10);
	add_face(f07,f08,f09);
	add_face(f07,f05,f06);
	add_face(f10,f05,f07);
	add_face(f09,f10,f07);

	int b01 = add_vertex(-1.5,  2.0, 0.5); /* 1 */
	int b02 = add_vertex( 1.5,  2.0, 0.5);
	int b03 = add_vertex( 1.5,  1.0, 0.5);
	int b04 = add_vertex( 0.5,  1.0, 0.5);
	int b05 = add_vertex( 0.5, -1.0, 0.5); /* 5 */
	int b06 = add_vertex( 1.5, -1.0, 0.5);
	int b07 = add_vertex( 1.5, -2.0, 0.5);
	int b08 = add_vertex(-1.5, -2.0, 0.5);
	int b09 = add_vertex(-1.5, -1.0, 0.5);
	int b10 = add_vertex(-0.5, -1.0, 0.5); /* 10 */
	int b11 = add_vertex(-0.5,  1.0, 0.5);
	int b12 = add_vertex(-1.5,  1.0, 0.5);

	add_face(b12,b02,b01);
	add_face(b12,b11,b02);
	add_face(b04,b03,b02);
	add_face(b04,b02,b11);
	add_face(b05,b04,b11);
	add_face(b10,b05,b11);
	add_face(b09,b08,b07);
	add_face(b06,b05,b07);
	add_face(b07,b05,b10);
	add_face(b09,b07,b10);

	add_face(b09,f10,f09);
	add_face(b09,b10,f10);

	add_face(b05,f06,f05);
	add_face(b05,b06,f06);

	add_face(b09,f09,f08);
	add_face(b08,b09,f08);

	add_face(f08,f07,b07);
	add_face(f08,b07,b08);

	add_face(f07,f06,b06);
	add_face(f07,b06,b07);

	add_face(f11,f10,b10);
	add_face(f11,b10,b11);

	add_face(f04,b05,f05);
	add_face(f04,b04,b05);

	add_face(b02,f02,f01);
	add_face(b01,b02,f01);

	add_face(f04,f03,b03);
	add_face(f04,b03,b04);

	add_face(b12,f12,f11);
	add_face(b11,b12,f11);

	add_face(f03,f02,b02);
	add_face(f03,b02,b03);

	add_face(b01,f01,f12);
	add_face(b12,b01,f12);

	add_spring(f09,b09,0.0006);
	add_spring(f10,b10,0.0006);
	add_spring(f10,f09,0.0006);
	add_spring(b10,b09,0.0006);
	add_spring(f09,b10,0.0006);
	add_spring(f10,b09,0.0006);

	add_spring(f05,b05,0.0006);
	add_spring(f06,b06,0.0006);
	add_spring(f06,f05,0.0006);
	add_spring(b06,b05,0.0006);
	add_spring(f05,b06,0.0006);
	add_spring(f06,b05,0.0006);

	add_spring(f04,f05,0.0006);
	add_spring(f10,f11,0.0006);
	add_spring(f11,f04,0.0006);
	add_spring(f10,f05,0.0006);
	add_spring(f10,f04,0.0006);
	add_spring(f11,f05,0.0006);

	add_spring(b04,b05,0.0006);
	add_spring(b10,b11,0.0006);
	add_spring(b11,b04,0.0006);
	add_spring(b10,b05,0.0006);
	add_spring(b10,b04,0.0006);
	add_spring(b11,b05,0.0006);

	add_spring(f04,b04,0.0006);
	add_spring(f05,b05,0.0006);
	add_spring(f10,b10,0.0006);
	add_spring(f11,b11,0.0006);

	add_spring(f01,f02,0.0006);
	add_spring(f01,f12,0.0006);
	add_spring(f02,f03,0.0006);
	add_spring(f12,f03,0.0006);
	add_spring(f12,f02,0.0006);
	add_spring(f01,f03,0.0006);

	add_spring(b01,b02,0.0006);
	add_spring(b01,b12,0.0006);
	add_spring(b02,b03,0.0006);
	add_spring(b12,b03,0.0006);
	add_spring(b12,b02,0.0006);
	add_spring(b01,b03,0.0006);

	add_spring(f01,b01,0.0006);
	add_spring(f02,b02,0.0006);
	add_spring(f03,b03,0.0006);
	add_spring(f12,b12,0.0006);

	add_spring(f03,f04,0.0006);
	add_spring(f02,f04,0.0006);
	add_spring(f12,f11,0.0006);
	add_spring(f01,f11,0.0006);

	add_spring(b03,b04,0.0006);
	add_spring(b02,b04,0.0006);
	add_spring(b12,b11,0.0006);
	add_spring(b01,b11,0.0006);

	add_spring(b03,f04,0.0006);
	add_spring(b02,f04,0.0006);
	add_spring(b12,f11,0.0006);
	add_spring(b01,f11,0.0006);

	add_spring(f03,b04,0.0006);
	add_spring(f02,b04,0.0006);
	add_spring(f12,b11,0.0006);
	add_spring(f01,b11,0.0006);

	add_spring(f08,f07,0.0006);
	add_spring(f09,f06,0.0006);
	add_spring(f09,f07,0.0006);
	add_spring(f08,f06,0.0006);

	add_spring(b08,b07,0.0006);
	add_spring(b09,b06,0.0006);
	add_spring(b09,b07,0.0006);
	add_spring(b08,b06,0.0006);

	add_spring(f08,b09,0.0006);
	add_spring(f09,b08,0.0006);

	add_spring(f06,b07,0.0006);
	add_spring(f07,b06,0.0006);

	add_spring(f08,b05,0.0006);
	add_spring(f07,b10,0.0006);
	add_spring(b08,f05,0.0006);
	add_spring(b07,f10,0.0006);

	add_spring(f08,f05,0.0006);
	add_spring(f07,f10,0.0006);
	add_spring(f08,f10,0.0006);
	add_spring(f07,f05,0.0006);

	add_spring(b08,b05,0.0006);
	add_spring(b07,b10,0.0006);
	add_spring(b08,b10,0.0006);
	add_spring(b07,b05,0.0006);

	add_spring(f08,b07,0.0006);
	add_spring(b08,f07,0.0006);

	add_spring(f05,b01,0.0006);
	add_spring(f08,b02,0.0006);
	add_spring(b05,f01,0.0006);
	add_spring(b08,f02,0.0006);

	finish_normals();
}

/* Vertex, fragment, program */
GLuint v, f, p;

/* Read a file into a buffer and return a pointer to the buffer */
char * readFile(char * filename, int * size) {
	FILE * tex;
	char * texture;
	tex = fopen(filename, "r");
	fseek(tex, 0L, SEEK_END);
	*size = ftell(tex);
	texture = malloc(*size);
	fseek(tex, 0L, SEEK_SET);
	fread(texture, *size, 1, tex);
	fclose(tex);
	return texture;
}

/* Set the offset for adding vertices to the scene */
void set_offset(float a, float b, float c) {
	offset_x = a;
	offset_y = b;
	offset_z = c;
}

/* Initialize the scene */
void init() {
	/* Initialize the tables */
	init_model();
	/* Add an I a bit above the ground */
	set_offset(0.0,2.0,0.0);
	//load_wavefront("teapot.obj");
	set_offset(0.0,0.5,0.0);
	generateI();
	/* Check for GLEW compatibility */
	glewInit();
	if (glewIsSupported("GL_VERSION_2_0")) {
		printf("Ready.\n");
	} else {
		/* We don't have OpenGL 2.0 support! BAIL! */
		printf("wtf?\n");
		exit(1);
	}

	float fBGAlpha = 0.0f; /* background-opacity */
	float afBGColor[] = {0.0f, 0.0f, 0.0f}; /* background-color itself */

	/* the Render-extention expects premultiplied alpha colors */
	glClearColor (fBGAlpha * afBGColor[0],
				  fBGAlpha * afBGColor[1],
				  fBGAlpha * afBGColor[2],
				  fBGAlpha);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Some nice defaults */
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	/* Load in the shader programs */
	char *vs = NULL,
		 *fs = NULL;
	int32_t v_size, f_size;
	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);
	vs = readFile("blocki.vert", &v_size); /* Vertex shader */
	fs = readFile("blocki.frag", &f_size); /* Fragment shader */
	glShaderSource(v, 1, &vs, &v_size); /* Load... */
	glShaderSource(f, 1, &fs, &f_size);
	free(vs); free(fs); /* Free the data blobs */
	glCompileShader(v); /* Compile... */
	glCompileShader(f);
	p = glCreateProgram(); /* Create a program */
	glAttachShader(p, v);  /* Attach the two shaders */
	glAttachShader(p, f);
	glLinkProgram(p);      /* Link it all together */

	/* Use our shaders */
	glUseProgram(p);

	/* Check for errors */
	GLenum glErr;
	int    retCode = 0;
	glErr = glGetError();
	while (glErr != GL_NO_ERROR)
	{
		printf("glError: %s\n", gluErrorString(glErr));
		retCode = 1;
		glErr = glGetError();
	}


}

void lights(void) {
	/* Basic moving lighting */
	GLfloat white[] = {1.0,1.0,1.0,1.0};
	float l_scale = 200.0;
	/* The light position is based on the mouse cursor location */
	GLfloat lpos[] = {l_scale * x_light, l_scale * y_light, 20.0};

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
}

/* Normalize a vector (passed as a normal) */
void normalize(normal_t * n) {
	double d = sqrt((n->x * n->x)
				   +(n->y * n->y)
				   +(n->z * n->z));
	n->x /= d;
	n->y /= d;
	n->z /= d;
}

/* Update physics */
void updateParticles() {
	/* Update positions by velocity */
	for (unsigned int i = 0; i < vertices.len; ++i) {
		vertices.nodes[i]->x   += vertices.nodes[i]->v_x;
		vertices.nodes[i]->y   += vertices.nodes[i]->v_y;
		vertices.nodes[i]->z   += vertices.nodes[i]->v_z;
	}
	/* Regenerate normals */
	finish_normals();

	/* Apply gravity */
	for (unsigned int i = 0; i < vertices.len; ++i) {
		vertices.nodes[i]->v_x += 0.0;
		vertices.nodes[i]->v_y -= GRAVITY;
		vertices.nodes[i]->v_z += 0.0;
	}

	/* Run spring physics */
	for (unsigned int i = 0; i < springs.len; ++i) {
		spring_t * s = springs.nodes[i];
		/* Distance between spring ends */
		double d = distance(s->start,s->end);
		/* Force to apply */
		double f = -(s->k) * (d - s->rest_length);
		normal_t n;
		/* Direction to apply force in */
		n.x = s->start->x - s->end->x;
		n.y = s->start->y - s->end->y;
		n.z = s->start->z - s->end->z;
		normalize(&n);
		/* Update forces */
		s->start->v_x +=  n.x * f / 2.0;
		s->start->v_z +=  n.z * f / 2.0;
		s->end->v_x   += -n.x * f / 2.0;
		s->end->v_z   += -n.z * f / 2.0;
		/* We apply forces differently when we are colliding with the table */
		if (s->end->y < GROUND_LEVEL) {
			s->start->v_y +=  n.y * f * 2;
		} else if (s->start->y < GROUND_LEVEL){
			s->end->v_y   += -n.y * f * 2;
		} else {
			s->start->v_y +=  n.y * f;
			s->end->v_y   += -n.y * f;
		}
		/* Apply dampening */
		s->end->v_x *= 0.9999;
		s->end->v_y *= 0.9999;
		s->end->v_z *= 0.9999;
	}

	/* Normal forces from hitting the ground */
	for (unsigned int i = 0; i < vertices.len; ++i) {
		if (vertices.nodes[i]->y < GROUND_LEVEL) {
			if (vertices.nodes[i]->v_y < 0.0) {
				vertices.nodes[i]->v_y = -vertices.nodes[i]->v_y * 0.8;
				vertices.nodes[i]->y = GROUND_LEVEL;
			}
		}
	}

}

/* Camera path points */
float cameraPoints[][3] = {
	{3.0,1.0,0.0},
	{2.0,2.0,2.0},
	{0.0,1.0,3.0},
	{-2.0,2.0,2.0},
	{-3.0,1.0,0.0},
	{-2.0,2.0,-2.0},
	{0.0,1.0,-3.0},
	{2.0,2.0,-2.0}
};

/* Camera point count */
int frames = 8;

/* Cubic Hermite inerpolation */
double interpolate(float a, float b, float t) {
	/* Use 0 for tangents for smooth, simple animation */
	float m1 = -0.0;
	float m2 = 0.0;
	return (2 * (t * t * t) - 3 * (t * t) + 1) * a +
		   ((t * t * t) - 2 * (t * t) + t) * m1 +
		   (-2 * (t * t * t) + 3 * (t * t)) * b +
		   ((t * t * t) - (t * t)) * m2;
}

/* Update the camera position */
void moveCamera() {
	time += 0.0004;
	/* Get the previous and next positions */
	int   frame1 = ((int)time % frames);
	int   frame2 = frame1+1;
	double i;
	/* Get the midpoint between the two based on the time */
	float inter = modf(time, &i);
	if (frame1 == frames - 1) {
		frame2 = 0;
	}
	/* Update the camera coordinates */
	cam_x = interpolate(cameraPoints[frame1][0],cameraPoints[frame2][0],inter);
	cam_y = interpolate(cameraPoints[frame1][1],cameraPoints[frame2][1],inter);
	cam_z = interpolate(cameraPoints[frame1][2],cameraPoints[frame2][2],inter);
}

void display(void) {
	glLoadIdentity ();
	lights();
	/* Point the camera */
	gluLookAt(cam_x,cam_y,cam_z,
			  0.0,0.0,0.0,
			  0.0,100.0,0.0);

	if (!rotation_paused) {
		/* Update the camera */
		moveCamera();
	}

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/* Draw the teapot */
	glBegin(GL_TRIANGLES);
	for (uint32_t i = 0; i < faces.len; ++i) {
		glNormal3f(faces.nodes[i]->a->normal.x, faces.nodes[i]->a->normal.y, faces.nodes[i]->a->normal.z);
		glTexCoord2f(faces.nodes[i]->a->u,faces.nodes[i]->a->v);
		glVertex3f(faces.nodes[i]->a->x, faces.nodes[i]->a->y, faces.nodes[i]->a->z);
		glNormal3f(faces.nodes[i]->b->normal.x, faces.nodes[i]->b->normal.y, faces.nodes[i]->b->normal.z);
		glTexCoord2f(faces.nodes[i]->b->u,faces.nodes[i]->b->v);
		glVertex3f(faces.nodes[i]->b->x, faces.nodes[i]->b->y, faces.nodes[i]->b->z);
		glNormal3f(faces.nodes[i]->c->normal.x, faces.nodes[i]->c->normal.y, faces.nodes[i]->c->normal.z);
		glTexCoord2f(faces.nodes[i]->c->u,faces.nodes[i]->c->v);
		glVertex3f(faces.nodes[i]->c->x, faces.nodes[i]->c->y, faces.nodes[i]->c->z);
	}
	glEnd();

	/* Update the springs */
	updateParticles();

	glXSwapBuffers (pDisplay, glxWindow);
}

void reshape (int w, int h) {
	/* Reshape the viewport properly */
	win_width = w;
	win_height = h;
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective(90.0,(double)w / (double)h,0.0001,10.0);
	glMatrixMode (GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 'p':
			/* Pause / unpause object movement */
			rotation_paused = !rotation_paused;
			break;
		case 27:
			exit(0);
			break;
	}
}
int main(int argc, char** argv) {
	/* default values */
	char * filename = "teapot.obj";
	int c, index;
	/* Parse some command-line arguments */
	while ((c = getopt(argc, argv, "s:")) != -1) {
		switch (c) {
			case 's':
				/* Set scale */
				scale = atof(optarg);
				break;
			default:
				/* Uh, that's it for -args */
				printf("Unrecognized argument!\n");
				break;
		}
	}
	/* Get an optional filename from the last non-- parameter */
	for (index = optind; index < argc; ++index) {
		filename = argv[index];
	}
	findRGBAVisual();
	/* Initialize glut */
	glutInit(&argc, argv);
	/* Load up the file, set everything else up */
	init ();

	reshape(WIN_WIDTH,WIN_HEIGHT);

	XEvent event;
	while (1) {
		int b = XCheckWindowEvent (pDisplay,
								   window,
								   StructureNotifyMask |
								   ButtonPressMask |
								   ButtonReleaseMask |
								   KeyPressMask |
								   PointerMotionMask,
								   &event);
		if (b) {
			int i;
			switch (event.type)
			{
				/* only test for the ESC-key and exit */
				case KeyPress :
					i = XLookupKeysym(&event.xkey, 0);
					keyboard(i,0,0);
				break;
				
				case MotionNotify :
					{
						/* Update light source location */
						x_light = (event.xmotion.x - (float)(win_width / 2)) / ((float)win_height) ;
						y_light = (event.xmotion.y - (float)(win_height / 2)) / ((float)win_height);
					}
				break;

				/* our window was moved or resized */
				case ConfigureNotify :
					reshape (event.xconfigure.width, event.xconfigure.height);
				break;
			}
			
		} else {
			display();
		}
	}

	XUnmapWindow (pDisplay, window);
	XDestroyWindow (pDisplay, window);
	XCloseDisplay (pDisplay);

	return 0;
}
