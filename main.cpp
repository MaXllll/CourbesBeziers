#include "stdafx.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <numeric>
#include <limits>
#include <math.h>

#include "Point.h"
#include "CPolygon.h"
#include "Window.h"
#include "CVector.h"
#include "Edge.h"
#include "Node.h"

#include <boost/numeric/ublas/matrix.hpp>


#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#define TRANSLATE 5
#define SCALE 6
#define ROTATE 7 

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
static int drawmodeEdit = 1;


// Color picker attributes
#define WIDTH 750
#define HEIGHT 750

int i = 0, mousex, mousey;
//current colors
float pick[3];

//Bit modeEdit , if 0 the user can add new points, if 1 the user can move existing points 
bool modeEdit = 1;

bool modeJoin = 0;

// Bit Select/Move
bool select_move = 0;
int sp_indexPoly = -1;
int sp_indexPoint = -1;
int transfMode = TRANSLATE;

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

#pragma region B�zier
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

std::vector<float> nodalVector;

std::vector<std::vector<Point>> splines;

std::vector<std::vector<Point>> controlPolygons;


Point baryCentre(Point p1, Point p2, float k)
{
	float x = (1.f - k) * p1.x_get() + k * p2.x_get();
	float y = (1.f - k) * p1.y_get() + k * p2.y_get();
	return Point(x, y);

}

void CalculateSplines()
{
	nodalVector = std::vector<float>();
	nodalVector.push_back((1.f / 3.f));
	nodalVector.push_back((1.f / 3.f));
	nodalVector.push_back((1.f / 3.f));

	splines.clear();
	controlPolygons.clear();
	splines = std::vector<std::vector<Point>>();

	for (size_t j= 0; j < polygons.size(); j++)
	{ 
		if (polygons[j].get_points().size() > 4)
		{
			int size = polygons[j].get_points().size();
			Point newBezierPoint = Point();
			Point startBezier = Point();
			splines.push_back(std::vector<Point>());
			controlPolygons.push_back(std::vector<Point>());

			for (size_t i = 1; i <= size - 3; i++)
			{
				Point p0 = polygons[j].get_points()[i];
				Point p1 = polygons[j].get_points()[i + 1];
				Point p2 = polygons[j].get_points()[i + 2];

				std::vector<Point> controlPoints = std::vector<Point>();
				if (i == 1)
				{
					controlPoints.push_back(polygons[j].get_points()[i - 1]);
					Point r0p0 = baryCentre(p0, p1, nodalVector[i]);
					Point r0p1 = baryCentre(p1, p2, nodalVector[i + 1]);
					startBezier = baryCentre(r0p0, r0p1, nodalVector[i]);
					controlPoints.push_back(p0);
					controlPoints.push_back(r0p0);
					controlPoints.push_back(startBezier);
					std::vector<Point> bezier = CalculateBezier(controlPoints);
					splines[j].insert(splines[j].end(), bezier.begin(), bezier.end());

					newBezierPoint = r0p1;

					//Filling control points polygon
					controlPolygons[j].push_back(r0p0);
					controlPolygons[j].push_back(r0p1);
				}
				else {
					controlPoints.push_back(startBezier);
					Point r1p0 = baryCentre(p0, p1, 1 - nodalVector[2]);
					Point r0p1 = baryCentre(p1, p2, nodalVector[0]);
					startBezier = baryCentre(r1p0, r0p1, nodalVector[0]);
					controlPoints.push_back(newBezierPoint);
					controlPoints.push_back(r1p0);
					controlPoints.push_back(startBezier);

					std::vector<Point> bezier = CalculateBezier(controlPoints);
					splines[j].insert(splines[j].end(), bezier.begin(), bezier.end());

					newBezierPoint = r0p1;

					//Filling control points polygon
					controlPolygons[j].push_back(r1p0);
					controlPolygons[j].push_back(r0p1);
				}

				if (i == size - 3)
				{
					controlPoints.clear();
					controlPoints.push_back(startBezier);
					controlPoints.push_back(newBezierPoint);
					controlPoints.push_back(p2);

					std::vector<Point> bezier = CalculateBezier(controlPoints);
					splines[j].insert(splines[j].end(), bezier.begin(), bezier.end());

					//Filling control points polygon
					controlPolygons[j].push_back(newBezierPoint);
				}
			}
		}
	}


}


#pragma endregion

#pragma region TRANSFORMATION

