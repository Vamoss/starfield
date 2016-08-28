#include "ofApp.h"

void ofApp::setup() {

	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofDisableArbTex();

	ofLoadImage(texture, "star.png");
	shader.load("shaders/shader");

	backwardSound.loadSound("sounds/backward.mp3");
	forwardSound.loadSound("sounds/forward.mp3");
	backgroundSound.loadSound("sounds/background.mp3");
	
	// enable depth->video image calibration
	kinect.setRegistration(true);
	kinect.init(true, false, false);
	kinect.open();
	// print the intrinsic IR sensor values
	if (kinect.isConnected()) {
		ofLogNotice() << "Kinect connected!";
	}

	depthImage.allocate(CAM_WIDTH, CAM_HEIGHT);
	thresholdImage.allocate(CAM_WIDTH, CAM_HEIGHT);

	initDefaults();

	depthDebugDraw.addLayer(depthImage);
	depthDebugDraw.addLayer(roi);

	thresholdDebugDraw.addLayer(thresholdImage);
	thresholdDebugDraw.addLayer(roi);

	initGui();

#ifndef DEBUG
	ofHideCursor();
	ofSetFullscreen(true);
#endif
	
	initFBOs();

	camera.setFarClip(100000);
}

void ofApp::initFBOs()
{
	ofFbo::Settings s;
	s.width = ofGetWidth();
	s.height = ofGetHeight();
	s.internalformat = GL_RGBA;
	fboNew.allocate(s);
	fboComp.allocate(s);

	s.width = ofGetWidth()/2;
	s.height = ofGetHeight()/2;
	fboBlur.allocate(s);

	fboNew.begin();
	ofClear(0, 0, 0, 0);
	fboNew.end();

	fboComp.begin();
	ofClear(0, 0, 0, 0);
	fboComp.end();

	fboBlur.begin();
	ofClear(0, 0, 0, 0);
	fboBlur.end();
}

void ofApp::initDefaults()
{
	avgDepth = 0;
	avgDepthSmoothed = 0;
	cameraZ = 0;
	kinectAngle = prevKinectAngle = 0;

	soundReqSpeedsAmps = 1.0f;
	bUserFound = 0;
	bGoingForward = false;
	bUseFBOs = true;
	bUseVBO = true;

	roi.width = CAM_WIDTH;
	roi.height = CAM_HEIGHT;

	trailVelocity = 100;

	offSetX = 0.53;
	offSetY = 0.5;
	starSize = 256.0;

	speed = 4000;

	numStars = 1400;
	galaxySize = 10000;

	currPosMin = currPosMin = 0;
	currSpeed = currSpeedMax = 0;
}

void ofApp::initGui()
{
	gui.addContent("depth", depthDebugDraw);
	gui.addSlider("kinectAngle", kinectAngle, -30, 30);
	gui.addSlider("x", roi.x, 0, CAM_WIDTH);
	gui.addSlider("y", roi.y, 0, CAM_HEIGHT);
	gui.addSlider("width", roi.width, 0, CAM_WIDTH);
	gui.addSlider("height", roi.height, 0, CAM_HEIGHT);
	gui.addContent("result", thresholdDebugDraw).setNewColumn(true);
	gui.addSlider("farCrop", farCrop, 0, 255);
	gui.addSlider("blob size min", blobSizeMin, 0, CAM_WIDTH*CAM_HEIGHT / 2);
	gui.addSlider("offsetX", offSetX, 0.0f, 1.0f).setNewColumn(true);
	gui.addSlider("offsetY", offSetY, 0.0f, 1.0f);
	gui.addSlider("speed", speed, 1.0f, 30000.f);
	gui.addSlider("galaxySize", galaxySize, 4000, 15000.f);
	gui.addSlider("starSize", starSize, 128.f, 1024.f);
	gui.addSlider("numStars", numStars, 500, 2000.f);
	gui.addSlider("soundReqSpeedsAmps", soundReqSpeedsAmps, 0.0f, 5.f);
	gui.addToggle("bUseFBOs", bUseFBOs);
	gui.addToggle("bUseVBO", bUseVBO);
	gui.addSlider("trailVelocity", trailVelocity, 0, 255);
	

	gui.loadFromXML();

}

