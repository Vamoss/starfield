#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxSimpleGuiToo.h"
#include "DebugDraw.h"
#include "ROI.h"
#include "OffAxisProjectionCamera.h"
#include "ofxKinect.h"

class ofApp : public ofBaseApp {

public:

	void setup();
	void update();
	void draw();
	void exit();
	void keyPressed(int key);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);

	ofxKinect kinect;

	OffAxisProjectionCamera camera;

	ofVbo vbo;
	ofShader shader;
	vector <ofVec3f> points;
	vector <float> sizes;
	ofTexture texture;

	ofSoundPlayer backwardSound;
	ofSoundPlayer forwardSound;
	ofSoundPlayer backgroundSound;
	float soundReqSpeedsAmps;

	float offSetX;
	float offSetY;

	float cameraZ;
	float speed;

	float currPosMin;
	float currPosMax;
	float currSpeed;
	float currSpeedMax;

	float starSize;
	float galaxySize;
	int numStars;

protected:

	void initDefaults();
	void initGui();
	void updateAvgDepth();
	void updateAudio();

	bool bUseVBO;
	bool bUseFBOs;
	void initFBOs();
	ofFbo fboNew, fboComp, fboBlur;
	int trailVelocity;

	int bUserFound;
	bool bGoingForward;

	float avgDepth;
	float avgDepthSmoothed;
	float delta;
	ROI roi;
	int farCrop;

	float blobSizeMin;
	int kinectAngle, prevKinectAngle;

	DebugDraw depthDebugDraw;
	DebugDraw thresholdDebugDraw;

	ofxCvGrayscaleImage 	depthImage;
	ofxCvGrayscaleImage 	thresholdImage;

};
