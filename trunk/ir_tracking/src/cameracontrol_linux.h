#ifdef __linux__
#ifndef _CAMCTRL_LINUX_H
#define _CAMCTRL_LINUX_H

#include "webcam.h" /* Header for libwebcam */
#include "cameracontrol.h" /* OSI base class */

#include <stdint.h>

/**
* @brief Class that handles interfacing towards libwebcam
*/
class cam_ctl_linux : public cam_ctl {
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
        cam_ctl_linux();
    private:

        CControl * get_control_struct(int cam_idx, CControlId cid);
        int set_simple_control(int cam_idx, CControlId cid, int32_t value);
        int get_choice_control(int cam_idx, CControlId cid);

        /**
        * @brief Array of device descriptors
        */
        CDevice *cdevs;
        /**
        * @brief Control enumeration for all devices
        */
        CControl **controls;
        /**
        * @brief Device handles to open devices
        */
        CHandle *dev_handles;
        /**
        * @brief Numbers of active cameras in the system
        */
        int num_cams;
        /**
        * @brief Number of controls per camera
        */
        int *num_controls;
        /**
         * @brief Default camera index
         */
        int default_cam_idx;
};

#endif /* _CAMCTRL_LINUX_H */
#endif /* __linux__ */
