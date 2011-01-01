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
        int setup();

        /**
        * @brief Set brightness for the given camera
        */
        int set_simple_control(int cam_id, enum simple_control scid, int value);

        /**
        * @brief Set all controls to default values
        */
        int default_all_controls(int cam_id);

        /**
         * @brief Set a choice based control
         */
        int set_choice_control(int cam_id, enum choice_control ccid);

        /**
         * @brief Set a boolean control
         */
        int set_boolean_control(int cam_id, enum boolean_control bcid, bool value);

        /**
        * @brief Constructor
        */
        cam_ctl_win32();
    private:
};

#endif /* _CAMCTRL_WIN32_H */
