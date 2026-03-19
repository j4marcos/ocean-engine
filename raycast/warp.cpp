
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

struct Vec2 {
	float x;
	float y;
};

static int gWindowWidth = 1000;
static int gWindowHeight = 700;

static float gWarpStrength = 0.22f;
static float gWarpRadius = 0.28f;
static Vec2 gWarpCenter = {0.25f, 0.0f};

static Vec2 gRayStart = {-0.95f, -0.52f};
static Vec2 gRayDir = {1.0f, 0.16f};

static float vecLength(const Vec2 v) {
	return std::sqrt(v.x * v.x + v.y * v.y);
}

static Vec2 vecAdd(const Vec2 a, const Vec2 b) {
	return {a.x + b.x, a.y + b.y};
}

static Vec2 vecSub(const Vec2 a, const Vec2 b) {
	return {a.x - b.x, a.y - b.y};
}

static Vec2 vecScale(const Vec2 v, const float s) {
	return {v.x * s, v.y * s};
}

static Vec2 vecNormalize(const Vec2 v) {
	const float len = vecLength(v);
	if (len <= 1e-6f) {
		return {0.0f, 0.0f};
	}
	return {v.x / len, v.y / len};
}

static Vec2 warpField(const Vec2 p) {
	const Vec2 toCenter = vecSub(gWarpCenter, p);
	const float distance = vecLength(toCenter);
	const float softened = distance + 0.02f;
	const float influence = std::exp(-(distance * distance) / (gWarpRadius * gWarpRadius));
	const float magnitude = (gWarpStrength * influence) / (softened * softened);
	return vecScale(vecNormalize(toCenter), magnitude);
}

static void drawWarpFieldOutline() {
	glColor3f(0.05f, 0.55f, 0.90f);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 96; ++i) {
		const float t = 2.0f * 3.14159265359f * static_cast<float>(i) / 96.0f;
		const float x = gWarpCenter.x + std::cos(t) * gWarpRadius;
		const float y = gWarpCenter.y + std::sin(t) * gWarpRadius;
		glVertex2f(x, y);
	}
	glEnd();
	glLineWidth(1.0f);
}

static void drawWarpCore() {
	glColor3f(0.18f, 0.86f, 1.0f);
	glPointSize(8.0f);
	glBegin(GL_POINTS);
	glVertex2f(gWarpCenter.x, gWarpCenter.y);
	glEnd();
	glPointSize(1.0f);
}

static void drawRayPath() {
	Vec2 position = gRayStart;
	Vec2 direction = vecNormalize(gRayDir);

	const float stepLength = 0.006f;
	const int maxSteps = 1800;

	glColor3f(1.0f, 0.96f, 0.36f);
	glLineWidth(2.8f);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < maxSteps; ++i) {
		glVertex2f(position.x, position.y);

		const Vec2 accel = warpField(position);
		direction = vecNormalize(vecAdd(direction, vecScale(accel, stepLength)));
		position = vecAdd(position, vecScale(direction, stepLength));

		if (position.x > 1.10f || position.x < -1.10f || position.y > 1.10f || position.y < -1.10f) {
			break;
		}
	}
	glEnd();
	glLineWidth(1.0f);
}

static void drawInfoBar() {
	glColor3f(0.22f, 0.22f, 0.24f);
	glBegin(GL_QUADS);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f(1.0f, -1.0f);
	glVertex2f(1.0f, -0.86f);
	glVertex2f(-1.0f, -0.86f);
	glEnd();

	glColor3f(0.7f, 0.7f, 0.7f);
	glBegin(GL_LINES);
	glVertex2f(-1.0f, -0.86f);
	glVertex2f(1.0f, -0.86f);
	glEnd();
}

static void display() {
	glClear(GL_COLOR_BUFFER_BIT);

	drawInfoBar();
	drawWarpFieldOutline();
	drawWarpCore();
	drawRayPath();

	glutSwapBuffers();
}

static void reshape(const int w, const int h) {
	gWindowWidth = w;
	gWindowHeight = h;

	glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	const float aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;
	if (aspect >= 1.0f) {
		glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
	} else {
		glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, -1.0, 1.0);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void keyboard(const unsigned char key, const int x, const int y) {
	(void)x;
	(void)y;

	switch (key) {
		case 27:
		case 'q':
		case 'Q':
			std::exit(0);
			break;
		case '+':
		case '=':
			gWarpStrength += 0.02f;
			glutPostRedisplay();
			break;
		case '-':
		case '_':
			gWarpStrength -= 0.02f;
			if (gWarpStrength < 0.01f) {
				gWarpStrength = 0.01f;
			}
			glutPostRedisplay();
			break;
		case '[':
			gWarpRadius -= 0.02f;
			if (gWarpRadius < 0.08f) {
				gWarpRadius = 0.08f;
			}
			glutPostRedisplay();
			break;
		case ']':
			gWarpRadius += 0.02f;
			glutPostRedisplay();
			break;
		case 'a':
		case 'A':
			gWarpCenter.x -= 0.03f;
			glutPostRedisplay();
			break;
		case 'd':
		case 'D':
			gWarpCenter.x += 0.03f;
			glutPostRedisplay();
			break;
		case 'w':
		case 'W':
			gWarpCenter.y += 0.03f;
			glutPostRedisplay();
			break;
		case 's':
		case 'S':
			gWarpCenter.y -= 0.03f;
			glutPostRedisplay();
			break;
	}
}

static void init() {
	glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
	glDisable(GL_DEPTH_TEST);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(gWindowWidth, gWindowHeight);
	glutInitWindowPosition(120, 80);
	glutCreateWindow("WarpField 2D Light Ray (OpenGL 2.1)");

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	glutMainLoop();
	return 0;
}
