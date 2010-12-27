/**
* Camera control through direct show and proprietary logitech drivers
*/

#include "stdafx.h"
#include <dshow.h>
#include <Ks.h>				// Required by KsMedia.h
#include <KsMedia.h>		// For KSPROPERTY_CAMERACONTROL_FLAGS_*

/**
* Check the system for dshow compatible cameras
*/
int enum_devices(	);

void inc_exposure(int cam_no);
void dec_exposure(int cam_no);
void auto_exposure(int cam_no);

void lock_framerate(int cam_no);
void unlock_framerate(int cam_no);