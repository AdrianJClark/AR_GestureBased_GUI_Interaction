#include "OpiraLibrary.h"


namespace OPIRALibrary {
	IplImage* getDifferenceImage(IplImage* currentImage, MarkerTransform mt);
	CvPoint findFingerTip(IplImage* differenceImage, MarkerTransform mt);
}