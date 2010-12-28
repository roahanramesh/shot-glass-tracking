#include "testApp.h"
#include "webcam.h"

#define SHOW_TRANSFORMED


    CDevice *cdevs;
    CControl *controls;
    CHandle dev_handle;
    CControlValue control_value;


//--------------------------------------------------------------
void testApp::setup(){



    int result = 0;
    unsigned int size = 164;
    unsigned int control_size = 0;
    unsigned int control_count = 0;
    unsigned int count = 0;

    cdevs = (CDevice *) malloc(sizeof(*cdevs) * size);

	camWidth 		= 640;	// try to grab at this size.
	camHeight 		= 480;
	ofx_color_img.allocate(camWidth, camHeight);
	c_init();

	printf("Trying to list devices\n");

	if ((result = c_enum_devices(cdevs, &size, &count)) != C_SUCCESS) {
        printf("Failed to enumerate devices on system %d %d!\n", result, size);
    }

    printf("Got %d devices\n", count);

    /* Print CDevice info */
    for (unsigned int i =0; i < count; i++) {
    printf("Device %d info: sname = %s, name = %s, driver = %s,  location = %s\n", i,
           cdevs[i].shortName, cdevs[i].name, cdevs[i].driver, cdevs[i].location);
    }

    dev_handle = c_open_device(cdevs[0].shortName);

    if (dev_handle < 0) {
        printf("Failed to open device!\n");
        return;
    }

    /* Enumerate available controls */
    c_enum_controls(dev_handle, NULL, &control_size, NULL);

    controls = (CControl *) malloc(control_size);

    if (C_SUCCESS != (result = c_enum_controls(dev_handle, controls, &control_size, &control_count))) {
        printf("Failed to enumerate controls %d\n", result);
        return;
    }

    /* Print CDevice info */
    for (unsigned int i =0; i < control_count; i++) {
    printf("Control info %d id = %d name = %s type = %d \n", i,
           controls[i].id, controls[i].name, controls[i].type);

           if (controls[i].type == 6) {
               printf("Range: min = %d max = %d step = %d\n",
                      controls[i].min.value, controls[i].max.value, controls[i].step.value);
           }
    }


    /* Beef up exposure */
    if (C_SUCCESS != c_get_control(dev_handle, CC_BRIGHTNESS, &control_value)) {
            printf("Failed to get control!\n");
    }

    printf("Control info: type %d value %d\n", control_value.type, control_value.value);

    /* Beef up exposure */
    control_value.value = 255;
    if (C_SUCCESS != (result = c_set_control(dev_handle, CC_BRIGHTNESS, &control_value))) {
            printf("Failed to set control %d!\n", result);
    }

    if (C_SUCCESS != c_get_control(dev_handle, CC_BRIGHTNESS, &control_value)) {
            printf("Failed to get control!\n");
    }

    printf("Control info: type %d value %d\n", control_value.type, control_value.value);

    //c_close_device(dev_handle);

    //c_cleanup();

    printf("Enumerated devices!\n");



	vidGrabber.setVerbose(true);
	vidGrabber.listDevices();
	vidGrabber.setDeviceID(1);
	vidGrabber.initGrabber(camWidth,camHeight);

#ifdef SHOW_TRANSFORMED
	videoInverted 	= new unsigned char[camWidth*camHeight*3];
	videoTexture.allocate(camWidth,camHeight, GL_RGB);
#endif

    printf("Setup complete!\n");

    if (cdevs) {
        free(cdevs);
    }

}

//--------------------------------------------------------------
void testApp::update(){

	ofBackground(100,100,100);

	if (C_SUCCESS != c_get_control(dev_handle, CC_BRIGHTNESS, &control_value)) {
            printf("Failed to get control!\n");
    }

    printf("Control info: type %d value %d\n", control_value.type, control_value.value);

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