void ofApp::update() {

	if (prevKinectAngle != kinectAngle) {
		kinect.setCameraTiltAngle(kinectAngle);
		prevKinectAngle = kinectAngle;
	}

	if (kinect.isConnected()) {
		kinect.update();
		if (kinect.isFrameNew()) {
			depthImage.setFromPixels(kinect.getDepthPixels(), CAM_WIDTH, CAM_HEIGHT);
			CvRect cvROI = cvRect(roi.x, roi.y, roi.width, roi.height);
			cvSetImageROI(depthImage.getCvImage(), cvROI);
			cvSetZero(thresholdImage.getCvImage());
			updateAvgDepth();
		}
	}
	else {
		avgDepth = (float)ofGetMouseY() / (float)ofGetHeight();
	}

	// smooth it's value

	delta = (avgDepth - avgDepthSmoothed)*0.15;
	avgDepthSmoothed += delta;

	currSpeed += (delta - currSpeed)*0.15;

	if (currSpeed > currSpeedMax) currSpeedMax = currSpeed;
	if (avgDepthSmoothed < currPosMin) currPosMin = avgDepthSmoothed;
	if (avgDepthSmoothed > currPosMax) currPosMax = avgDepthSmoothed;

	cameraZ += -currSpeed * (currSpeed > 0 ? 2 : 1) * speed;

	updateAudio();

	for (int i = 0; i<points.size(); i++) {
		if (points[i].z >(cameraZ + 10000)) {
			points.erase(points.begin() + i);
			sizes.erase(sizes.begin() + i);
			i--;
		}
	}

	ofPoint p;
	for (int i = points.size(); i<numStars; i++) {
		p = ofPoint(ofRandom(-galaxySize, galaxySize), ofRandom(-galaxySize, galaxySize), cameraZ - ofRandom(-10000, 70000));
		points.push_back(p);
		sizes.push_back(cameraZ - p.z);
	}

	int total = (int)points.size();
	vbo.setVertexData(&points[0], total, GL_DYNAMIC_DRAW);
}

void ofApp::updateAudio() {
	if (!bGoingForward && delta > 0.005*soundReqSpeedsAmps) {
		bGoingForward = true;
		forwardSound.play();
	}
	else if (bGoingForward && delta < -0.005*soundReqSpeedsAmps) {
		bGoingForward = false;
		backwardSound.play();
	}

	//background sound
	//fade in when detect movement
	//fade out when no movement detected
	int maxFramesCount = 500;
	int minFramesCount = -100;
	if (abs(currSpeed) > 0.0001 && bUserFound<maxFramesCount) {
		bUserFound++;
	}
	else if (bUserFound > minFramesCount) {
		bUserFound--;
	}

	if (bUserFound > 10) {
		if (!backgroundSound.isPlaying()) {
			backgroundSound.setVolume(1);
			backgroundSound.play();
		}
		else if (backgroundSound.getVolume() < 1) {
			backgroundSound.setVolume(backgroundSound.getVolume()*1.05);
		}
	}
	else if (bUserFound < 0 && backgroundSound.isPlaying()) {
		if (backgroundSound.getVolume() > 0.01) {
			backgroundSound.setVolume(backgroundSound.getVolume()*0.99);
		}
		else if (bUserFound == minFramesCount) {
			backgroundSound.stop();
			backgroundSound.setPosition(0);
		}
	}
}

