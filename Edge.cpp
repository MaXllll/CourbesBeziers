#include "stdafx.h"
#include "Edge.h"
#include "Node.h"

typedef Edge* EdgePtr;

Edge::Edge(void)
{
	this->yMin = 0;
	this->yMax = 0;
	this->xMin = 0;
	this->slope = 0;
	this->yIntercept = 0;
}


Edge::Edge(float yMax, float yMin, float xMin, float slope, float yIntercept)
{
	this->yMax = yMax;
	this->yMin = yMin;
	this->xMin = xMin;
	this->slope = slope;
	this->yIntercept = yIntercept;
}

Edge::~Edge(void)
{
}

std::ostream& operator<<(std::ostream& out, const Edge &e)
{
	if(e.isEmpty())
		out << "empty";
	else
	{
		out << "[ yMax : " << e.getYMax() << " yMin : " << e.getYMin() << " xMin : " << e.getXMin() << " slope : " << e.getSlope() << " yIntercept : " << e.getYIntercept() <<  "] , ";
	}
	return out;
}

bool Edge::isEmpty() const
{
	return (this->yMax == 0.0f && this->yMin == 0.0f && this->xMin == 0.0f && this->slope == 0.0f && this->yIntercept == 0.0f);
}

float Edge::getYMax() const
{
	return this->yMax;
}

float Edge::getYMin() const
{
	return this->yMin;
}

float Edge::getXMin() const
{
	return this->xMin;
}

float Edge::getSlope() const
{
	return this->slope;
}

float Edge::getYIntercept() const
{
	return this->yIntercept;
}

void Edge::setYMax(float yMax)
{
	this->yMax = yMax;
}

void Edge::setYMin(float yMin)
{
	this->yMin = yMin;
}

void Edge::setXMin(float xMin)
{
	this->xMin = xMin;
}

void Edge::setSlope(float slope)
{
	this->slope = slope;
}

void Edge::setYIntercept(float yIntercept)
{
	this->yIntercept = yIntercept;
}