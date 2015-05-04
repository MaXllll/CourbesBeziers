#pragma once
#include <memory>
#include <stdio.h>
#include <iostream>
class Edge
{

public:

	Edge(void);
	Edge(float yMax, float yMin, float xMin, float slope, float yIntercept);
	~Edge(void);

	float getYMax() const;
    float getYMin() const;
	float getXMin() const;
	float getSlope() const;
	float getYIntercept() const;
    
    bool isEmpty() const;

	void setYMax(float yMax);
    void setYMin(float xMin);
	void setXMin(float xMin);
	void setSlope(float mRev);
	void setYIntercept(float yIntercept);

private:
	float yMax;
    float yMin;
	float xMin;
	float slope;
	float yIntercept;

};

std::ostream& operator<<(std::ostream& out, const Edge &e);