Point translate(Point a, float diffX, float diffY)
{
	boost::numeric::ublas::matrix<float> translation = boost::numeric::ublas::identity_matrix<float>(3);
	translation(0, 2) = diffX;
	translation(1, 2) = diffY;

	boost::numeric::ublas::matrix<float> point = boost::numeric::ublas::scalar_matrix<float>(3, 1);
	point(0, 0) = a.x_get();
	point(1, 0) = a.y_get();

	boost::numeric::ublas::matrix<float> newPoint = boost::numeric::ublas::prod(translation, point);

	return Point(newPoint(0, 0), newPoint(1, 0));
}

void TranslateAll(float x, float y){
	for (int i = 0; i < polygons.size(); i++){
		std::vector<Point> currentPoints = polygons[i].get_points();
		for (int j = 0; j < polygons[i].get_points().size(); j++){
			currentPoints[j] = translate(currentPoints[j], x, y);
		}
		polygons[i].set_points(currentPoints);
	}

}


Point barycentreCurve(CPolygon curve){

	std::vector<Point> currentPoints = curve.get_points();
	Point barycentre;
	int size = curve.get_points().size();
	for (int j = 0; j < size; j++){
		barycentre.x_set(barycentre.x_get() + currentPoints[j].x_get());
		barycentre.y_set(barycentre.y_get() + currentPoints[j].y_get());
	}
	barycentre.x_set(barycentre.x_get()/size);
	barycentre.y_set(barycentre.y_get() / size);
	return barycentre;
}

Point scale(Point a, float diffX, float diffY, float sX, float sY)
{
	boost::numeric::ublas::matrix<float> translation0 = boost::numeric::ublas::identity_matrix<float>(3);
	translation0(0, 2) = -diffX;
	translation0(1, 2) = -diffY;

	boost::numeric::ublas::matrix<float> scale = boost::numeric::ublas::identity_matrix<float>(3);
	scale(0, 0) = sX;
	scale(1, 1) = sY;

	boost::numeric::ublas::matrix<float> translation1 = boost::numeric::ublas::identity_matrix<float>(3);
	translation1(0, 2) = diffX;
	translation1(1, 2) = diffY;

	boost::numeric::ublas::matrix<float> point = boost::numeric::ublas::scalar_matrix<float>(3, 1);
	point(0, 0) = a.x_get();
	point(1, 0) = a.y_get();


	boost::numeric::ublas::matrix<float> newPoint = boost::numeric::ublas::prod(translation0, point);
	//boost::numeric::ublas::matrix<float> newPoint = boost::numeric::ublas::prod(translation1, point);
	newPoint = boost::numeric::ublas::prod(scale, newPoint);
	newPoint = boost::numeric::ublas::prod(translation1, newPoint);
	
	return Point(newPoint(0, 0), newPoint(1, 0));
}

void ScaleAll(float x, float y){
	Point worldCenter(0, 0);
	for (int i = 0; i < polygons.size(); i++){
		/*
		Center of scale is the first point
		std::vector<Point> currentPoints = polygons[i].get_points();
		float diffX = currentPoints[0].x_get() - worldCenter.x_get();
		float diffY = currentPoints[0].y_get() - worldCenter.y_get();
		*/
		Point barycentre = barycentreCurve(polygons[i]);
		std::vector<Point> currentPoints = polygons[i].get_points();
		float diffX = barycentre.x_get() - worldCenter.x_get();
		float diffY = barycentre.y_get() - worldCenter.y_get();

		for (int j = 0; j < polygons[i].get_points().size(); j++){
			currentPoints[j] = scale(currentPoints[j], diffX, diffY, x, y);
		}
		polygons[i].set_points(currentPoints);
	}

}

Point rotate(Point a, float diffX, float diffY, float angle)
{
	boost::numeric::ublas::matrix<float> translation0 = boost::numeric::ublas::identity_matrix<float>(3);
	translation0(0, 2) = -diffX;
	translation0(1, 2) = -diffY;

	boost::numeric::ublas::matrix<float> scale = boost::numeric::ublas::identity_matrix<float>(3);
	scale(0, 0) = cos(angle);
	scale(1, 0) = sin(angle);
	scale(0, 1) = -sin(angle);
	scale(1, 1) = cos(angle);

	boost::numeric::ublas::matrix<float> translation1 = boost::numeric::ublas::identity_matrix<float>(3);
	translation1(0, 2) = diffX;
	translation1(1, 2) = diffY;

	boost::numeric::ublas::matrix<float> point = boost::numeric::ublas::scalar_matrix<float>(3, 1);
	point(0, 0) = a.x_get();
	point(1, 0) = a.y_get();


	boost::numeric::ublas::matrix<float> newPoint = boost::numeric::ublas::prod(translation0, point);
	//boost::numeric::ublas::matrix<float> newPoint = boost::numeric::ublas::prod(translation1, point);
	newPoint = boost::numeric::ublas::prod(scale, newPoint);
	newPoint = boost::numeric::ublas::prod(translation1, newPoint);

	return Point(newPoint(0, 0), newPoint(1, 0));
}

