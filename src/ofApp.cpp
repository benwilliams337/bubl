#include "ofApp.h"

//Constructor for bubbles
bubl::bubl()
{
	radius = 0;
	targetRadius = 1;
	height = 0;
	lane = 0;
	drift = 0;
	targetDrift = 0;
}

//Bubble destructor
bubl::~bubl()
{
}


//Sets up the app
void ofApp::setup(){
	//Set up soundStream
	soundStream.listDevices();
	soundStream.setup(this, 0, NUM_CHANNELS, SAMPLE_RATE, BUFFER_SIZE, NUM_BUFFERS);

	//Init audio buffers
	audioLeft.resize(BUFFER_SIZE, 0);
	audioRight.resize(BUFFER_SIZE, 0);

	//Init FFT buffer and window
	memset(fftBuffer, 0, sizeof(complex) * BUFFER_SIZE / 2); //Init buffer to zeros
	hamming(fftWindow, BUFFER_SIZE);

	//Init graphics
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofBackground(0,0,60); //This is the water color
	ofEnableSmoothing();

	//Init vector for lane cooldowns
	laneCooldowns.resize(BUBBLES_PER_ROW, 0);
	for(int i = 0; i < BUBBLES_PER_ROW; i++)
	{
		laneCooldowns[i] = 0;
	}

	showWave = true;

	srand (time(NULL)); //Seed RNG

	//Print instructions
	cout << "Spacebar to turn water surface on/off" << endl;
}

