#ifndef _TEST_APP
#define _TEST_APP


#include "ofMain.h"
#include "ofxOpenCv.h"

#include "cameracontrol.h"

class testApp : public ofBaseApp{

	public:

        ~testApp();

		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

		ofVideoGrabber 		vidGrabber;
		unsigned char * 	videoInverted;
		ofTexture			videoTexture;
		int 				camWidth;
		int 				camHeight;
		ofxCvColorImage		ofx_color_img;
		cam_ctl             *uvc_cams;
};

#endif