void RotateAll(float angle){
	std::cout << sp_indexPoly << std::endl;
	if (sp_indexPoly != -1){
		Point worldCenter(0, 0);
		Point barycentre = barycentreCurve(polygons[i]);
		std::vector<Point> currentPoints = polygons[sp_indexPoly].get_points();
		float diffX = barycentre.x_get() - worldCenter.x_get();
		float diffY = barycentre.y_get() - worldCenter.y_get();

		for (int j = 0; j < polygons[i].get_points().size(); j++){
			currentPoints[j] = rotate(currentPoints[j], diffX, diffY, angle);
		}
		polygons[i].set_points(currentPoints);
	}
	else{
		Point worldCenter(0, 0);
		for (int i = 0; i < polygons.size(); i++){
			/*
			Center of scale is the first point
			std::vector<Point> currentPoints = polygons[i].get_points();
			float diffX = currentPoints[0].x_get() - worldCenter.x_get();
			float diffY = currentPoints[0].y_get() - worldCenter.y_get();
			*/
			Point barycentre = barycentreCurve(polygons[i]);
			std::vector<Point> currentPoints = polygons[i].get_points();
			float diffX = barycentre.x_get() - worldCenter.x_get();
			float diffY = barycentre.y_get() - worldCenter.y_get();

			for (int j = 0; j < polygons[i].get_points().size(); j++){
				currentPoints[j] = rotate(currentPoints[j], diffX, diffY, angle);
			}
			polygons[i].set_points(currentPoints);
		}
	}
}

#pragma endregion

#pragma region Join

void joinC0(int icurve1, int icurve2)
{
	std::vector<Point> curve1 = polygons[icurve1].get_points();
	std::vector<Point> curve2 = polygons[icurve2].get_points();

	Point lastPoint = curve1[curve1.size() - 1];
	Point firstPoint = curve2[0];

	float diffX = lastPoint.x_get() - firstPoint.x_get();
	float diffY = lastPoint.y_get() - firstPoint.y_get();

	for (size_t i = 0; i < curve2.size(); i++)
	{
		curve2[i] = translate(curve2[i], diffX, diffY);
	}

	polygons[icurve2].set_points(curve2);
}

void joinC1(int icurve1, int icurve2)
{
	joinC0(icurve1, icurve2);
	std::vector<Point> curve1 = polygons[icurve1].get_points();
	std::vector<Point> curve2 = polygons[icurve2].get_points();

	Point beforeLastPoint = curve1[curve1.size() - 2];
	Point lastPoint = curve1[curve1.size() - 1];

	Point firstPoint = curve2[0];
	Point secondPoint = curve2[1];

	float diffX = lastPoint.x_get() - beforeLastPoint.x_get();
	float diffY = lastPoint.y_get() - beforeLastPoint.y_get();

	float diffX2 = secondPoint.x_get() - firstPoint.x_get();
	float diffY2 = secondPoint.y_get() - firstPoint.y_get();

	float scalar = diffX * diffX2 + diffY * diffY2;

	float qX = diffX / scalar;
	float qY = diffY / scalar;
	std::cout << qX << std::endl;
	std::cout << qY << std::endl;

	curve2[1] = Point(lastPoint.x_get() + (diffX / qX), lastPoint.y_get() + (diffY / qX));

	polygons[icurve2].set_points(curve2);
}

void joinC2(int icurve1, int icurve2)
{
	joinC0(icurve1, icurve2);
	std::vector<Point> curve1 = polygons[icurve1].get_points();
	std::vector<Point> curve2 = polygons[icurve2].get_points();
	
	Point beforeBeforeLastPoint = curve1[curve1.size() - 3];
	Point beforeLastPoint = curve1[curve1.size() - 2];
	Point lastPoint = curve1[curve1.size() - 1];

	Point contructPoint = Point(beforeLastPoint.x_get() + (beforeLastPoint.x_get() - beforeBeforeLastPoint.x_get()), beforeLastPoint.y_get() + (beforeLastPoint.y_get() - beforeBeforeLastPoint.y_get()));

	float diffX = lastPoint.x_get() - beforeLastPoint.x_get();
	float diffY = lastPoint.y_get() - beforeLastPoint.y_get();

	curve2[1] = Point(lastPoint.x_get() + diffX, lastPoint.y_get() + diffY);
	curve2[2] = Point(curve2[1].x_get() + (curve2[1].x_get() - contructPoint.x_get()), curve2[1].y_get() + (curve2[1].y_get() - contructPoint.y_get()));

	polygons[icurve2].set_points(curve2);
}

