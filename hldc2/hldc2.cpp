#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef WIN32
#include "freeglut/include/GL/freeglut.h"
#else
#include <GL/freeglut.h>
#endif

#define PI 3.14159265358979323846f

static char *filename;

static int renderwidth, renderheight;

// Input
typedef struct input_s
{
	int mousepos[2];
	int moused[2];
	bool lbuttondown;
	bool rbuttondown;
	bool keys[256];

} input_t;

static int mousepos[2];
static input_t input;

enum keyaction_t
{
	ka_left,
	ka_right,
	ka_up,
	ka_down,
	ka_x,
	ka_y,
	NUM_KEY_ACTIONS
};

static bool keyactions[NUM_KEY_ACTIONS];

float objx, objy;
float movex, movey;

static void DebugBreak()
{
	printf("");
}

// ==============================================
// memory allocation

void *Mem_Alloc(int numbytes)
{
	#define MEM_ALLOC_SIZE	16 * 1024 * 1024

	typedef struct memstack_s
	{
		unsigned char mem[MEM_ALLOC_SIZE];
		int allocated;

	} memstack_t;

	static memstack_t memstack;
	unsigned char *mem;
	
	if(memstack.allocated + numbytes > MEM_ALLOC_SIZE)
	{
		printf("Error: Mem: no free space available\n");
		abort();
	}

	mem = memstack.mem + memstack.allocated;
	memstack.allocated += numbytes;

	return mem;
}

// ==============================================
// errors and warnings

static void Error(const char *error, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, error);
	vsprintf(buffer, error, valist);
	va_end(valist);

	printf("Error: %s", buffer);
	exit(1);
}

static void Warning(const char *warning, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, warning);
	vsprintf(buffer, warning, valist);
	va_end(valist);

	fprintf(stdout, "Warning: %s", buffer);
}

// ==============================================
// vector utils

static void Vec2_Copy(float a[2], float b[2])
{
	a[0] = b[0];
	a[1] = b[1];
}

static void Vec2_Normalize(float *v)
{
	float len, invlen;

	len = sqrtf((v[0] * v[0]) + (v[1] * v[1]));
	invlen = 1.0f / len;

	v[0] *= invlen;
	v[1] *= invlen;
}

// return the circular distance
static float Vec2_Length(float v[2])
{
	return sqrtf((v[0] * v[0]) + (v[1] * v[1]));
}

static float Vec2_Dot(float a[2], float b[2])
{
	return (a[0] * b[0]) + (a[1] * b[1]);
}

static void Plane2d(float abc[3], float a[2], float b[2])
{
	float x0, y0, x1, y1, x, y, l, nx, ny, d;

	x0 = a[0], y0 = a[1];
	x1 = b[0], y1 = b[1];

	x = x1 - x0;
	y = y1 - y0;
	l = sqrt((x * x) + (y * y));

	nx =  y / l;
	ny = -x / l;
	d = -x0 * nx - y0 * ny;

	abc[0] = nx, abc[1] = ny, abc[2] = d;
}

static float PlaneDistance(float abc[3], float xy[2])
{
	float a, b, c, x, y;

	a = abc[0], b = abc[1], c = abc[2];
	x = xy[0], y = xy[1];
	return (a * x) + (b * y) + c;
}

// Called every frame to process the current mouse input state
// We only get updates when the mouse moves so the current mouse
// position is stored and may be used for mulitple frames
static void ProcessInput()
{
	// mousepos has current "frame" mouse pos
	input.moused[0] = mousepos[0] - input.mousepos[0];
	input.moused[1] = mousepos[1] - input.mousepos[1];
	input.mousepos[0] = mousepos[0];
	input.mousepos[1] = mousepos[1];
}

static float CircleDistance(float p[2], float r)
{
	return Vec2_Length(p) - r;
}

#undef min
#define min(a, b) (a < b ? a : b)

#undef max
#define max(a, b) (a > b ? a : b)


#if 0
static float BoxDistance(float halfsize[2], float p[2])
{
	float d[2];

	d[0] = fabs(p[0]) - halfsize[0];
	d[1] = fabs(p[1]) - halfsize[1];

	float d2[2] = { max(d[0], 0.0f), max(d[1], 0.0f) };
	return min(max(d[0], d[1]), 0.0f) + Vec2_Length(d2);
}

