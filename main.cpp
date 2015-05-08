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

//Spline
std::vector<Point> splines;

Window window;

//Menu indentifier
static int modify = 1;
static int drawMode = 1;

#pragma region Utils
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

#pragma endregion

#pragma region Bézier
Point Decasteljau(float t, std::vector<Point> points)
{
	//std::vector<Point> points = polygons[k].get_points();

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

std::vector<Point> CalculateBezier(std::vector<Point> polygon)
{
	std::vector<Point> bezierR = std::vector<Point>();
	if (polygon.size() >= 3)
	{
		bezierR.push_back(polygon[0]);
		for (int k = 1; k <= pas; ++k)
		{
			Point a = Decasteljau((float)k / (float)pas, polygon);
			bezierR.push_back(a);
		}
	}
	return bezierR;
}

#pragma endregion

#pragma region Splines

std::vector<float> modalVector;

Point baryCentre(Point p1, Point p2, float k)
{
	float x = (1.f - k) * p1.x_get() + k * p2.x_get();
	float y = (1.f - k) * p1.y_get() + k * p2.y_get();
	return Point(x, y);

}

void CalculateSplines()
{
	modalVector = std::vector<float>();
	modalVector.push_back((1.f / 3.f));
	modalVector.push_back((1.f / 3.f));
	modalVector.push_back((1.f / 3.f));

	splines.clear();
	if (polygons[0].get_points().size() > 3)
	{
		int size = polygons[0].get_points().size();
		Point newBezierPoint = Point();
		Point startBezier = Point();

		for (size_t i = 1; i <= size - 3; i++)
		{
			Point p0 = polygons[0].get_points()[i];
			Point p1 = polygons[0].get_points()[i + 1];
			Point p2 = polygons[0].get_points()[i + 2];

			std::vector<Point> controlPoints = std::vector<Point>();
			if (i == 1)
			{
				controlPoints.push_back(polygons[0].get_points()[i - 1]);
				Point r0p0 = baryCentre(p0, p1, modalVector[i]);
				Point r0p1 = baryCentre(p1, p2, modalVector[i + 1]);
				startBezier = baryCentre(r0p0, r0p1, modalVector[i]);
				controlPoints.push_back(r0p0);
				controlPoints.push_back(startBezier);
				std::vector<Point> bezier = CalculateBezier(controlPoints);
				splines.insert(splines.end(), bezier.begin(), bezier.end());

				newBezierPoint = r0p1;
			}
			else {
				controlPoints.push_back(startBezier);
				Point r1p0 = baryCentre(p0, p1, 1 - modalVector[2]);
				Point r0p1 = baryCentre(p1, p2, modalVector[0]);
				startBezier = baryCentre(r1p0, r0p1, modalVector[0]);
				controlPoints.push_back(newBezierPoint);
				controlPoints.push_back(r1p0);
				controlPoints.push_back(startBezier);

				std::vector<Point> bezier = CalculateBezier(controlPoints);
				splines.insert(splines.end(), bezier.begin(), bezier.end());

				newBezierPoint = r0p1;
			}

			if (i == size - 3)
			{
				controlPoints.clear();
				controlPoints.push_back(startBezier);
				controlPoints.push_back(newBezierPoint);
				controlPoints.push_back(p2);

				std::vector<Point> bezier = CalculateBezier(controlPoints);
				splines.insert(splines.end(), bezier.begin(), bezier.end());
			}
		}
	}


}


#pragma endregion

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

		//CalculateBezier();
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
		std::cout << pas << std::endl;
	}
	// -
	else if (key == 45)
	{
		pas -= 1;
		std::cout << pas << std::endl;
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

	//Draw Bezier
	//glColor3d((float)(127.f / 255.f), (float)(48.f / 255.f), (float)(201.f / 255.f));

	//for (size_t i = 0; i < beziers.size(); ++i)
	//{
	//	glBegin(GL_LINE_STRIP);
	//	for (size_t j = 0; j < beziers[i].size(); ++j)
	//	{
	//		glVertex2f(beziers[i][j].x_get(), beziers[i][j].y_get());
	//	}
	//	glEnd();
	//}

	//Draw Spline
	glColor3d((float)(255.f / 255.f), (float)(94.f / 255.f), (float)(0.f / 255.f));

	/*for (size_t i = 0; i < beziers.size(); ++i)
	{*/
	glBegin(GL_LINE_STRIP);
	for (size_t j = 0; j < splines.size(); ++j)
	{
		glVertex2f(splines[j].x_get(), splines[j].y_get());
	}
	glEnd();
	//}
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


	for (int l = 0; l < polygons.size(); ++l)
	{
		beziers[l] = CalculateBezier(polygons[l].get_points());
	}
	CalculateSplines();

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