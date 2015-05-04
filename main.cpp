#include "stdafx.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <numeric>
#include <limits>

#include "Point.h"
#include "CPolygon.h"
#include "Window.h"
#include "CVector.h"
#include "Edge.h"
#include "Node.h"


#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

float height = 640.0f;
float width = 640.0f;

int currentPolygon = 0;
std::vector<CPolygon> polygons;

int pas = 20;

//Bezier
std::vector<std::vector<Point>> beziers;

Window window;

//Menu indentifier
static int modify = 1;
static int drawMode = 1;

float calculateSlope(Point a, Point b)
{
	if (std::fabs(b.x_get() - a.x_get()) < 0.01){
		return INFINITY;
	}
	return (b.y_get() - a.y_get()) / (b.x_get() - a.x_get());
}


float convertOpenGLToViewportCoordinate(float x)
{
	return (x + 1) / 2;
}

float convertViewportToOpenGLCoordinate(float x)
{
	return (x * 2) - 1;
}


Point Decasteljau(int k, float t)
{
	std::vector<Point> points = polygons[k].get_points();

	std::vector<std::vector<Point>> arbre = std::vector<std::vector<Point>>();
	arbre.push_back(points);
	for (int i = 0; i < points.size() - 1; ++i)
	{
	std:vector<Point> nextLevel = std::vector <Point>();
		for (int j = 0; j < arbre[i].size() - 1; ++j)
		{
			float x = (1.0 - t) * arbre[i][j].x_get() + t * arbre[i][j + 1.0].x_get();
			float y = (1.0 - t) * arbre[i][j].y_get() + t * arbre[i][j + 1.0].y_get();
			Point a = Point(x, y);
			nextLevel.push_back(a);
		}
		arbre.push_back(nextLevel);
	}
	return arbre[points.size() - 1][0];
}

void CalculateBezier()
{
	for (int l = 0; l < polygons.size(); ++l)
	{
		if (polygons[l].get_points().size() >= 3)
		{
			beziers[l].clear();
			beziers[l].push_back(polygons[l].get_points()[0]);
			for (int k = 1; k <= pas; ++k)
			{
				std::cout << pas << std::endl;
				Point a = Decasteljau(l, (float)k / (float)pas);
				beziers[l].push_back(a);
			}
		}
	}
}

#pragma mark GLUT
#pragma region GLUT

void clearAll()
{
	polygons.clear();
	CPolygon p;
	polygons.push_back(p);
	currentPolygon = 0;
	window.clearPoints();
	beziers.clear();
	beziers.push_back(std::vector<Point>());
}

void MouseButton(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			float new_x = convertViewportToOpenGLCoordinate(x / (float)glutGet(GLUT_WINDOW_WIDTH));

			float new_y = -convertViewportToOpenGLCoordinate(y / (float)glutGet(GLUT_WINDOW_HEIGHT));

			Point p(new_x, new_y);

			if (drawMode)
				polygons[currentPolygon].addPoint(p);
			else
				window.add_point(p);

		}
	}
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			float index = convertViewportToOpenGLCoordinate(y / (float)glutGet(GLUT_WINDOW_HEIGHT));
			index = convertOpenGLToViewportCoordinate(index);
			index *= glutGet(GLUT_WINDOW_HEIGHT);

		}
	}
}

void keyPressed(unsigned char key, int x, int y)
{
	if (key == 13)
	{
		//std::vector<CPolygon> p= windowing(polygons, window);
		//polygons = p;

		CalculateBezier();
	}
	else if (key == 'c')
	{
		clearAll();
	}
	else if (key == 'f')
	{
		//FillingLCALoop(polygons);
	}
	// +
	else if (key == 43)
	{
		pas += 1;
	}
	// -
	else if (key == 45)
	{
		pas -= 1;
	}
}

void update()
{
	glutPostRedisplay();
}

void DrawPolygon()
{
	//Polygon
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE_STRIP);
	glColor3d(0, 0, 0);

	std::vector<Point> points;
	for (int i = 0; i < polygons.size(); ++i){

		points = polygons[i].get_points();

		if (points.size() >= 2){

			glBegin(GL_LINE_STRIP);
			for (std::size_t i = 0; i < points.size(); ++i) {
				glVertex2f(points[i].x_get(), points[i].y_get());
			}
			glEnd();
		}
	}
	glColor3d((float)(127.f / 255.f), (float)(48.f / 255.f), (float)(201.f / 255.f));

	for (size_t i = 0; i < beziers.size(); ++i)
	{
		glBegin(GL_LINE_STRIP);
		for (size_t j = 0; j < beziers[i].size(); ++j)
		{
			glVertex2f(beziers[i][j].x_get(), beziers[i][j].y_get());
		}
		glEnd();
	}
}

void selectDraw(int selection) {
	CPolygon p;
	switch (selection)
	{
	case 11: drawMode = 1;
		if (polygons[currentPolygon].get_points().size() > 2){
			polygons.push_back(p);
			beziers.push_back(std::vector<Point>());
			currentPolygon++;
		}
		break;
	case 12: drawMode = 0;
		break;
	}
	glutPostRedisplay();
}

void selectModify(int selection) {
	std::vector<CPolygon> p;
	switch (selection) {
	case 1:
		//p = windowing(polygons, window);
		polygons = p;
		break;
	case 2:
		//FillingLCALoop(polygons);
		break;
	case 0:
		exit(0);
	}
	glutPostRedisplay();
}

void select(int selection) {
	switch (selection)
	{
	case 3:
		clearAll();
		break;
	case 0:
		exit(0);
	}
	glutPostRedisplay();
}

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1, 1, 1, 1);

	CalculateBezier();
	//    DrawPolygon(window.get_points());
	DrawPolygon();

	glutSwapBuffers();
}

static GLfloat view_rotx = 20.0F;
static GLfloat view_roty = 30.0F;
static GLfloat view_rotz = 0.0F;

static void special(int k, int x, int y) {
	switch (k) {
	case GLUT_KEY_UP: view_rotx += 2.0;
		break;
	case GLUT_KEY_DOWN: view_rotx -= 2.0;
		break;
	case GLUT_KEY_LEFT: view_roty += 2.0;
		break;
	case GLUT_KEY_RIGHT: view_roty -= 2.0;
		break;
	}
	glutPostRedisplay();
}


int main(int argc, char **argv) {

	CPolygon p;
	polygons.push_back(p);

	beziers.push_back(std::vector<Point>());

	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Projet Math - GLUT");
	//glEnable(GL_LINE_SMOOTH);

	// register callbacks
	glutDisplayFunc(renderScene);
	glutMouseFunc(MouseButton);
	glutIdleFunc(update);
	glutKeyboardFunc(keyPressed);

	int drawMenu = glutCreateMenu(selectDraw);
	glutAddMenuEntry("Bézier", 11);
	glutAddMenuEntry("Window", 12);
	int modifyMenu = glutCreateMenu(selectModify);
	glutAddMenuEntry("Windowing", 1);
	glutAddMenuEntry("Filling", 2);
	glutCreateMenu(select);
	glutAddSubMenu("Draw", drawMenu);
	glutAddSubMenu("Modify", modifyMenu);
	glutAddMenuEntry("Clear", 3);
	glutAddMenuEntry("Quitter", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	//glutKeyboardFunc(key);
	glutSpecialFunc(special);

	// enter GLUT event processing cycle
	glutMainLoop();
	return 1;
}

#pragma endregion