static float RoundedBoxDistance(float halfsize[2], float r, float p[2])
{
	float d[2];

	d[0] = fabs(p[0]) - halfsize[0];
	d[1] = fabs(p[1]) - halfsize[1];

	if (d[0] >= 0.0f && d[1] >= 0.0f)
	{
		float d2[2] = { d[0], d[1] };
		return Vec2_Length(d2) - r;
	}
	else
	{
		float d2[2] = { max(d[0] - r, 0.0f), max(d[1] - r, 0.0f) };
		return min(max(d[0] - r, d[1] - r), 0.0f) + Vec2_Length(d2);
	}
}
#endif

typedef struct trace_s
{
	float d;
	float n[2];
} trace_t;

static void BoxDistance(trace_t *tr, float halfsize[2], float p[2])
{
	float d[2];

	d[0] = fabs(p[0]) - halfsize[0];
	d[1] = fabs(p[1]) - halfsize[1];

	float d2[2] = { max(d[0], 0.0f), max(d[1], 0.0f) };
	tr->d    = min(max(d[0], d[1]), 0.0f) + Vec2_Length(d2);
	tr->n[0] = (d[0] > d[1] ? 1.0f : 0.0f);
	tr->n[1] = (d[1] > d[0] ? 1.0f : 0.0f);  

	// adjust the normal depending on the quadrant
	bool flipx = p[0] < 0.0f;
	bool flipy = p[1] < 0.0f; 
	if (flipx)
		tr->n[0] = -tr->n[0];
	if (flipy)
		tr->n[1] = -tr->n[1];

	if (tr->d == 0.0)
		DebugBreak();
}

static void RoundedBoxDistance(trace_t *tr, float halfsize[2], float r, float p[2])
{
	float d[2];

	d[0] = fabs(p[0]) - halfsize[0];
	d[1] = fabs(p[1]) - halfsize[1];

	if (d[0] >= 0.0f && d[1] >= 0.0f)
	{
		float d2[2] = { d[0], d[1] };
		tr->d    = Vec2_Length(d2) - r;
		tr->n[0] = d[0];
		tr->n[1] = d[1];
		Vec2_Normalize(tr->n);
	}
	else
	{
		float d2[2] = { max(d[0] - r, 0.0f), max(d[1] - r, 0.0f) };
		tr->d    =  min(max(d[0] - r, d[1] - r), 0.0f) + Vec2_Length(d2);
		tr->n[0] = (d[0] > d[1] ? 1.0f : 0.0f);
		tr->n[1] = (d[1] > d[0] ? 1.0f : 0.0f);  
	}

	// adjust the normal depending on the quadrant
	bool flipx = p[0] < 0.0f;
	bool flipy = p[1] < 0.0f; 
	if (flipx)
		tr->n[0] = -tr->n[0];
	if (flipy)
		tr->n[1] = -tr->n[1];
}

static char data [] =
{
	"11111111"
	"10000001"
	"10011001"
	"10011001"
	"10001001"
	"10000001"
	"10000001"
	"11111111"
};

static char GetCell(int x, int y)
{
	return data[y * 8 + x];
}

#if 0
static float Distance(float p[2])
{
	float d;
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			char c = GetCell(x, y);
	
			if (c != '1')
				continue;

			float center[2] = { x + 0.5f, y + 0.5f };
			float half[2] = { 0.5f, 0.5f };

			// convert p to the local box coodinate system
			float pp[2] = { p[0] - center[0], p[1] - center[1] };
		
			// expand the bounds by the player
			//half[0] += 0.1f;
			//half[1] += 0.1f;

			//float q = BoxDistance(half, pp);
			float q = RoundedBoxDistance(half, 0.3f, pp);

			if (x == 0 && y == 0)
				d = q;
			else
				d = min(d, q);
		}
	}

	return d;
}

static void Gradient(float grad[2], float p[2])
{
	float h = 0.01f;
	float dx, dy;

	float p0[2] = { p[0] - h, p[1] };
	float p1[2] = { p[0] + h, p[1] };
	dx = (Distance(p1) - Distance(p0)) / (2 * h);

	float p2[2] = { p[0], p[1] - h };
	float p3[2] = { p[0], p[1] + h };
	dy = (Distance(p3) - Distance(p2)) / (2 * h);

	grad[0] = dx;
	grad[1] = dy;
}

