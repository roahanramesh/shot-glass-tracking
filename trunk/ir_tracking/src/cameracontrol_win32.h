#ifndef _CAMCTRL_WIN32_H
#define _CAMCTRL_WIN32_H

#include "cameracontrol.h" /* OSI base class */

/**
* @brief Class that handles interfacing towards libwebcam
*/
class cam_ctl_win32 : public cam_ctl {
    public:
        /**
        * @brief Enumerate all cameras in the system and their controls
        */
        int setup();

        /**
        * @brief Set brightness for the given camera
        */
        int set_brightness(int cam_id, int value);

        /**
        * @brief Constructor
        */
        cam_ctl_win32();
    private:
};

#endif /* _CAMCTRL_WIN32_H */