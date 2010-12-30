#ifndef _CAMCTRL_H
#define _CAMCTRL_H

#include <stdint.h>

/**
 * @brief Abstract class definining camera control interface
 * , used for OSI.
 */
class cam_ctl {
    public:
        /**
        * @brief Enumerate all cameras in the system and their controls
        */
        virtual int setup() = 0;

        /**
        * @brief Set brightness for the given camera
        */
        virtual int set_brightness(int cam_id, int32_t value) = 0;
    private:
};

cam_ctl *create_camera_control();

#endif /* _CAMCTRL_H */