#pragma endregion

#pragma region COLOR PICKER
void hex(void){
	glBegin(GL_POLYGON);
	glColor3f(1, 0, 0);           //red
	glVertex2f(0, 2);           //top
	glColor3f(1, .38, .01);       //orange
	glVertex2f(2, 1);           //top right
	glColor3f(1, 1, 0);           //yellow
	glVertex2f(2, -1);          //bottom right
	glColor3f(0, 1, 0);           //green
	glVertex2f(0, -2);          //bottom
	glColor3f(0, 0, 1);           //blue
	glVertex2f(-2, -1);         //bottom left
	glColor3f(.8, 0, .8);         //purple
	glVertex2f(-2, 1);          //top left
	glEnd();
}

void square(void){
	glBegin(GL_POLYGON);
	glVertex2i(1, -1);
	glVertex2i(1, 1);
	glVertex2i(-1, 1);
	glVertex2i(-1, -1);
	glEnd();
}

void mouse(int button, int state, int x, int y){
	// Save the mouse position
	mousex = x;
	mousey = y;

	glReadPixels(mousex, mousey, 1, 1, GL_RGB, GL_FLOAT, &pick);
	cout << pick[0] << "pick";
	cout << " mouse x " << mousex << "\n";
	cout << " mouse y " << mousey << "\n";
	fflush(stdout);

	cout << "pick R: " << pick[1] << "\n";
	cout << "pick G: " << pick[0] << "\n";
	cout << "pick B: " << pick[2] << "\n";
	glutPostRedisplay();
	glutHideWindow();
}



void draw(void){
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();
	glScalef(20, 20, 1);
	hex();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(100, 100, 0);
	glColor3f(pick[1], pick[0], pick[2]);
	glScalef(20, 20, 1);
	square();
	glPopMatrix();

	glFlush();

}

