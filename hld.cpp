#include <stdio.h>
#include <GL/freeglut.h>

static int screenw = 768;
static int screenh = 480;
static int renderw = screenw / 4;
static int renderh = screenh / 4;
static int framenum = 0;

enum { BUILTIN_CHECK, BUILTIN_HLINE, BUILTIN_VLINE, SPR0, NUM_SPR };

static unsigned char *sprdata;
static int framestride = 9 * 9 * 4;
static GLuint sprtexture;

int ReadFile(const char* filename, void **data)
{
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		printf("Failed to open file \"%s\"\n", filename);

	int curpos = ftell(fp)
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, curpos, SEEK_SET);

	*data = malloc(size);
	fread(*data, size, 1, fp);
	fclose(fp);

	return size;
}

static void InitTexture()
{
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

	// spr0
	{
		glBindTexture(GL_TEXTURE_2D, SPR0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}
}

static void LoadData()
{
	ReadFile("diamond", (void**)&sprdata);

	InitTexture();
}

static void UploadTexture()
{
	unsigned char *pixels = sprdata + ((framenum % 42) * framestride);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 9, 9, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, SPR0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 9, 9, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static void DrawAlphaLayer()
{
	glColorMask(0, 0, 0, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, BUILTIN_VLINE);
	glColor3f(1, 1, 1);

	glBegin(GL_TRIANGLE_STRIP);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);

	glTexCoord2f(4.5f, 0.0f);
	glVertex2f(9.0, 0.0f);

	glTexCoord2f(0.0f, 4.5f);
	glVertex2f(0.0f, 9.0f);

	glTexCoord2f(4.5f, 4.5f);
	glVertex2f(9.0f, 9.0f);

	glEnd();

	glDisable(GL_BLEND);
	glColorMask(1, 1, 1, 1);
}


// 9 comes from the sprite width
// scaling will affect this
static int posx = renderw - 5 - 2;
static int posy = renderh - 5 - 2;
static int sizex = 9;
static int sizey = 9;
static int centerx = 4;
static int centery = 4;

static void DrawSpr()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO);
	//if ((framenum & 0x3) == (rand() %4))
	//	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, SPR0);
	glColor3f(1, 1, 1);

	glBegin(GL_TRIANGLE_STRIP);

	int basex = posx - centerx;
	int basey = posy - centery;

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(basex + 0.0f, basey + 0.0f);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(basex + sizex, basey + 0.0f);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(basex + 0.0f, basey + sizey);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(basex + sizex, basey + sizey);

	glEnd();

	glDisable(GL_BLEND);
}

static void Draw()
{
	UploadTexture();

	//DrawAlphaLayer();

	DrawSpr();
}

static void ReshapeFunc(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(-1, 1, -1, 1, -1, 1);

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

