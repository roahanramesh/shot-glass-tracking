#ifndef _CAMCTRL_WIN32_H
#define _CAMCTRL_WIN32_H

#include "cameracontrol.h" /* OSI base class */

/**
* @brief Class that handles interfacing towards libwebcam
*/
class cam_ctl_win32 : public cam_ctl {
    public:
        /**
         * @brief Destructor
         */
        ~cam_ctl_win32() {};
        /**
        * @brief Enumerate all cameras in the system and their controls
        */
        virtual int setup() = 0;

        /**
        * @brief Set brightness for the given camera
        */
        virtual int set_simple_control(int cam_id, enum simple_control scid, int value) = 0;

        /**
        * @brief Set all controls to default values
        */
        virtual int default_all_controls(int cam_id) = 0;

        /**
        * @brief Constructor
        */
        cam_ctl_win32();
    private:
};

#endif /* _CAMCTRL_WIN32_H */
