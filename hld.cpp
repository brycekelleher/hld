#include <stdio.h>
#include <GL/freeglut.h>

// attributes for sprite rendering:
// color
// transparency
// flip horizontal
// flip vertical
// mask effects, SOLID, VLINE, HLINE, DIAG
// frame, layer
// pass in a token to identify which sprite

static int screenw = 768;
static int screenh = 480;
static int renderw = screenw / 2;
static int renderh = screenh / 2;
static int framenum = 0;

// texture identifiers
enum
{
	BUILTIN_SOLID,
	BUILTIN_HLINE,
	BUILTIN_VLINE,
	BUILTIN_DLINE,
	BUILTIN_CHECK,
	BUILTIN_POINT,
	SPR0,
	NUM_SPR
};

// sprite data
static int sizex = 40;
static int sizey = 55;
static int centerx = 0;
static int centery = 0;
static int numframes = 1;
static int framestride = sizex * sizey * 4;
static unsigned char *sprdata;

int ReadFile(const char* filename, void **data)
{
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		printf("Failed to open file \"%s\"\n", filename);

	int curpos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, curpos, SEEK_SET);

	*data = malloc(size);
	fread(*data, size, 1, fp);
	fclose(fp);

	return size;
}

// fixme:
// modern opengl says that internal format and external format must match
// also, use a sized internal format
// this needs to happen anyway to use a luminance mask
static void InitTexture()
{
	// allow small rows on a byte alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// solid
	{
		unsigned char data[2] = { 0xff };
		glBindTexture(GL_TEXTURE_2D, BUILTIN_SOLID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// alpha hline
	{
		unsigned char data[2] = { 0xff, 0x0 };
		glBindTexture(GL_TEXTURE_2D, BUILTIN_HLINE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	
	// alpha vline
	{
		unsigned char data[2] = { 0xff, 0x0 };
		glBindTexture(GL_TEXTURE_2D, BUILTIN_VLINE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// alpha checkerboard
	{
		unsigned char data[4] = { 0xff, 0x0, 0x0, 0xff };
		glBindTexture(GL_TEXTURE_2D, BUILTIN_CHECK);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// alpha point
	{
		unsigned char data[4] = { 0xff, 0x0, 0x0, 0x0 };
		glBindTexture(GL_TEXTURE_2D, BUILTIN_POINT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// alpha diagonal lines
	{
		unsigned char data[16] =
		{
			 0x0,  0x0, 0xff,  0x0,
			 0x0,  0x0,  0x0, 0xff,
			0xff,  0x0,  0x0,  0x0,
			 0x0, 0xff,  0x0,  0x0
		};


		glBindTexture(GL_TEXTURE_2D, BUILTIN_DLINE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// spr0
	{
		glBindTexture(GL_TEXTURE_2D, SPR0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizex, sizey, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}

static void LoadData()
{
	ReadFile("troo", (void**)&sprdata);

	InitTexture();
}

static void UploadTexture()
{
	unsigned char *pixels = sprdata + ((framenum % numframes) * framestride);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizex, sizey, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, SPR0);
}

//
// interface to drawsprite
//
static int posx = 0;
static int posy = 0;
static GLuint alphatex = BUILTIN_SOLID;
static bool flipx = false;
static bool flipy = false;
static float rgba[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
#if 0
static int posx = renderw - sizex - 2;
static int posy = renderh - sizey - 2;
#endif

// internal draw sprite data
static float vertices[4][10];

static float AlphaScaleFactor()
{
	if (alphatex == BUILTIN_SOLID)
		return 1.0f;
	else if (alphatex == BUILTIN_DLINE)
		return 0.25f;
	else
		return 0.5f;
}

static void SwapFloat(float *x, float *y)
{
	float temp = *x;
	*x = *y;
	*y = temp;
}

static void AssembleVertexData()
{
	int basex = posx - centerx;
	int basey = posy - centery;

	float alphascale = AlphaScaleFactor();

	// assemble the vertex data xy, uv
	vertices[0][0] = basex;
	vertices[0][1] = basey;
	vertices[0][2] = 0.0f;
	vertices[0][3] = 0.0f;
	vertices[0][4] = 0.0f;
	vertices[0][5] = 0.0f;
	vertices[0][6] = rgba[0];
	vertices[0][7] = rgba[1];
	vertices[0][8] = rgba[2];
	vertices[0][9] = rgba[3];

	vertices[1][0] = basex + sizex;
	vertices[1][1] = basey;
	vertices[1][2] = 1.0f;
	vertices[1][3] = 0.0f;
	vertices[1][4] = sizex * alphascale;
	vertices[1][5] = 0.0f;
	vertices[1][6] = rgba[0];
	vertices[1][7] = rgba[1];
	vertices[1][8] = rgba[2];
	vertices[1][9] = rgba[3];

	vertices[2][0] = basex;
	vertices[2][1] = basey + sizey;
	vertices[2][2] = 0.0f;
	vertices[2][3] = 1.0f;
	vertices[2][4] = 0.0f;
	vertices[2][5] = sizey * alphascale;
	vertices[2][6] = rgba[0];
	vertices[2][7] = rgba[1];
	vertices[2][8] = rgba[2];
	vertices[2][9] = rgba[3];

	vertices[3][0] = basex + sizex;
	vertices[3][1] = basey + sizey;
	vertices[3][2] = 1.0f;
	vertices[3][3] = 1.0f;
	vertices[3][4] = sizex * alphascale;
	vertices[3][5] = sizey * alphascale;
	vertices[3][6] = rgba[0];
	vertices[3][7] = rgba[1];
	vertices[3][8] = rgba[2];
	vertices[3][9] = rgba[3];

	if (flipx)
	{
		SwapFloat(&vertices[0][2], &vertices[1][2]);
		SwapFloat(&vertices[2][2], &vertices[3][2]);
	}

	if (flipy)
	{
		SwapFloat(&vertices[0][3], &vertices[2][3]);
		SwapFloat(&vertices[1][3], &vertices[3][3]);
	}
}


static void DrawAlphaLayer()
{
	glColorMask(0, 0, 0, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, alphatex);
	glColor3f(1, 1, 1);

	glBegin(GL_TRIANGLE_STRIP);

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < 4; i++)
	{
		glTexCoord2f(vertices[i][4], vertices[i][5]);
		glColor4f(1, 1, 1, 1);
		glVertex2f(vertices[i][0], vertices[i][1]);
	}

	glEnd();

	glDisable(GL_BLEND);
	glColorMask(1, 1, 1, 1);
}


static void DrawColorLayer()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, SPR0);
	glColor3f(1, 1, 1);


	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < 4; i++)
	{
		glTexCoord2f(vertices[i][2], vertices[i][3]);
		glColor4f(vertices[i][6], vertices[i][7], vertices[i][8], vertices[i][9]);
		glVertex2f(vertices[i][0], vertices[i][1]);
	}

	glEnd();

	glDisable(GL_BLEND);
}

static void DrawSpr()
{

	// setup the state
	{
		alphatex = BUILTIN_SOLID;
		flipx = 1;
		flipy = 1;

		// this would be done by the client
		//if ((framenum & 0x3) == (rand() %4))
		//	alphatex = ((rand() % 2) ? BUILTIN_VLINE : BUILTIN_POINT);
	}

	AssembleVertexData();

	DrawAlphaLayer();

	DrawColorLayer();
}

static void Draw()
{
	UploadTexture();

	DrawSpr();
}

static void ReshapeFunc(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, screenw, screenh);
}

static void DisplayFunc()
{
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, renderw, 0, renderh, -1, 1);

	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	Draw();

	glutSwapBuffers();
}

static void TimerFunc(int value)
{
	framenum += 1;
	glutPostRedisplay();
	glutTimerFunc(200, TimerFunc, 0);
}

static void InitGlut(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH);
	glutCreateWindow("test window");
	glutReshapeWindow(screenw, screenh);

	glutDisplayFunc(DisplayFunc);
	glutReshapeFunc(ReshapeFunc);

	glutTimerFunc(33, TimerFunc, 0);

	// need to explictly request an alpha plane or it isn't created
	int alphabits;
	glGetIntegerv(GL_ALPHA_BITS, &alphabits);
	printf("alphabits = %i\n", alphabits);
}

int main(int argc, char *argv[])
{
	InitGlut(argc, argv);

	LoadData();

	glutMainLoop();

	return 0;
}

