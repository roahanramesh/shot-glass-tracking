#ifdef _WIN32
#include "cameracontrol_win32.h"

//--------------------------------------------------------------
cam_ctl_win32::cam_ctl_win32()
{
}

//--------------------------------------------------------------
int cam_ctl_win32::setup()
{
    return 0;
}

//--------------------------------------------------------------
int cam_ctl_win32::set_brightness(int cam_id, int value)
{
    return 0;
}

#endif /* _WIN32 */
