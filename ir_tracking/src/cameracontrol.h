#ifndef _CAMCTRL_H
#define _CAMCTRL_H

enum simple_control {
    SC_BRIGHTNESS,
    SC_GAIN,
    SC_EXPOSURE_TIME_ABSOLUTE,
    SC_SATURATION,
    SC_HUE,
    SC_SHARPNESS,
};

/**
 * @brief Abstract class definining camera control interface
 * , used for OSI.
 */
class cam_ctl {
    public:
        /**
         * @brief Need virtual destructor for inheritance.
         */
        virtual ~cam_ctl() {};
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
    private:
};

cam_ctl *create_camera_control();

#endif /* _CAMCTRL_H */
