
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

struct Vec2 {
	float x;
	float y;
};

struct WarpHole {
	Vec2 center;
	float radius;
	float coreRadius;
	float strength;
};

struct Wormhole {
	WarpHole holeA;
	WarpHole holeB;
};

static int gWindowWidth = 1000;
static int gWindowHeight = 700;

static Wormhole gWormhole = {
	{{-0.45f, 0.08f}, 0.28f, 0.06f, 0.22f},
	{{0.92f, -0.08f}, 0.28f, 0.06f, 0.22f}
};

static Vec2 gRayStart = {-0.95f, -0.52f};
static Vec2 gRayDir = {1.0f, 0.16f};
static Vec2 gCubeCenter = {0.08f, 0.38f};
static float gCubeHalfSize = 0.12f;
static float gRenderWarpScale = 0.045f;

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

static float vecDot(const Vec2 a, const Vec2 b) {
	return a.x * b.x + a.y * b.y;
}

static Vec2 vecNormalize(const Vec2 v) {
	const float len = vecLength(v);
	if (len <= 1e-6f) {
		return {0.0f, 0.0f};
	}
	return {v.x / len, v.y / len};
}

static float clampf(const float value, const float minValue, const float maxValue) {
	if (value < minValue) {
		return minValue;
	}
	if (value > maxValue) {
		return maxValue;
	}
	return value;
}

static Vec2 warpFieldFromHole(const Vec2 p, const WarpHole& hole) {
	const Vec2 toCenter = vecSub(hole.center, p);
	const float distance = vecLength(toCenter);
	const float softened = distance + 0.02f;
	const float influence = std::exp(-(distance * distance) / (hole.radius * hole.radius));
	const float magnitude = (hole.strength * influence) / (softened * softened);
	return vecScale(vecNormalize(toCenter), magnitude);
}

static Vec2 warpField(const Vec2 p) {
	const Vec2 fieldA = warpFieldFromHole(p, gWormhole.holeA);
	const Vec2 fieldB = warpFieldFromHole(p, gWormhole.holeB);
	return vecAdd(fieldA, fieldB);
}

static Vec2 warpRenderPoint(const Vec2 p) {
	return vecAdd(p, vecScale(warpField(p), gRenderWarpScale));
}

static void drawCircle(const Vec2 center, const float radius, const GLenum mode, const int segments) {
	glBegin(mode);
	for (int i = 0; i < segments; ++i) {
		const float t = 2.0f * 3.14159265359f * static_cast<float>(i) / static_cast<float>(segments);
		const float x = center.x + std::cos(t) * radius;
		const float y = center.y + std::sin(t) * radius;
		glVertex2f(x, y);
	}
	glEnd();
}

static void drawWarpFieldOutline(const WarpHole& hole) {
	glColor3f(0.05f, 0.55f, 0.90f);
	glLineWidth(2.0f);
	drawCircle(hole.center, hole.radius, GL_LINE_LOOP, 96);
	glLineWidth(1.0f);
}

static void drawWarpCore(const WarpHole& hole) {
	glColor3f(0.18f, 0.86f, 1.0f);
	drawCircle(hole.center, hole.coreRadius, GL_POLYGON, 48);
}

static Vec2 teleportToOppositeSide(
	const Vec2 entryPoint,
	const Vec2 previousPoint,
	const WarpHole& source,
	const WarpHole& destination,
	const float margin
) {
	Vec2 normal = vecNormalize(vecSub(entryPoint, source.center));
	if (vecLength(normal) <= 1e-6f) {
		normal = vecNormalize(vecSub(previousPoint, source.center));
	}
	if (vecLength(normal) <= 1e-6f) {
		normal = {1.0f, 0.0f};
	}

	// Exit on the opposite side of destination mouth and keep a small outside margin.
	return vecAdd(destination.center, vecScale(normal, -(destination.coreRadius + margin)));
}

static void drawRayPath(Vec2 position) {
	Vec2 direction = vecNormalize(gRayDir);

	const float stepLength = 0.006f;
	const int maxSteps = 1800;
	const float exitMargin = 0.008f;

	glColor3f(1.0f, 0.96f, 0.36f);
	glLineWidth(2.8f);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < maxSteps; ++i) {
		glVertex2f(position.x, position.y);

		const Vec2 previousPosition = position;
		const Vec2 accel = warpField(position);
		direction = vecNormalize(vecAdd(direction, vecScale(accel, stepLength)));
		Vec2 nextPosition = vecAdd(position, vecScale(direction, stepLength));
		int didTeleport = 0;

		const float prevDistA = vecLength(vecSub(previousPosition, gWormhole.holeA.center));
		const float nextDistA = vecLength(vecSub(nextPosition, gWormhole.holeA.center));
		const float prevDistB = vecLength(vecSub(previousPosition, gWormhole.holeB.center));
		const float nextDistB = vecLength(vecSub(nextPosition, gWormhole.holeB.center));

		if (prevDistA >= gWormhole.holeA.coreRadius && nextDistA < gWormhole.holeA.coreRadius) {
			nextPosition = teleportToOppositeSide(
				nextPosition,
				previousPosition,
				gWormhole.holeA,
				gWormhole.holeB,
				exitMargin
			);
			didTeleport = 1;
		} else if (prevDistB >= gWormhole.holeB.coreRadius && nextDistB < gWormhole.holeB.coreRadius) {
			nextPosition = teleportToOppositeSide(
				nextPosition,
				previousPosition,
				gWormhole.holeB,
				gWormhole.holeA,
				exitMargin
			);
			didTeleport = 1;
		}

		if (didTeleport) {
			// If direction is inward at the exit mouth, project it outward to avoid re-entering core.
			const WarpHole& exitHole = (prevDistA >= gWormhole.holeA.coreRadius && nextDistA < gWormhole.holeA.coreRadius)
				? gWormhole.holeB
				: gWormhole.holeA;
			const Vec2 toCenter = vecNormalize(vecSub(exitHole.center, nextPosition));
			const float inwardAmount = vecDot(direction, toCenter);
			if (inwardAmount > 0.0f) {
				direction = vecNormalize(vecSub(direction, vecScale(toCenter, inwardAmount * 1.2f)));
			}

			// Break strip so no artificial segment connects both mouths.
			glEnd();
			glBegin(GL_LINE_STRIP);
			glVertex2f(nextPosition.x, nextPosition.y);
		}

		position = nextPosition;

		if (position.x > 1.10f || position.x < -1.10f || position.y > 1.10f || position.y < -1.10f) {
			break;
		}
	}
	glEnd();
	glLineWidth(1.0f);
}

