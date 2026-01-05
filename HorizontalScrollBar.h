#ifndef VERTICAL_SCROLL_BAR
#define VERTICAL_SCROLL_BAR
#include <windows.h>

class HorizontalScrollBar {
public:
	HorizontalScrollBar(int x, int y, int width, int height, int minValue=0, int maxValue=100, int defaultValue=50);
	void render();
	void testEvent(int fingerX, int fingerY);
private:
	int x, y;
	int width, height;
	int minValue, maxValue;
	int curValue;
	int tabWidth;
	int tabColor[3];
	bool isLocked;
	int calcPosFromValue(int value);
	int calcValueFromPos(int pos);
	double getElapsedTime(LARGE_INTEGER start);

	LARGE_INTEGER nearTime; bool nearSet;
	LARGE_INTEGER farTime;	bool farSet;
};

#endif