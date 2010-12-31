#include "testApp.h"
#include "cameracontrol_linux.h"

#define SHOW_TRANSFORMED

//--------------------------------------------------------------
void testApp::setup(){
    int result = -1;
    camWidth 		= 640;	// try to grab at this size.
	camHeight 		= 480;
	ofx_color_img.allocate(camWidth, camHeight);

	uvc_cams = create_camera_control();

	if (!uvc_cams) {
        printf("Failed to create camera control!\n");
        return;
	}

	result = uvc_cams->setup();

	if (result < 0) {
	    printf("Failed to setup cameras!\n");
	    return;
    }

    uvc_cams->set_simple_control(-1, SC_BRIGHTNESS, 50);
    uvc_cams->set_simple_control(-1, SC_SHARPNESS, 100);
    //uvc_cams->default_all_controls(-1);

	vidGrabber.setVerbose(true);
	vidGrabber.listDevices();
#if defined(__linux__)
	vidGrabber.setDeviceID(1);
#else
	vidGrabber.setDeviceID(0);
#endif /* Usually running linux on laptop with 2 cams */

	vidGrabber.initGrabber(camWidth,camHeight);

#ifdef SHOW_TRANSFORMED
	videoInverted 	= new unsigned char[camWidth*camHeight*3];
	videoTexture.allocate(camWidth,camHeight, GL_RGB);
#endif

    printf("Setup complete!\n");
}

//--------------------------------------------------------------
void testApp::update(){

	ofBackground(100,100,100);

	vidGrabber.grabFrame();
#ifdef SHOW_TRANSFORMED
    if (vidGrabber.isFrameNew()) {
            ofx_color_img.setFromPixels(vidGrabber.getPixels(), camWidth,camHeight);
            ofx_color_img.blurGaussian(11);
    }
#if 0
	if (vidGrabber.isFrameNew()){
		int totalPixels = camWidth*camHeight*3;
		unsigned char * pixels = vidGrabber.getPixels();
		for (int i = 0; i < totalPixels; i++){
			videoInverted[i] = 255 - pixels[i];
		}
		videoTexture.loadData(videoInverted, camWidth,camHeight, GL_RGB);
	}
#endif
#endif

}

//--------------------------------------------------------------
void testApp::draw(){
	ofSetColor(0xffffff);
	vidGrabber.draw(20,20);
#ifdef SHOW_TRANSFORMED
    ofx_color_img.draw(20,20+camHeight);
    #if 0
	videoTexture.draw(20,20+camHeight,camWidth,camHeight);
	#endif
#endif
}

testApp::~testApp()
{
    printf("Destructor for testApp called!\n");
    delete uvc_cams;
    if (videoInverted) {
        free(videoInverted);
    }
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){

	// in fullscreen mode, on a pc at least, the
	// first time video settings the come up
	// they come up *under* the fullscreen window
	// use alt-tab to navigate to the settings
	// window. we are working on a fix for this...

	if (key == 's' || key == 'S'){
		vidGrabber.videoSettings();
	}


}


//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}
