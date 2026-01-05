#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "stdlib.h"
#include "GL\glut.h"

#include "OpiraLibrary.h"
#include "RegistrationOPIRAMM.h"
#include "CaptureLibrary.h"
#include "RegistrationAlgorithms\OCVSurf.h"
#include "GestureLibrary.h"
#include <windows.h>

#include "VerticalScrollBar.h"
#include "HorizontalScrollBar.h"

using namespace OPIRALibrary;
void initGLTextures();
void draw(IplImage* frame_input, std::vector<MarkerTransform> mt, string windowName);

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

GLuint GLTextureID;

bool running = true;

CvPoint scrollBarPos;
CvPoint scrollTabPos;

VerticalScrollBar *vs;
HorizontalScrollBar *hs;

void main(int argc, char **argv) {
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
	//_CrtSetBreakAlloc(19879);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(640, 480);
	glutCreateWindow("SimpleTest");
	
	//int x, int y, int width, int height, int minValue=0, int maxValue=100, int defaultValue=50
	IplImage *tmpIm = cvLoadImage("media/3.png");
	vs = new VerticalScrollBar(tmpIm->width-40,0,40,tmpIm->height);
	hs = new HorizontalScrollBar(0,(tmpIm->height)/2.0-40,tmpIm->width,40);
	cvReleaseImage(&tmpIm);

	//Create a new Registration Algorithm using Opira and the SURF algorithm
	Registration *r = new RegistrationOPIRA(new OCVSurf());
	vector<string> files;
	for (int i=3; i<4; i++) {
		char filename[50];
		sprintf(filename, "media/%d.png", i);
		r->addMarker(filename);
	}
	r->displayImage = false;

	//Initialise our Camera
	//Capture* camera = new Video("c:/colbook.avi", "camera.yml");
	Capture* camera = new Camera(0,"camera.yml");
	initGLTextures();

	while (running) {
		//Grab a frame 
		IplImage *new_frame = camera->getFrame();

		if (new_frame==0) break;

		vector<MarkerTransform> mt = r->performRegistration(new_frame, camera->getParameters(), camera->getDistortion());

		draw(new_frame, mt, "Orig");

		//Check for the escape key and give the computer some processing time
		switch (cvWaitKey(1)) {
			case 27:
				running = false; break;
		}
	
		//Clean up
		cvReleaseImage(&new_frame);
		for (unsigned int i=0; i<mt.size(); i++) { mt.at(i).clear(); } mt.clear();
	};
	delete camera;
	delete r;
}


void draw(IplImage* frame_input, std::vector<MarkerTransform> mt, string windowName)
{
	//Clear the depth buffer 
	glClearDepth( 1.0 ); glClear(GL_DEPTH_BUFFER_BIT); glDepthFunc(GL_LEQUAL);

	//Set the viewport to the window size
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    //Set the Projection Matrix to an ortho slightly larger than the window
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glOrtho(-0.5, WINDOW_WIDTH-0.5, WINDOW_HEIGHT-0.5, -0.5, 1.0, -1.0);
    //Set the modelview to the identity
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();

	//Turn off Light and enable a texture
	glDisable(GL_LIGHTING);	glEnable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, GLTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, frame_input->width, frame_input->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, frame_input->imageData);
	
	//Draw the background
	glPushMatrix();
        glColor3f(255, 255, 255);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(0.0, 0.0);	glVertex2f(0.0, 0.0);
            glTexCoord2f(1.0, 0.0);	glVertex2f(WINDOW_WIDTH, 0.0);
            glTexCoord2f(0.0, 1.0);	glVertex2f(0.0, WINDOW_HEIGHT);
            glTexCoord2f(1.0, 1.0);	glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
        glEnd();
	glPopMatrix();

	//Turn off Texturing
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_LIGHTING);	glDisable(GL_TEXTURE_2D); glEnable(GL_DEPTH_TEST);

	//Loop through all the markers found
	for (unsigned int i =0; i<mt.size(); i++) {
		if (mt.at(i).score>8) {
			double* projectionMat = mt.at(i).projMat;
			double* translationMat = mt.at(i).transMat;
			CvSize markerSize = mt.at(i).marker.size;

			//Set the Viewport Matrix
			glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

			//Load the Projection Matrix
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixd( projectionMat );

			//Load the camera modelview matrix 
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixd( translationMat );

			IplImage *differenceImage = getDifferenceImage(frame_input, mt.at(i));
			CvPoint fingerTip = findFingerTip(differenceImage, mt.at(i));

			cvShowImage("diff", differenceImage);
			//Create the stencil buffer
			{
				//Set up stencil buffering and create images		
				glEnable(GL_STENCIL_TEST); glStencilFunc(GL_ALWAYS, 1, 1); glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				cvFlip(differenceImage, differenceImage);
				if (differenceImage->width != WINDOW_WIDTH || differenceImage->height != WINDOW_HEIGHT) {
					IplImage *tmpIm = cvCreateImage(cvSize(WINDOW_WIDTH, WINDOW_HEIGHT), differenceImage->depth, differenceImage->nChannels);
					cvResize(differenceImage, tmpIm); 
					glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, tmpIm->imageData);
					cvReleaseImage(&tmpIm);
				} else {
					glDrawPixels(WINDOW_WIDTH, WINDOW_HEIGHT, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, differenceImage->imageData);
				}
				cvFlip(differenceImage, differenceImage);
				glColorMask(1,1,1,1); glStencilFunc(GL_EQUAL,1,1); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); glEnable(GL_STENCIL_TEST);
			}

			vs->testEvent(fingerTip.x, fingerTip.y);
			vs->render();

			hs->testEvent(fingerTip.x, fingerTip.y);
			hs->render();

			cvReleaseImage(&differenceImage);
			glDisable(GL_STENCIL_TEST);

		}

	}
	
	//Copy the OpenGL Graphics context into an IPLImage
	IplImage* outImage = cvCreateImage(cvSize(WINDOW_WIDTH,WINDOW_HEIGHT), IPL_DEPTH_8U, 3);
	glReadPixels(0,0,WINDOW_WIDTH,WINDOW_HEIGHT,GL_RGB, GL_UNSIGNED_BYTE, outImage->imageData);
	cvCvtColor( outImage, outImage, CV_BGR2RGB );
	cvFlip(outImage, outImage);

	cvShowImage(windowName.c_str(), outImage);
	cvReleaseImage(&outImage);
}


void initGLTextures() {
	//Set up Materials 
	GLfloat mat_specular[] = { 0.4, 0.4, 0.4, 1.0 };
	GLfloat mat_diffuse[] = { .8,.8,.8, 1.0 };
	GLfloat mat_ambient[] = { .4,.4,.4, 1.0 };

	glShadeModel(GL_SMOOTH);//smooth shading
	glMatrixMode(GL_MODELVIEW);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, 100.0);//define the material
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);//enable the material
	glEnable(GL_NORMALIZE);

	//Set up Lights
	GLfloat light0_ambient[] = {0.1, 0.1, 0.1, 0.0};
	float light0_diffuse[] = { 0.8f, 0.8f, 0.8, 1.0f };
	glLightfv (GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glEnable(GL_LIGHT0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable (GL_LINE_SMOOTH);	

	//Initialise the OpenCV Image for GLRendering
	glGenTextures(1, &GLTextureID); 	// Create a Texture object
    glBindTexture(GL_TEXTURE_2D, GLTextureID);  //Use this texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	
	glBindTexture(GL_TEXTURE_2D, 0);

}



