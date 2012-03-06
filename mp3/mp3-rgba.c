/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * CS 418 MP3 - Kevin Lange <lange7@illinois.edu>
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

/* this here is from the hints nvidia gives in their driver docs for utilizing
** the new AddARGBGLXVisuals option for xorg.conf */
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

Bool wait_for_notify (Display* pDisplay,
					  XEvent* pEvent,
					  XPointer arg)
{
	return (pEvent->type == MapNotify) && (pEvent->xmap.window == (Window) arg);
}

Display* pDisplay = NULL;
Window window;
GLXWindow glxWindow;

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

		if (pPictFormat->direct.alphaMask > 0)
		{
			fprintf (stderr, "Strike, found a GLX visual with alpha-support!\n");
			pVisInfo = glXGetVisualFromFBConfig (pDisplay, pFBConfigs[i]);
			renderFBConfig = pFBConfigs[i];
			bKeepGoing = True;
			break;
		} else {
			fprintf(stderr, "Nope: %d\n", pPictFormat->direct.alphaMask);
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



GLuint texture_a; /* Diffuse texture */
GLuint texture_b; /* Environment spheremap */

/* Scene scale */
float scale = 1.0;
/* Object rotation */
float rot = 0.0;
/* Camera height */
float height = 1.0;
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

/* Model vertices */
vertices_t vertices = {.len = 0, .capacity = 0};
/* Model triangles */
faces_t    faces    = {.len = 0, .capacity = 0};

/* Initialize the model objects */
void init_model() {
	/* We give an initial capacity of 16 for each */
	vertices.capacity = 16;
	vertices.nodes    = (vertex_t **)malloc(sizeof(vertex_t *) * vertices.capacity);
	faces.capacity    = 16;
	faces.nodes       = (face_t **)malloc(sizeof(face_t *) * faces.capacity);
}

/* Add the given coordinates to the vertex list */
void add_vertex(float x, float y, float z) {
	if (vertices.len == vertices.capacity) {
		/* When we run out of space, increase by two */
		vertices.capacity *= 2;
		vertices.nodes = (vertex_t **)realloc(vertices.nodes, sizeof(vertex_t *) * vertices.capacity);
	}
	/* Create a new vertex in the list */
	vertices.nodes[vertices.len] = malloc(sizeof(vertex_t));
	vertices.nodes[vertices.len]->x = x * scale;
	vertices.nodes[vertices.len]->y = y * scale;
	vertices.nodes[vertices.len]->z = z * scale;
	/* Set texture coordinates by cylindrical mapping */
	float theta = atan2(z,x);
	vertices.nodes[vertices.len]->u = (theta + PI) / (TAO);
	vertices.nodes[vertices.len]->v = (y / 2.0);
	/* Initialize normals to 0,0,0 */
	vertices.nodes[vertices.len]->normal.x = 0.0f;
	vertices.nodes[vertices.len]->normal.y = 0.0f;
	vertices.nodes[vertices.len]->normal.z = 0.0f;
	vertices.len++;
}

/* Add a face with the given vertices */
void add_face(int a, int b, int c) {
	if (faces.len == faces.capacity) {
		/* Double size when we run out... */
		faces.capacity *= 2;
		faces.nodes = (face_t **)realloc(faces.nodes, sizeof(face_t *) * faces.capacity);
	}
	if (vertices.len < a || vertices.len < b || vertices.len < c) {
		/* Frick... */
		fprintf(stderr, "ERROR: Haven't yet collected enough vertices for the face %d %d %d (have %d)!\n", a, b, c, vertices.len);
		exit(1);
	}
	/* Create a new triangle */
	faces.nodes[faces.len] = malloc(sizeof(face_t));
	faces.nodes[faces.len]->a = vertices.nodes[a-1];
	faces.nodes[faces.len]->b = vertices.nodes[b-1];
	faces.nodes[faces.len]->c = vertices.nodes[c-1];
	/* Calculate some normals */
	vertex_t u = {.x = faces.nodes[faces.len]->b->x - faces.nodes[faces.len]->a->x,
				  .y = faces.nodes[faces.len]->b->y - faces.nodes[faces.len]->a->y,
				  .z = faces.nodes[faces.len]->b->z - faces.nodes[faces.len]->a->z};
	vertex_t v = {.x = faces.nodes[faces.len]->c->x - faces.nodes[faces.len]->a->x,
				  .y = faces.nodes[faces.len]->c->y - faces.nodes[faces.len]->a->y,
				  .z = faces.nodes[faces.len]->c->z - faces.nodes[faces.len]->a->z};
	/* Set the face normals */
	faces.nodes[faces.len]->normal.x = ((u.y * v.z) - (u.z * v.y));
	faces.nodes[faces.len]->normal.y = -((u.z * v.x) - (u.x * v.z));
	faces.nodes[faces.len]->normal.z = ((u.x * v.y) - (u.y * v.x));
	faces.len++;
}

void finish_normals() {
	/* Loop through vertices and accumulate normals for them */
	for (uint32_t i = 0; i < faces.len; ++i) {
		/* Vertex a */
		faces.nodes[i]->a->normal.x += faces.nodes[i]->normal.x;
		faces.nodes[i]->a->normal.y += faces.nodes[i]->normal.y;
		faces.nodes[i]->a->normal.z += faces.nodes[i]->normal.z;
		/* Vertex b */
		faces.nodes[i]->b->normal.x += faces.nodes[i]->normal.x;
		faces.nodes[i]->b->normal.y += faces.nodes[i]->normal.y;
		faces.nodes[i]->b->normal.z += faces.nodes[i]->normal.z;
		/* Vertex c */
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
	/* Initialize the lists */
	init_model();
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
					add_vertex(x, y, z);
				}
				break;
			case 'f':
				{
					/* Face */
					int a, b, c;
					collected = fscanf(obj, "%d %d %d\n", &a, &b, &c);
					if (collected < 3) fprintf(stderr, "ERROR: Only collected %d vertices!\n", collected);
					add_face(a,b,c);
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

/* Initialize the scene */
void init(char * object, char * diffuse, char * sphere) {
	load_wavefront(object);
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
//	glClearColor (0.5, 0.5, 1.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	/* Initialize the two textures */
	char * texture;
	int32_t size;
	int dif_size, env_size;
	glGenTextures(1,&texture_a);
	glGenTextures(1,&texture_b);
	/* Diffuse texture { */
	/* The diffuse texture is a wood texture */
	texture = readFile(diffuse, &size); /* We have stored are textures as raw RGBA */
	dif_size = (int)sqrt(size / 4);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_a);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, dif_size, dif_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
	free(texture);
	/* } */
	/* Sphere map texture { */
	texture = readFile(sphere, &size);
	env_size = (int)sqrt(size / 4);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_b);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, env_size, env_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
	free(texture);
	/* } */

	/* Load in the shader programs */
	char *vs = NULL,
		 *fs = NULL;
	int32_t v_size, f_size;
	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);
	vs = readFile("teapot.vert", &v_size); /* Vertex shader */
	fs = readFile("teapot.frag", &f_size); /* Fragment shader */
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

	/* Set the texture sources */
	GLint tex0 = glGetUniformLocation(p, "texture");
	GLint tex1 = glGetUniformLocation(p, "spheremap");
	glUniform1i(tex0, 0);
	glUniform1i(tex1, 1);

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
	float l_scale = 30.0;
	GLfloat lpos[] = {l_scale * x_light, l_scale * y_light, 3.0};

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
}

void display(void) {
	glLoadIdentity ();
	lights();
	/* Point the camera */
	gluLookAt(4.0 * sin(rot),height,-4.0 * cos(rot),
			  0.0,cam_offset,0.0,
			  0.0,100.0,0.0);

	if (!rotation_paused) {
		rot += 0.002;
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

	//glutSwapBuffers();
	glXSwapBuffers (pDisplay, glxWindow);
	//glFlush ();

	//glutPostRedisplay();
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
		case 'w':
			/* Raise camera */
			height += 0.07;
			break;
		case 's':
			/* Lower camera */
			height -= 0.07;
			break;
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
	char * diffuse  = "wood.rgba";
	char * sphere   = "nvidia.rgba";
	int c, index;
	/* Parse some command-line arguments */
	while ((c = getopt(argc, argv, "d:e:h:s:")) != -1) {
		switch (c) {
			case 'd':
				diffuse = optarg;
				break;
			case 'e':
				sphere = optarg;
				break;
			case 's':
				/* Set scale */
				scale = atof(optarg);
				break;
			case 'h':
				cam_offset = atof(optarg);
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
	init (filename, diffuse, sphere);

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