static void DrawCursor()
{
	float xy[2], d, grad[2];

	// convert mouse position from screen to identity
	xy[0] = (float)mousepos[0] / (float)renderwidth;
	xy[1] = 1.0f - ((float)mousepos[1] / (float)renderheight);

	// convert from identity to model pos
	xy[0] = xy[0] * 8;
	xy[1] = xy[1] * 8;

	fprintf(stdout, "x, y: %2.2f, %2.2f\n", xy[0], xy[1]);

	d = Distance(xy);
	fprintf(stdout, "distance %f\n", d);

	Gradient(grad, xy);
	Vec2_Normalize(grad);
	fprintf(stdout, "gradient %f, %f\n", grad[0], grad[1]);

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex2f(xy[0], xy[1]);
	glVertex2f(xy[0] + (d * -grad[0]), xy[1] + (d * -grad[1]));
	glEnd();
}

static unsigned char *BuildTextureData(int texw, int texh)
{
	unsigned char *data = (unsigned char*)malloc(texw * texh * 4);
	for (int y = 0; y < texh; y++)
	{
		for (int x = 0; x < texw; x++)
		{
			float xy[2];

			// convert mouse position from screen to identity
			xy[0] = (float)x / (float)texw;
			//xy[1] = 1.0f - ((float)y / (float)renderheight);
			xy[1] = (float)y / (float)texh;

			// convert from identity to model pos
			xy[0] = xy[0] * 8.0f;
			xy[1] = xy[1] * 8.0f;

			float d = Distance(xy);
			d = max(-1.0f, min(d, 1.0f));
			d *= 32;
			
			unsigned char *t = data + (y * texw * 4) + (x * 4);
			*t++ = max(0, d) + 50;
			*t++ = 0;
			*t++ = max(0, -d) + 50;
			*t++ = 255;
		}
	}

	return data;
}

static void DrawField()
{
	static int texw, texh;
	static GLuint texture;

	if (texw != renderwidth || texh != renderheight)
	{
		texw = renderwidth;
		texh = renderheight;

		if (texture)
		{
			glDeleteTextures(1, &texture);
			texture = 0;
		}
		if (!texture)
			glGenTextures(1, &texture);

		printf("rebuilding texture data %i, %i\n", texw, texh);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texw, texh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(GL_TEXTURE_2D, texture);
		unsigned char *data = BuildTextureData(texw, texh);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texw, texh, GL_RGBA, GL_UNSIGNED_BYTE, data);
		free(data);
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glMatrixMode(GL_PROJECTION);
	//glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	{

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 0.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, 0.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(0.0f, 1.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, 1.0f);
		glEnd();
	}

	//glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

#endif

static void DrawGrid()
{
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			char c = GetCell(x, y);
			if (c != '1')
				continue;

			float s = 1;

			glColor3f(1, 1, 1);
			glBegin(GL_LINE_LOOP);
			glVertex2f(x    , y    );
			glVertex2f(x + s, y    );
			glVertex2f(x + s, y + s);
			glVertex2f(x    , y + s);
			glEnd();
		}
	}
}

static void DrawObject(float x, float y)
{
	float s = 0.4f;

	glColor3f(1, 0, 1);

	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(x - s, y - s);
	glVertex2f(x + s, y - s);
	glVertex2f(x - s, y + s);
	glVertex2f(x + s, y + s);
	glEnd();
}

static void DrawPlayer()
{
	DrawObject(objx, objy);
}

static void Draw()
{
	//DrawField();

	// draw field will nuke the projection matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 8, 0, 8, -1, 1);

	DrawGrid();

	//DrawCursor();
}

static void TryMove()
{
	float nextx, nexty;

	// get the target location
	nextx = objx + movex;
	nexty = objy + movey;

#if 1
	// iterate through every object and position correct it
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			char c = GetCell(x, y);
	
			if (c != '1')
				continue;

			float center[2] = { x + 0.5f, y + 0.5f };
			float half[2] = { 0.5f, 0.5f };
			
			// convert p to the local box coodinate system
			float pp[2] = { nextx - center[0], nexty - center[1] };
		
			// expand the bounds by the player
			//half[0] += 0.1f;
			//half[1] += 0.1f;

			trace_t tr;
			//BoxDistance(&tr, half, pp);
			RoundedBoxDistance(&tr, half, 0.4f, pp);


			// allow slop on the intersection
			tr.d += 0.005f;

			// don't need to do anything if no collision
			if (tr.d > 0.0f)
				continue;

			//DebugBreak();
			//printf("d=%f, n=%f, %f\n", tr.d, tr.n[0], tr.n[1]);

			// otherwise position correct
			nextx += (-tr.d * tr.n[0]);
			nexty += (-tr.d * tr.n[1]);
		}
	}