//--------------------------------------------------------------
void ofApp::update(){

	//Copy audio data into another vector so audio data and FFT data aren't fighting over the same memory
	vector<float> tempAudio;
	tempAudio = audioLeft;

	//Update FFT
	apply_window(&tempAudio[0], fftWindow, BUFFER_SIZE); //Window the buffer
	rfft(&tempAudio[0], BUFFER_SIZE / 2, FFT_FORWARD); //Compute FFT in place
	memcpy(fftBuffer, &tempAudio[0], sizeof(complex) * BUFFER_SIZE / 2); //Copy to FFT buffer

	//Remove any bubbles that have left the screen
	int screenHeight = ofGetWindowHeight();
	for(int i = 0; i < bubbles.size(); i++)
	{
		//Check if the bubble's bottom has passed the top of the screen
		if((bubbles[i]->height - 1) * screenHeight > bubbles[i]->radius)
		{
			bubl* removedBubble = bubbles[i];
            bubbles.erase(bubbles.begin() + i);
            delete removedBubble;
		}
	}

	//Add new bubbles for any non-zero fourier points
	//Logarithmic bins adapted from http://stackoverflow.com/a/11852142
	int fftFrameCounter = 0;
	for(int i = 0; i < BUBBLES_PER_ROW; i++)
	{
		//Average amplitude across FFT bins that belong in this bubble bin
		float amplitude = 0;
		float cutoffFreq = exp(6.089 * (float)i / (float)BUBBLES_PER_ROW) * 50;
		int framesInBin = 0;
		//Compute frequency for current FFT bin, compare to cutoff frequency for current bubble bin
		while((float)fftFrameCounter * (float)SAMPLE_RATE / (float)BUFFER_SIZE <= cutoffFreq)
		{
			amplitude += cmp_abs(fftBuffer[fftFrameCounter]);
			framesInBin++;
			fftFrameCounter++;
		}
		amplitude /= framesInBin;

		//Only generate bubble if lane cooldown has passed and amplitude is high enough to pass BUBBLE_THRESHOLD
		if(laneCooldowns[i] <= 0 && amplitude > BUBBLE_THRESHOLD)
		{
			bubl* newBubble = new bubl();
			newBubble->radius = 0;
			newBubble->targetRadius = amplitude * MAX_BUBBLE_RADIUS;
			newBubble->lane = i;
			newBubble->color = ofColor(ofRandom(100, 255), ofRandom(100, 255), ofRandom(100, 255)); //Random pastels for bubble colors

			bubbles.push_back(newBubble);
			laneCooldowns[i] = LANE_COOLDOWN; //Reset cooldown for lane
		}
		laneCooldowns[i]--; //Apply cooldown
	}

	//Progress bubble animations
	for(int i = 0; i < bubbles.size(); i++)
	{
		//Bubbles float up
		bubbles[i]->height += BUBBLE_SPEED;

		//Bubbles expand from bottom
		if(bubbles[i]->radius < bubbles[i]->targetRadius)
		{
            bubbles[i]->radius = (bubbles[i]->height / 0.15) * bubbles[i]->targetRadius;
		}

		//Bubbles drift left and right
		if(abs(bubbles[i]->targetDrift - bubbles[i]->drift) < 0.001) //If bubble is close enough to its target
		{
			//Randomly assign a new drift target
            bubbles[i]->targetDrift = ((float)((rand() % 20000) - 10000)) / 10000.0;
		}
		//Drift towards target
		bubbles[i]->drift += BUBBLE_DRIFT_SPEED * (bubbles[i]->drift < bubbles[i]->targetDrift ? 1.0 : -1.0);
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	int windowHeight = ofGetWindowHeight();
	int windowWidth = ofGetWindowWidth();

	float waveform_width = windowWidth * 0.9;
	float waveform_height = windowHeight * 0.5;

	drawBubbles(windowWidth, windowHeight);
	//Draw waveform second so it overlaps bubbles
	if(showWave)
		drawWaveform(0, windowHeight * 0.2, windowWidth, windowHeight * 0.2);

	//Code enclosed to draw spectrum as a line, but unused in production
	//drawSpectrum(windowWidth * 0.05, windowHeight * 0.9, windowWidth * 0.9, windowHeight * 0.5);

}

void ofApp::audioIn(float* input, int bufferSize, int numChannels) {
    //Just read audio into our designated buffers.
    //Samples are interleaved.
	for(int i = 0; i < bufferSize; i++)
	{
		audioLeft[i] = input[i * 2];
		audioRight[i] = input[i * 2 + 1];
	}
}

void ofApp::drawWaveform(float x, float y, float width, float height)
{
	ofPushStyle();
		ofPushMatrix();

			ofTranslate(x, y, 0);
			ofSetColor(135, 206, 235); //Color of sky
			ofSetLineWidth(3);

			//Draw waveform
			ofBeginShape();
				//Note: making the assumption that audioLeft and audioRight are the same size...
                for(int i = 0; i < audioLeft.size(); i++)
				{
					//Take average of channels
                    float amplitude = (audioLeft[i] + audioRight[i]) / 2.0;
                    ofVertex(i * width / audioLeft.size(), height * audioLeft[i], 0);
				}
				//Draw additional vertices to close shape off sides of screen
				ofVertex(width, -y);
				ofVertex(x, -y);
				ofVertex(x, 0);
			ofEndShape(true); //Shape is closed: fill shape

		ofPopMatrix();
	ofPopStyle();
}

//Unused method: draws FFT spectrum as a line
void ofApp::drawSpectrum(float x, float y, float width, float height)
{
	ofPushStyle();
		ofPushMatrix();
			//Start on left side of screen, vertically centered, with a 5% padding on the left
			ofTranslate(x, y, 0);
			ofSetColor(200, 200, 200);
			ofSetLineWidth(3);
			ofNoFill();

			//Draw waveform
			ofBeginShape();
				//Note: making the assumption that audioLeft and audioRight are the same size...
                for(int i = 0; i < BUFFER_SIZE / 2; i++)
				{
                    //FFT
                    float xPercent = 2.0 * i / ((float)BUFFER_SIZE);
                    float amplitude = -10.0 * cmp_abs(fftBuffer[i]);
                    ofVertex(xPercent * width, amplitude * height, 0);
				}
			ofEndShape(false); //Not a connected polygon

		ofPopMatrix();
	ofPopStyle();
}

void ofApp::drawBubbles(int screenWidth, int screenHeight)
{
	ofPushStyle();
		ofPushMatrix();
            ofTranslate(screenWidth * 0.05, 0, 0); //5% buffer room on left/right of screen for bubbles

			float laneWidth = 0.9 * screenWidth / BUBBLES_PER_ROW;
			for(int i = 0; i < bubbles.size(); i++)
			{
				//Determine bubble's coordinates
				float bubble_x = (bubbles[i]->lane * screenWidth * 0.9) / (BUBBLES_PER_ROW - 1);
				bubble_x += bubbles[i]->drift * laneWidth / 2;
				float bubble_y = screenHeight - (bubbles[i]->height * screenHeight);

				//Draw bubble
				ofSetColor(bubbles[i]->color);
				ofCircle(bubble_x, bubble_y, bubbles[i]->radius);
			}
		ofPopMatrix();
	ofPopStyle();
}

void ofApp::exit()
{
	//Clean up bubbles vector
    for(int i = 0; i < bubbles.size(); i++)
	{
		delete bubbles[i];
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if(key == SPACEBAR)
		showWave = !showWave;
}

//Unused openFrameworks methods below here

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
