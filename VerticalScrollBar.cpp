#include "VerticalScrollBar.h"
#include <windows.h>
#include "GL\glut.h"
#include <stdio.h>

const int bevel = 5;

VerticalScrollBar::VerticalScrollBar(int x, int y, int width, int height, int minValue, int maxValue, int defaultValue) {
	this->x = x; this->y = y;
	this->width = width; this->height = height;
	this->minValue = minValue; this->maxValue = maxValue;
	this->curValue = defaultValue;
	this->isLocked = false; this->nearSet = false; this->farSet = false;
	this->tabColor[0] = 1; this->tabColor[1] = 0; this->tabColor[2] = 0;
	this->tabHeight = this->width-(bevel*2);
}

void VerticalScrollBar::testEvent(int fingerX, int fingerY) {
	int tabPosX = this->x + bevel; int tabPosY = this->y + calcPosFromValue(this->curValue);
	int dist = (fingerX-tabPosX)*(fingerX-tabPosX) + (fingerY-tabPosY)*(fingerY-tabPosY);

	if (dist < 250 && !isLocked) {
		if (!nearSet) {
			QueryPerformanceCounter(&nearTime);
			nearSet=true;
			tabColor[0]=tabColor[1]=0; tabColor[2]=1;
		} else if (getElapsedTime(nearTime) > 0.5) {
			tabColor[0]=tabColor[2]=0; tabColor[1]=1;
			isLocked=true; farSet = false;
		}
	} 

	if (dist < 500 && isLocked) {
		tabColor[0]=tabColor[2]=0; tabColor[1]=1;
		curValue = calcValueFromPos(fingerY-(this->y+bevel));
		if (curValue <minValue) curValue =minValue;
		if (curValue >maxValue) curValue =maxValue;
	} 
	
	if (dist > 500 && isLocked) {
		if (!farSet) {
			QueryPerformanceCounter(&farTime);
			farSet=true;
			tabColor[0]=tabColor[1]=0; tabColor[2]=1;
		} else if (getElapsedTime(farTime) > 0.5) {
			tabColor[1]=tabColor[2]=0; tabColor[0]=1;
			isLocked=false; nearSet=false;
		}
	}
}


void VerticalScrollBar::render() {

	glPushMatrix();
//	glLoadIdentity();
	glDisable(GL_LIGHTING);
	glTranslatef(this->x, this->y,0);
	glColor3f(.5, .5, .5);
    glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(0.0, 0.0);
        glVertex2f(this->width, 0.0);
        glVertex2f(0.0, this->height);
        glVertex2f(this->width, this->height);
    glEnd();

	glTranslatef(bevel, calcPosFromValue(curValue)-this->tabHeight/2.0,-0.5);
	printf("cur:%d - %d\n", curValue, calcPosFromValue(curValue));
	glColor3f(tabColor[0],tabColor[1],tabColor[2]);
	glPushMatrix();
        glBegin(GL_TRIANGLE_STRIP);
            glVertex2f(0.0, 0.0);
            glVertex2f(this->width-(bevel*2), 0.0);
            glVertex2f(0.0, this->width-(bevel*2));
            glVertex2f(this->width-(bevel*2), this->width-(bevel*2));
        glEnd();
	glPopMatrix();

	glPopMatrix();
}

double VerticalScrollBar::getElapsedTime(LARGE_INTEGER start) {
	LARGE_INTEGER stopTime;
	LARGE_INTEGER frequency;
	QueryPerformanceCounter(&stopTime);
	QueryPerformanceFrequency( &frequency );

	LARGE_INTEGER time;
	time.QuadPart = stopTime.QuadPart - start.QuadPart;
 
	return ((double)time.QuadPart /(double)frequency.QuadPart);
}

int VerticalScrollBar::calcPosFromValue(int value) {
	float ratio = float(value-this->minValue) / float(this->maxValue-this->minValue);
	
	float posMin = bevel+((float)this->tabHeight/2.0);
	float posMax = this->height - posMin;
	
	return ratio * float(posMax-posMin)+posMin;
}

int VerticalScrollBar::calcValueFromPos(int pos) {
	float posMin = bevel+((float)this->tabHeight/2.0);
	float posMax = this->height - posMin;

	float ratio = float(pos-posMin) / float(posMax-posMin);
	
	return ratio * float(this->maxValue-this->minValue)+this->minValue;
}