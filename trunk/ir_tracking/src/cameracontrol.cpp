#include "cameracontrol.h"
#include "cameracontrol_linux.h"
#include "cameracontrol_win32.h"

#include <cstdlib>

cam_ctl *create_camera_control()
{
    #if defined(__linux__)
        return new cam_ctl_linux;
    #else if defined(_WIN32) 
        return new cam_ctl_win32;
	#endif /* @todo: Add more OS:es */
		return NULL;
}
