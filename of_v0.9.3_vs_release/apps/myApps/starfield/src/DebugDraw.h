#pragma once

#include "ofMain.h"

class DebugDraw : public ofBaseDraws {

public:
	using ofBaseDraws::draw;
	void draw(float x, float y) const{
		draw(x, y, getWidth(), getHeight());
	}
	void draw(float x, float y, float w, float h) const {
		for (int i = 0; i<layers.size(); i++) {
			layers[i]->draw(x, y, w, h);
		}
	}
	float getWidth() const {
		return layers.size() > 0 ? layers[0]->getWidth() : 0;
	}
	float getHeight() const {
		return layers.size() > 0 ? layers[0]->getHeight() : 0;
	}
	void addLayer(ofBaseDraws &layer) {
		layers.push_back(&layer);
	}

protected:
	vector<ofBaseDraws*> layers;
};