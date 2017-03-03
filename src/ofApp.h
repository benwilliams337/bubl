#pragma once

#include "ofMain.h"
#include "chuck_fft.h"
#include <math.h>


//Constants
#define SAMPLE_RATE         44100
#define NUM_CHANNELS        2       //Stereo audio
#define BUFFER_SIZE         1024    //Number of frames per buffer
#define NUM_BUFFERS         2       //Number of buffers latency

#define BUBBLES_PER_ROW		32		//Number of bubbles generated per buffer: should be a power of 2
#define MAX_BUBBLE_RADIUS	800	//Maximum allowed radius of bubbles
#define BUBBLE_SPEED		0.004	//Each bubble will rise this fraction of the screen's height each update
#define BUBBLE_DRIFT_SPEED	0.004	//Speed at which bubbles drift
#define LANE_COOLDOWN		15		//# of frames after which we have to wait before triggering another bubble
#define BUBBLE_THRESHOLD	0.0005	//The average amplitude from a bin after which a bubble is produced (should be between 0 and 1)

#define SPACEBAR 			32

class bubl
{
	public:
		float radius;
		float targetRadius;
		float height;
		int lane;
		float drift;
		float targetDrift;
		ofColor color;

		bubl();
		~bubl();
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void exit();

		//Methods to draw various components
		void drawWaveform(float x, float y, float width, float height);
		void drawSpectrum(float x, float y, float width, float height);
		void drawBubbles(int screenWidth, int screenHeight);

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		//Method for polling audio input
		void audioIn(float* input, int bufferSize, int numChannels);

		//Sound stream object
		ofSoundStream soundStream;

		//Buffers
		vector<float> audioLeft;
		vector<float> audioRight;

		complex fftBuffer[BUFFER_SIZE / 2];
		float fftWindow[BUFFER_SIZE];

		//Bubbles
		vector<bubl*> bubbles;
		vector<int> laneCooldowns;

		bool showWave;
};