void my_init(void){
	//glClearColor(0, 0, 0, 1);               //sets clear color
	//glLineWidth(4);
	gluOrtho2D(-100, 100, -100, 100);       //sets origin
}
void openWindow(){
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Color Picker");
	glutDisplayFunc(draw);
	glutMouseFunc(mouse);

	my_init();

	glClearColor(1, 1, 1, 0);

	glutMainLoop();                         //listens for events
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

void AddPoint(int x, int y){
	float new_x = convertViewportToOpenGLCoordinate(x / (float)glutGet(GLUT_WINDOW_WIDTH));

	float new_y = -convertViewportToOpenGLCoordinate(y / (float)glutGet(GLUT_WINDOW_HEIGHT));

	Point p(new_x, new_y);

	if (drawmodeEdit)
		polygons[currentPolygon].addPoint(p);
	else
		window.add_point(p);
}

void SelectPoint(int x, int y){
	float new_x = convertViewportToOpenGLCoordinate(x / (float)glutGet(GLUT_WINDOW_WIDTH));

	float new_y = -convertViewportToOpenGLCoordinate(y / (float)glutGet(GLUT_WINDOW_HEIGHT));

	Point p(new_x, new_y);

	float chosenMaxDistance = 0.15;
	for (int i = 0; i < polygons.size(); i++){
		std::vector<Point> currentPoints = polygons[i].get_points();
		for (int j = 0; j < polygons[i].get_points().size(); j++){
			//currentPoints[j];
			// distance between two points
			float distance = sqrt((p.x_get() - currentPoints[j].x_get())*(p.x_get() - currentPoints[j].x_get()) + (p.y_get() - currentPoints[j].y_get())*(p.y_get() - currentPoints[j].y_get()));
			if (chosenMaxDistance > distance){
				sp_indexPoly = i;
				sp_indexPoint = j;
				std::cout << "Poly : " << sp_indexPoly << std::endl;
				break;
			}
		}
	}
}

void MovePoint(int x, int y){
	if (!modeEdit && sp_indexPoint != -1){
		float new_x = convertViewportToOpenGLCoordinate(x / (float)glutGet(GLUT_WINDOW_WIDTH));
		float new_y = -convertViewportToOpenGLCoordinate(y / (float)glutGet(GLUT_WINDOW_HEIGHT));
		Point p(new_x, new_y);
		std::vector<Point> currentPoints = polygons[sp_indexPoly].get_points();
		currentPoints[sp_indexPoint] = p;
		polygons[sp_indexPoly].set_points(currentPoints);
	}
}

void MouseButton(int button, int state, int x, int y)
{
	std::cout << "MouseButton : " << std::endl;
	sp_indexPoly = -1;
	sp_indexPoint = -1;
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			if (modeEdit){
				AddPoint(x,y);
			}
			else{
				SelectPoint(x, y);
			}
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

void changemodeEdit(){
	modeEdit = !modeEdit;
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
	else if (key == 'p'){
		openWindow();
	}
	else if (key == 'e'){
		std::cout << "trololozd" << std::endl;
		changemodeEdit();
	}
	if (transfMode == TRANSLATE){
	    if (key == 'j'){
			TranslateAll(0,-0.1);
		}
		else if (key == 'u'){
			TranslateAll(0, 0.1);
		}
		else if (key == 'h'){
			TranslateAll(-0.1, 0);
		}
		else if (key == 'k'){
			TranslateAll(0.1, 0);
		}
	}
	else if (transfMode == SCALE){
		if (key == 'j'){
			ScaleAll(1, 0.8);
		}
		else if (key == 'u'){
			ScaleAll(1, 1.2);
		}
		else if (key == 'h'){
			ScaleAll(1.2, 1);
		}
		else if (key == 'k'){
			ScaleAll(0.8, 1);
		}
	}
	else if (transfMode == ROTATE){
		if (key == 'k'){
			RotateAll(90);
		}
		else if (key == 'u'){
			RotateAll(180);
		}
		else if (key == 'j'){
			RotateAll(-180);
		}
		else if (key == 'h'){
			RotateAll(-90);
		}
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

	//Draw Spline
	glColor3d((float)(255.f / 255.f), (float)(94.f / 255.f), (float)(0.f / 255.f));
	glColor3d(pick[1], pick[0], pick[2]);
	for (size_t i = 0; i < splines.size(); ++i)
	{
		glBegin(GL_LINE_STRIP);
		for (size_t j = 0; j < splines[i].size(); ++j)
		{
			glVertex2f(splines[i][j].x_get(), splines[i][j].y_get());
		}
		glEnd();
	}

	//Draw Control Polygons
	for (size_t i = 0; i < splines.size(); ++i)
	{
		glBegin(GL_LINE_STRIP);
		for (size_t j = 0; j < controlPolygons[i].size(); j += 1)
		{
			glVertex2f(controlPolygons[i][j].x_get(), controlPolygons[i][j].y_get());
		}
		glEnd();
	}

}

void selectDraw(int selection) {
	CPolygon p;
	switch (selection)
	{
	case 11: drawmodeEdit = 1;
		if (polygons[currentPolygon].get_points().size() > 2){
			polygons.push_back(p);
			beziers.push_back(std::vector<Point>());
			currentPolygon++;
		}
		break;
	case 12: drawmodeEdit = 0;
		break;
	}
	glutPostRedisplay();
}

void selectJoin(int selection) {
	switch (selection) {
	case 1:
		modeJoin = 1;
		joinC0(0, 1);
		break;
	case 2:
		modeJoin = 2;
		joinC1(0, 1);
		break;
	case 3:
		modeJoin = 3;
		joinC2(0, 1);
		break;
	case 0:
		exit(0);
	}
	glutPostRedisplay();
}

void selectTransf(int selection){
	switch (selection) {
	case 1:
		transfMode = TRANSLATE;
		break;
	case 2:
		transfMode = SCALE;
		break;
	case 3:
		transfMode = ROTATE;
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
		openWindow();
		break;
	case 4:
		clearAll();
		break;
	case 0:
		exit(0);
	}
	glutPostRedisplay();
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
	glutMotionFunc(MovePoint);
	glutIdleFunc(update);
	glutKeyboardFunc(keyPressed);

	int drawMenu = glutCreateMenu(selectDraw);
	glutAddMenuEntry("B�zier /Spline", 11);
	int joinMenu = glutCreateMenu(selectJoin);
	glutAddMenuEntry("C0", 1);
	glutAddMenuEntry("C1", 2);
	glutAddMenuEntry("C2", 3);
	int transfMenu = glutCreateMenu(selectTransf);
	glutAddMenuEntry("Translate", 1);
	glutAddMenuEntry("Scale", 2);
	glutAddMenuEntry("Rotate", 3);
	glutCreateMenu(select);
	glutAddSubMenu("Draw", drawMenu);
	glutAddSubMenu("Join", joinMenu);
	glutAddSubMenu("Transform", transfMenu);
	glutAddMenuEntry("Choose colors", 3);
	glutAddMenuEntry("Clear", 4);
	glutAddMenuEntry("Quitter", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	//glutKeyboardFunc(key);
	glutSpecialFunc(special);

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}
#pragma endregion


