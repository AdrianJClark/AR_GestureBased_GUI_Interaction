#include "GestureLibrary.h"
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

void rgbSub(IplImage *src1, IplImage *src2, IplImage *dst);
float rgbToNRGB(float r, float g, float b, float r2, float g2, float b2);
void minMax(IplImage *image, IplImage *difference);

//using namespace OPIRALibrary;
IplImage* OPIRALibrary::getDifferenceImage(IplImage* frame_input, MarkerTransform mt) {
		//Compute Difference Image
		IplImage *unDistort = cvCreateImage(cvGetSize(frame_input), frame_input->depth, frame_input->nChannels);
		IplImage *difference = cvCreateImage(cvGetSize(frame_input), frame_input->depth, 1);

		//Warp and smooth the image
		cvWarpPerspective(mt.marker.image, unDistort, mt.homography);
		cvSmooth(unDistort, unDistort);

		IplImage *mask = cvCreateImage(cvGetSize(frame_input), frame_input->depth, 1);
		cvCvtColor(unDistort, mask, CV_RGB2GRAY);
		IplImage *andImage = cvCreateImage(cvGetSize(frame_input), frame_input->depth, frame_input->nChannels); cvZero(andImage);

		cvCopy(frame_input, andImage, mask);
		//Do background subtraction to arrive at a single channel difference map
		minMax(andImage, difference);
		//rgbSub(unDistort, frame_input, difference);

		//Threshold the difference map and Erode and dilate to remove any noisy points
		cvThreshold(difference, difference, 32,255, CV_THRESH_BINARY_INV);
		cvDilate(difference, difference); cvErode(difference, difference);
		cvReleaseImage (&unDistort); 
		cvReleaseImage (&andImage); cvReleaseImage (&mask);
		return difference;
	}
//};

void minMax(IplImage *image, IplImage *difference) {
	for (int y=0; y<image->height; y++) {
		for (int x=0; x<image->width; x++) {
			unsigned char r,g,b;
			b = CV_IMAGE_ELEM(image,unsigned char,y,x*3);
			g = CV_IMAGE_ELEM(image,unsigned char,y,x*3+1);
			r = CV_IMAGE_ELEM(image,unsigned char,y,x*3+2);
			CV_IMAGE_ELEM(difference, unsigned char, y, x) = max(r, max(g,b)) - min(r, min(g, b));
		//	printf("%d, %d, %d - %d\n", r, g, b, max(r, max(g,b)) - min(r, min(g, b)));
		}
	}

}

float rgbToNRGB(float r, float g, float b, float r2, float g2, float b2) {
	float nr, ng, nb, nr2, ng2, nb2;
	if(r+g+b > 0){
		nr = r/(r+g+b);
		ng = g/(r+g+b);
		nb = 1.0f - nr - ng;
	} else {
		nr = ng = nb = 1.0f/3.0f;
	}
	if(r2+g2+b2>0){
		nr2 = r2/(r2+g2+b2);
		ng2 = g2/(r2+g2+b2);
		nb2 = 1.0f - nr2 - ng2;
	} else {
		nr2 = ng2 = nb2 = 1.0f/3.0f;
	}
	return ((nr-nr2)*(nr-nr2) + (ng-ng2)*(ng-ng2)+(nb-nb2)*(nb-nb2))*min(r+g+b,r2+g2+b2);
}

void rgbSub(IplImage *src1, IplImage *src2, IplImage *dst) {
	int nl = src1->height;
	int nc = src1->width;

	int i, j;
	float r, g, b, r2, g2, b2;

	IplImage *src1f = cvCreateImage(cvGetSize(src1), IPL_DEPTH_32F, 3); cvScale(src1, src1f, 1.0f/255.0f);
	IplImage *src2f = cvCreateImage(cvGetSize(src2), IPL_DEPTH_32F, 3); cvScale(src2, src2f, 1.0f/255.0f);

	IplImage *dstf = cvCreateImage(cvGetSize(dst), IPL_DEPTH_32F, 1); 

	for (i=0; i<nl; i++) {
		for (j=0; j<nc-1; j++) {
				b = CV_IMAGE_ELEM(src1f,float,i,j*3);
				g = CV_IMAGE_ELEM(src1f,float,i,j*3+1);
				r = CV_IMAGE_ELEM(src1f,float,i,j*3+2);

				b2 = CV_IMAGE_ELEM(src2f,float,i,j*3);
				g2 = CV_IMAGE_ELEM(src2f,float,i,j*3+1);
				r2 = CV_IMAGE_ELEM(src2f,float,i,j*3+2);

				CV_IMAGE_ELEM(dstf,float,i,j) = rgbToNRGB(r,g,b,r2,g2,b2) * min(r+g+b, r2+g2+b2);
		}
	}
	
	cvConvertScale(dstf, dst, 5000);

	cvReleaseImage(&src1f); cvReleaseImage(&src2f); cvReleaseImage(&dstf);
}


