#pragma once

#include "ofMain.h"

#define CAM_WIDTH 640
#define CAM_HEIGHT 480

class ROI : public ofBaseDraws, public ofRectangle {

public:
	using ofBaseDraws::draw;
	void draw(float x, float y) const {
		draw(x, y, getWidth(), getHeight());
	}
	void draw(float x, float y, float w, float h) const {

		ofPushMatrix();
		ofTranslate(x, y);
		ofScale(w / CAM_WIDTH, h / CAM_HEIGHT);

		ofSetColor(255, 0, 0);
		ofNoFill();
		ofSetLineWidth(1);
		ofRect(*this);

		ofPopMatrix();
	}
	float getWidth() const {
		return CAM_WIDTH;
	}
	float getHeight() const {
		return CAM_HEIGHT;
	}
};