void ofApp::updateAvgDepth() {

	int pos;
	int totalDepths = 0;
	int numDepths = 0;
	int width = roi.x + roi.width;
	int height = roi.y + roi.height;

	unsigned char * depthPixels = depthImage.getPixels();
	unsigned char * thresholdPixels = thresholdImage.getPixels();

	for (int i = roi.x; i<width; i++) {

		for (int j = roi.y; j<height; j++) {

			pos = j*CAM_WIDTH + i;

			if (depthPixels[pos] > farCrop){// && bInsideROI) {
				thresholdPixels[pos] = depthPixels[pos];
				totalDepths += depthPixels[pos];
				numDepths++;
			}
		}
	}
	if(numDepths>0)
		avgDepth = 1 - (float)totalDepths / (numDepths * 255);

	thresholdImage.flagImageChanged();
}

void ofApp::draw() {
	ofEnableAlphaBlending();
	ofBackground(0, 0, 0);

	if(bUseFBOs) fboNew.begin();
		ofClear(0, 0, 0, trailVelocity);

		if (bUseVBO) {
			shader.begin();
			shader.setUniform1f("starSize", starSize);
			texture.bind();

			ofSetColor(255, 255, 255, 255);

			ofEnablePointSprites();

			camera.setPosition(0, 0, cameraZ);
			camera.begin(offSetX, offSetY);

			ofEnableBlendMode(OF_BLENDMODE_ADD);

			vbo.draw(GL_POINTS, 0, (int)points.size());

			ofDisableBlendMode();

			texture.unbind();
			shader.end();

			camera.end();

			ofDisablePointSprites();
		}
		else {
			//for old computers, VBO doenst run OK :(
			//so we need to draw each texture
			ofSetColor(255, 255, 255, 255);
			camera.setPosition(0, 0, cameraZ);
			camera.begin(offSetX, offSetY);
			ofEnableBlendMode(OF_BLENDMODE_ADD);
			float scale = starSize/256.*10.;
			for (int i = 0; i < points.size(); i++) {
				ofPushMatrix();
				ofTranslate(points[i]);
				ofScale(scale, scale, scale);
				texture.draw(0, 0);
				ofPopMatrix();
			}
			ofDisableBlendMode();
			camera.end();
		}
	if (bUseFBOs) fboNew.end();

	if (bUseFBOs) {
		// composite old and new  
		fboComp.begin();
		{
			ofSetColor(255, 255, 255, 255);
			fboBlur.draw(0, 0, fboComp.getWidth(), fboComp.getHeight());

			ofSetColor(255, 255, 255, 255);
			fboNew.draw(0, 0, fboComp.getWidth(), fboComp.getHeight());
		}
		fboComp.end();

		fboBlur.begin();
		{
			ofSetColor(255, 255, 255, 255);
			fboComp.draw(0, 0, fboBlur.getWidth(), fboBlur.getHeight());
		}
		fboBlur.end();

		fboComp.draw(0, 0);
	}
	ofDisableAlphaBlending();

	if (gui.isOn()) {
		ofSetColor(0, 128, 128);
		ofRect(0, ofGetHeight() - 40, abs(currSpeed)*ofGetWidth() * 10, 10);
		ofSetColor(128, 0, 0);
		ofRect(0, ofGetHeight() - 30, avgDepth*ofGetWidth(), 10);
		ofSetColor(255, 0, 0);
		ofRect(0, ofGetHeight() - 20, avgDepthSmoothed*ofGetWidth(), 20);
		ofSetColor(255, 255, 255);
		ofRect(currPosMin*ofGetWidth(), ofGetHeight() - 20, 4, 20);
		ofRect(currPosMax*ofGetWidth(), ofGetHeight() - 20, 4, 20);
	}

	ofSetColor(255, 255, 255);
	gui.draw();
}

void ofApp::exit() {

#ifdef USE_KINECT
	kinect.setCameraTiltAngle(0); // zero the tilt on exit
	kinect.close();
#endif
}

void ofApp::mouseReleased(int x, int y, int button) {

}

void ofApp::windowResized(int w, int h)
{
	initFBOs();
}

void ofApp::keyPressed(int key) {

	switch (key) {

	case ' ':
		gui.toggleDraw();
		if (gui.isOn()) ofShowCursor();
		else ofHideCursor();
		break;
	case 'f':
		ofToggleFullscreen();
		break;
	}
}