static void drawWarpedSquare() {
	const Vec2 v0 = {gCubeCenter.x - gCubeHalfSize, gCubeCenter.y - gCubeHalfSize};
	const Vec2 v1 = {gCubeCenter.x + gCubeHalfSize, gCubeCenter.y - gCubeHalfSize};
	const Vec2 v2 = {gCubeCenter.x + gCubeHalfSize, gCubeCenter.y + gCubeHalfSize};
	const Vec2 v3 = {gCubeCenter.x - gCubeHalfSize, gCubeCenter.y + gCubeHalfSize};

	const Vec2 w0 = warpRenderPoint(v0);
	const Vec2 w1 = warpRenderPoint(v1);
	const Vec2 w2 = warpRenderPoint(v2);
	const Vec2 w3 = warpRenderPoint(v3);

	glColor3f(0.95f, 0.44f, 0.20f);
	glBegin(GL_POLYGON);
	glVertex2f(w0.x, w0.y);
	glVertex2f(w1.x, w1.y);
	glVertex2f(w2.x, w2.y);
	glVertex2f(w3.x, w3.y);
	glEnd();

	glColor3f(1.0f, 0.76f, 0.54f);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(w0.x, w0.y);
	glVertex2f(w1.x, w1.y);
	glVertex2f(w2.x, w2.y);
	glVertex2f(w3.x, w3.y);
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
	drawWarpFieldOutline(gWormhole.holeA);
	drawWarpFieldOutline(gWormhole.holeB);
	drawWarpCore(gWormhole.holeA);
	drawWarpCore(gWormhole.holeB);
	drawWarpedSquare();
	drawRayPath(gRayStart);

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
			gWormhole.holeA.strength += 0.02f;
			gWormhole.holeB.strength += 0.02f;
			glutPostRedisplay();
			break;
		case '-':
		case '_':
			gWormhole.holeA.strength = clampf(gWormhole.holeA.strength - 0.02f, 0.01f, 5.0f);
			gWormhole.holeB.strength = gWormhole.holeA.strength;
			glutPostRedisplay();
			break;
		case '[':
			gWormhole.holeA.radius = clampf(gWormhole.holeA.radius - 0.02f, 0.08f, 0.8f);
			gWormhole.holeB.radius = gWormhole.holeA.radius;
			glutPostRedisplay();
			break;
		case ']':
			gWormhole.holeA.radius = clampf(gWormhole.holeA.radius + 0.02f, 0.08f, 0.8f);
			gWormhole.holeB.radius = gWormhole.holeA.radius;
			glutPostRedisplay();
			break;
		case 'a':
		case 'A':
			gWormhole.holeA.center.x -= 0.03f;
			gWormhole.holeB.center.x -= 0.03f;
			glutPostRedisplay();
			break;
		case 'd':
		case 'D':
			gWormhole.holeA.center.x += 0.03f;
			gWormhole.holeB.center.x += 0.03f;
			glutPostRedisplay();
			break;
		case 'w':
		case 'W':
			gWormhole.holeA.center.y += 0.03f;
			gWormhole.holeB.center.y += 0.03f;
			glutPostRedisplay();
			break;
		case 's':
		case 'S':
			gWormhole.holeA.center.y -= 0.03f;
			gWormhole.holeB.center.y -= 0.03f;
			glutPostRedisplay();
			break;
		case 'f':
		case 'F':
			gWormhole.holeA.center.x -= 0.03f;
			glutPostRedisplay();
			break;
		case 'h':
		case 'H':
			gWormhole.holeA.center.x += 0.03f;
			glutPostRedisplay();
			break;
		case 't':
		case 'T':
			gWormhole.holeA.center.y += 0.03f;
			glutPostRedisplay();
			break;
		case 'g':
		case 'G':
			gWormhole.holeA.center.y -= 0.03f;
			glutPostRedisplay();
			break;
		case 'j':
		case 'J':
			gCubeCenter.x -= 0.03f;
			glutPostRedisplay();
			break;
		case 'l':
		case 'L':
			gCubeCenter.x += 0.03f;
			glutPostRedisplay();
			break;
		case 'i':
		case 'I':
			gCubeCenter.y += 0.03f;
			glutPostRedisplay();
			break;
		case 'k':
		case 'K':
			gCubeCenter.y -= 0.03f;
			glutPostRedisplay();
			break;
		case '4':
			gWormhole.holeB.center.x -= 0.03f;
			glutPostRedisplay();
			break;
		case '6':
			gWormhole.holeB.center.x += 0.03f;
			glutPostRedisplay();
			break;
		case '8':
			gWormhole.holeB.center.y += 0.03f;
			glutPostRedisplay();
			break;
		case '5':
			gWormhole.holeB.center.y -= 0.03f;
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