#endif

	// commit the position changes
	//printf("cur=%f, %f next=%f, %f\n", objx, objy, nextx, nexty);
	objx = nextx;
	objy = nexty;
}

static void Player_Frame()
{
	float s = 0.05f;

	movex = 0;
	if (keyactions[ka_left])
		movex -= s;
	if (keyactions[ka_right])
		movex += s;

	movey = 0;
	if (keyactions[ka_down])
		movey -= s;
	if (keyactions[ka_up])
		movey += s;

	TryMove();
}

// glut functions
static void DisplayFunc()
{
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glEnable(GL_DEPTH_TEST);

	Draw();

	DrawPlayer();

	glutSwapBuffers();
}

static void ReshapeFunc(int w, int h)
{
	renderwidth = w;
	renderheight = h;

	glViewport(0, 0, renderwidth, renderheight);
}

static void MouseFunc(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON)
		input.lbuttondown = (state == GLUT_DOWN);
	if(button == GLUT_RIGHT_BUTTON)
		input.rbuttondown = (state == GLUT_DOWN);
}

static void MouseMoveFunc(int x, int y)
{
	mousepos[0] = x;
	mousepos[1] = y;
}

static void KeyDownFunc(unsigned char key, int x, int y)
{
	if (key == 'a')
		keyactions[ka_left] = true;
	if (key == 'd')
		keyactions[ka_right] = true;
	if (key == 'w')
		keyactions[ka_up] = true;
	if (key == 's')
		keyactions[ka_down] = true;
	if (key == 'x')
		keyactions[ka_x] = true;
	if (key == 'z')
		keyactions[ka_y] = true;
}
static void KeyUpFunc(unsigned char key, int x, int y)
{
	if (key == 'a')
		keyactions[ka_left] = false;
	if (key == 'd')
		keyactions[ka_right] = false;
	if (key == 'w')
		keyactions[ka_up] = false;
	if (key == 's')
		keyactions[ka_down] = false;
	if (key == 'x')
		keyactions[ka_x] = false;
	if (key == 'z')
		keyactions[ka_y] = false;
}
static void SpecialDownFunc(int key, int x, int y)
{
	if (key == GLUT_KEY_LEFT)
		keyactions[ka_left] = true;
	if (key == GLUT_KEY_RIGHT)
		keyactions[ka_right] = true;
	if (key == GLUT_KEY_UP)
		keyactions[ka_up] = true;
	if (key == GLUT_KEY_DOWN)
		keyactions[ka_down] = true;
}
static void SpecialUpFunc(int key, int x, int y)
{
	if (key == GLUT_KEY_LEFT)
		keyactions[ka_left] = false;
	if (key == GLUT_KEY_RIGHT)
		keyactions[ka_right] = false;
	if (key == GLUT_KEY_UP)
		keyactions[ka_up] = false;
	if (key == GLUT_KEY_DOWN)
		keyactions[ka_down] = false;
}

// ticked at 60hz
// split into Frame()
static void TimerFunc(int value)
{
	// standard mouse input
	ProcessInput();

	Player_Frame();

	// kick a display refresh
	glutPostRedisplay();
	glutTimerFunc(16, TimerFunc, 0);
}

static void PrintUsage()
{}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(400, 400);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("test");

	glutReshapeFunc(ReshapeFunc);
	glutDisplayFunc(DisplayFunc);
	glutKeyboardFunc(KeyDownFunc);
	glutKeyboardUpFunc(KeyUpFunc);
	glutSpecialFunc(SpecialDownFunc);
	glutSpecialUpFunc(SpecialUpFunc);
	glutMouseFunc(MouseFunc);
	glutMotionFunc(MouseMoveFunc);
	glutPassiveMotionFunc(MouseMoveFunc);
	glutTimerFunc(16, TimerFunc, 0);


	objx = 2.0f;
	objy = 2.0f;

	glutMainLoop();

	return 0;
}

