#include "OPIRALibrary.h"
#include <opencv2/highgui/highgui_c.h>

namespace OPIRALibrary {
	class RegistrationOPIRAMM: public RegistrationOPIRA {
	public:
		RegistrationOPIRAMM(RegistrationAlgorithm *registrationAlgorithm, int maxMarkerVisible): RegistrationOPIRA(registrationAlgorithm) {
			mVisible = maxMarkerVisible;
		}

		~RegistrationOPIRAMM() {}

		vector<MarkerTransform> performRegistration(IplImage* frame_input, CvMat* captureParams, CvMat* captureDistortion) 
			{
			vector<MarkerTransform> retVal;
			string windowText;
			if (previousImage==0) previousImage = cvCreateImage(cvGetSize(frame_input), frame_input->depth, frame_input->nChannels);

			vector<PointMatches> matches = regAlgorithm->findAllMatches(frame_input);

			currentImage = cvCreateImage(cvGetSize(frame_input), IPL_DEPTH_8U, 1); cvConvertImage(frame_input, currentImage);
			currentPyramid = cvCreateImage(cvGetSize(currentImage), IPL_DEPTH_32F, 1);

			for (int x=0; x<mVisible; x++) {
				int i = findMaxScore(matches);

				PointMatches *bestMatch=0;
				PointMatches pMatch = Ransac(matches.at(i));

				if (pMatch.count >=minRegistrationCount) { bestMatch = &pMatch; windowText = regAlgorithm->getName(); }
				
				//If we are able to match at least 2 points through optical flow
				PointMatches optFlow = opticalFlow(previousMatches.at(i));
				if (optFlow.count >=minOptFlowCount && optFlow.count>=pMatch.count) { bestMatch = &optFlow; windowText = "Optical Flow"; }
				previousMatches.at(i).clear();

				if (bestMatch !=0) {
					CvMat* homography = getGoodHomography(*bestMatch, captureParams, captureDistortion, markers.at(i).size);
					
					PointMatches unSift = undistortRegister(frame_input, i, homography);
					if (unSift.count>=minRegistrationCount && unSift.count > bestMatch->count) { bestMatch = &unSift; windowText = "OPIRA";}
					cvReleaseMat(&homography);

					//Copy the points into the previous array
					previousMatches.at(i).clone(*bestMatch);

					//Display the matches if we feel so inclined
					if (displayImage) displayMatches(markers.at(i).image, frame_input, *bestMatch, regAlgorithm->getName() + " OPIRA", windowText, 1.0);

					//Calculate the OGL viewpoint and projection and transformation matrices
					retVal.push_back(computeMarkerTransform(*bestMatch, i, cvGetSize(frame_input), captureParams, captureDistortion));
					unSift.clear();
				}

				//Cleanup
				pMatch.clear();
				optFlow.clear();
			}

			for (int i=0; i<matches.size(); i++) {
				matches.at(i).clear();
			}

			cvReleaseImage(&previousImage); previousImage = currentImage; currentImage = 0;
			cvReleaseImage(&previousPyramid); previousPyramid = currentPyramid; currentPyramid = 0;
			return retVal;
		}
	private:
		int mVisible;

		int findMaxScore(vector<PointMatches> pm) {
			int maxCount = 0; int maxInd = 0;
			for (int j=0; j<pm.size(); j++) {
				if (pm.at(j).count>maxCount) {
					maxCount = pm.at(j).count;
					maxInd = j;
				}
			}
			return maxInd;
		}
	};
}