CvPoint OPIRALibrary::findFingerTip(IplImage* differenceImage, MarkerTransform mt) {
	CvMemStorage *ms = cvCreateMemStorage(0);
	CvSeq *contours;
	IplImage *contImage = cvCreateImage(cvSize(differenceImage->width +2, differenceImage->height+2), differenceImage->depth, differenceImage->nChannels);
	cvCopyMakeBorder(differenceImage, contImage, cvPoint(1,1), IPL_BORDER_CONSTANT, cvScalarAll(255));
	int contourCount = cvFindContours(contImage, ms, &contours);

	
	int largestSize = 0; CvSeq *largestContour;
	for (CvSeq *c=contours; c!=NULL; c=c->h_next) {
		CvRect rTmp=cvBoundingRect(c);
		int curSize = rTmp.width*rTmp.height;
		if (curSize > largestSize && (rTmp.width<contImage->width-2 || rTmp.height < contImage->height-2)) {
			largestContour = c; largestSize=curSize;
		}

	}

	if (largestSize==0) return cvPoint(-1,-1);
//	printf("%d\n", largestSize);
	cvReleaseImage(&contImage);

	IplImage *unDistort2 = cvCreateImage(cvGetSize(differenceImage), differenceImage->depth, differenceImage->nChannels); cvSetZero(unDistort2);
	cvShowImage("before", differenceImage);
	CvPoint *points = (CvPoint*)malloc(sizeof(CvPoint)*largestContour->total);
	cvCvtSeqToArray(largestContour, points);
	for (int i=0; i<largestContour->total-1; i++) {
		CvPoint *p = CV_GET_SEQ_ELEM(CvPoint, largestContour, i);
		CvPoint *p2 = CV_GET_SEQ_ELEM(CvPoint, largestContour, i+1);
		cvLine(unDistort2, *p, *p2, cvScalarAll(255),2);
	}

	IplImage *unDistort = cvCreateImage(mt.marker.size, differenceImage->depth, differenceImage->nChannels); cvSet(unDistort, cvScalarAll(255));

	CvMat *invHomo = cvCloneMat(mt.homography);
	cvInvert(mt.homography, invHomo);

	//Warp and smooth the image
	cvWarpPerspective(unDistort2, unDistort, invHomo);
	cvShowImage("unDist", unDistort);

	vector<CvPoint> pointlist;
	for (int y=0; y< unDistort->height; y++) {
		for (int x=0; x<unDistort->width; x++) {
			if (CV_IMAGE_ELEM(unDistort, unsigned char, y, x)!=0) pointlist.push_back(cvPoint(x, y));
		}
	}

	//printf("pointlist: %d\n", pointlist.size());
	CvMoments moments;
	cvMoments(unDistort, &moments);

	CvPoint2D32f center = cvPoint2D32f(moments.m10/moments.m00, moments.m01/moments.m00);
	cvCircle(unDistort, cvPoint(center.x, center.y), 3, cvScalarAll(128));

	float alpha = 0.5*atan2(2*cvGetCentralMoment(&moments,1,1), cvGetCentralMoment(&moments,2,0) - cvGetCentralMoment(&moments,0,2));
	if (alpha>0) alpha = alpha-3.1415926535897932384626433832795;
	float h = 30;
	CvPoint2D32f p1 = cvPoint2D32f(center.x - h*cos(alpha), center.y- h*sin(alpha));
	CvPoint2D32f p2 = cvPoint2D32f(center.x + h*cos(alpha), center.y + h*sin(alpha));
	cvLine(unDistort, cvPoint(p1.x, p1.y), cvPoint(p2.x, p2.y), cvScalarAll(128));

	float leftIntercept = ((p2.y-p1.y)/(p2.x-p1.x))*(0-p1.x)+p1.y;
	float rightIntercept = ((p2.y-p1.y)/(p2.x-p1.x))*(unDistort->width-p1.x)+p1.y;
	float topIntercept = (0-p1.y)/((p2.y - p1.y)/(p2.x-p1.x))+p1.x;
	float bottomIntercept = (unDistort->height-p1.y)/((p2.y - p1.y)/(p2.x-p1.x))+p1.x;

	CvPoint intercept;
	if (leftIntercept>=0 && leftIntercept<=unDistort->height) {
		cvLine(unDistort, cvPoint(0, leftIntercept), cvPoint(5, leftIntercept), cvScalarAll(128));
		intercept = cvPoint(0,leftIntercept);
	} else if (bottomIntercept>=0 && bottomIntercept<=unDistort->width) {
		cvLine(unDistort, cvPoint(bottomIntercept, unDistort->height), cvPoint(bottomIntercept, unDistort->height-5), cvScalarAll(128));
		intercept = cvPoint(bottomIntercept,unDistort->height);
	}

	if (rightIntercept>=0 && rightIntercept<=unDistort->height) {
		cvLine(unDistort, cvPoint(unDistort->width, rightIntercept), cvPoint(unDistort->width-5, rightIntercept), cvScalarAll(128));
	}
	if (topIntercept>=0 && topIntercept<=unDistort->width) {
		cvLine(unDistort, cvPoint(topIntercept,0), cvPoint(topIntercept,5), cvScalarAll(128));
	}

	CvPoint furthestPoint; int furthestDist = 0;
	for (int i=0; i<pointlist.size(); i++) {
		int dist = (pointlist.at(i).x-0)*(pointlist.at(i).x-0)+(pointlist.at(i).y-leftIntercept)*(pointlist.at(i).y-leftIntercept);
		if (dist>furthestDist) {
			furthestDist = dist; furthestPoint = pointlist.at(i);
		}
	}
	cvCircle(unDistort, furthestPoint, 3, cvScalarAll(100));

	cvShowImage("after", unDistort);
	//cvDrawContours(unDistort2


	cvReleaseImage(&unDistort2);
	cvReleaseMat(&invHomo);
	cvReleaseImage(&unDistort);

	return furthestPoint;
}
