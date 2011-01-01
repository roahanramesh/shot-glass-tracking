#ifndef _CAMCTRL_H
#define _CAMCTRL_H

enum simple_control {
    SC_BRIGHTNESS,
    SC_GAIN,
    SC_EXPOSURE_TIME_ABSOLUTE,
    SC_SATURATION,
    SC_HUE,
    SC_SHARPNESS,
    SC_EXPOSURE_AUTO_PRIORITY,
};

enum boolean_control {
    BC_WB_TEMP_AUTO, /* Manual WB */
    BC_EXPOSURE_AUTO, /* Manual exposure */
    BC_DISABLE_VIDEOP, /* Raw image */
};

enum choice_control {
    _CHOICE_PLF_START,
    CHOICE_PLF_50_HZ,
    CHOICE_PLF_60_HZ,
    CHOICE_PLF_DISABLED,
    _CHOICE_PLF_END,
    _CHOICE_EXPOSURE_START,
    CHOICE_EXPOSURE_SHUTTER,
    CHOICE_EXPOSURE_APERTURE,
    CHOICE_EXPOSURE_AUTO,
    CHOICE_EXPOSURE_MANUAL,
    _CHOICE_EXPOSURE_END,
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

        /**
         * @brief Set a choice based control
         */
        virtual int set_choice_control(int cam_id, enum choice_control ccid) = 0;

        /**
         * @brief Set a boolean control
         */
        virtual int set_boolean_control(int cam_id, enum boolean_control bcid, bool value) = 0;
    private:
};

cam_ctl *create_camera_control();

#endif /* _CAMCTRL_H */
