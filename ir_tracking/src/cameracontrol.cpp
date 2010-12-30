#include "cameracontrol.h"
#include "cameracontrol_linux.h"

cam_ctl *create_camera_control()
{
    #ifdef __linux__
        return new cam_ctl_linux;
    #else /* @todo: Add more OS:es */
        return NULL;
    #endif
}
