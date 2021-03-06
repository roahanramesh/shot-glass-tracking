#ifdef __linux__
#include "cameracontrol_linux.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <assert.h>
#include <limits.h>

#define CHOICE_STRING_PLF_50 "50 Hz"
#define CHOICE_STRING_PLF_60 "60 Hz"
#define CHOICE_STRING_PLF_DISABLED "Disabled"
#define CHOICE_STRING_EXPOSURE_AUTO "Auto Mode"
#define CHOICE_STRING_EXPOSURE_MANUAL "Manual Mode"
#define CHOICE_STRING_EXPOSURE_SHUTTER "Shutter Priority Mode"
#define CHOICE_STRING_EXPOSURE_APERTURE "Aperture Priority Mode"

/**
 * Destructor, free all resources and reset all settings.
 */
cam_ctl_linux::~cam_ctl_linux()
{
    printf("Destructor for cam_ctl_linux called!\n");

    if (controls) {
        for (int i = 0; i < num_cams; i++) {
            if (controls[i]) {
                free(controls[i]);
            }
        }
    }
    if (dev_handles) {
        for (int i = 0; i < num_cams; i++) {
            (void) default_all_controls(i);
            c_close_device(dev_handles[i]);
        }
        free(dev_handles);
    }
    if (num_controls) {
        free(num_controls);
    }
    if (controls) {
        free(controls);
    }
    if (cdevs) {
        free(cdevs);
    }

    c_cleanup();
}
//--------------------------------------------------------------
cam_ctl_linux::cam_ctl_linux() : cdevs(0), controls(0), dev_handles(0),
    num_cams(0), num_controls(0), default_cam_idx(1)
{
}

//--------------------------------------------------------------
int cam_ctl_linux::setup()
{
    unsigned int size = 0;
    unsigned int count = 0;
    int result;

    /**
     * Initiate libwebcam
     */
    c_init();

    /**
     * Enumerate all active cameras in the system
     */

    /* Begin by getting size of structure to be filled with data */
    if ((result = c_enum_devices(NULL, &size, &count)) != C_BUFFER_TOO_SMALL) {
        printf("Failed to get number of active cameras! %d\n", result);
        return -1;
    }

    cdevs = (CDevice *) malloc(size);

    if (!cdevs) {
        printf("Failed to allocate space for device structures!\n");
        return -1;
    }

    /* Now fill in the data */
    if ((result = c_enum_devices(cdevs, &size, &count)) != C_SUCCESS) {
        printf("Failed to get device info! %d\n", result);
        goto cleanup_devices;
    }

    /* Print CDevice info */
    printf("******** Device list **********\n\n");
    for (unsigned int i = 0; i < count; i++) {
        printf("Device %d info: sname = %s, name = %s, driver = %s,  location = %s\n", i,
            cdevs[i].shortName, cdevs[i].name, cdevs[i].driver, cdevs[i].location);
    }
    printf("******** End Device list **********\n\n");

    /* fill in member variable */
    num_cams = count;

    /**
     * Now enumerate all available controls for all cameras
     */

    /* Allocate controls and device handles */
    controls = (CControl **) calloc(sizeof(*controls) * num_cams, 1);

    if (!controls) {
        printf("Failed to allocate controls\n");
        goto cleanup_devices;
    }

    num_controls = (int *) calloc(sizeof(int) * num_cams, 1);

    if (!num_controls) {
        printf("Failed to allocate num controls array!\n");
        goto cleanup_controls;
    }

    dev_handles = (CHandle *) malloc(sizeof(*dev_handles));

    if (!dev_handles) {
        printf("Failed to allocate controls!\n");
        goto cleanup_num_controls;
    }

    /* Go through all devices and enumerate controls */
    for (int i = 0; i < num_cams; i++) {
        size = 0;
        count = 0;
        CControl *cur_ctrl;
        CHandle cur_dev;

        /* Open the device */
        dev_handles[i] = c_open_device(cdevs[i].shortName);

        cur_dev = dev_handles[i];

        if (cur_dev < 0) {
            printf("Failed to open device!\n");
            goto cleanup_devhandles;
        }

        if ((result = c_enum_controls(cur_dev, NULL, &size, NULL)) != C_BUFFER_TOO_SMALL) {
            printf("Failed enumerate controls for device %d, result %d\n", i, result);
            goto cleanup_devhandles;
        }

        /* Allocate space for controls info */
        controls[i] = (CControl *) malloc(size);

        if (!controls[i]) {
            printf("Failed to allocate space for control enumeration!\n");
            goto cleanup_controls_arr;
        }

        cur_ctrl = controls[i];


        if ((result = c_enum_controls(cur_dev, cur_ctrl, &size, &count)) != C_SUCCESS) {
            printf("Failed enumerate controls for device %d, result %d\n", i, result);
            goto cleanup_controls_arr;
        }

        printf("******** Controls for %s **********\n\n", cdevs[i].name);
        for (unsigned int j =0; j < count; j++) {
            printf("---------------------\n");
            printf("Control info %d id = %d name = %s type = %d \n", i,
                cur_ctrl[j].id, cur_ctrl[j].name, cur_ctrl[j].type);

            switch (cur_ctrl[j].type) {
            case CC_TYPE_DWORD:
            case CC_TYPE_WORD:
            case CC_TYPE_BYTE:
                printf("Simple control: min = %d max = %d step = %d default = %d\n",
                       cur_ctrl[j].min.value, cur_ctrl[j].max.value,
                       cur_ctrl[j].step.value, cur_ctrl[j].def.value);
                break;
            case CC_TYPE_CHOICE:
                printf("Choice list: ");
                for (unsigned int k = 0; k < cur_ctrl[j].choices.count; k++) {
                        printf("[%d %s]", cur_ctrl[j].choices.list[k].index,
                               cur_ctrl[j].choices.list[k].name);
                }
                printf(" default [%d %s]\n",  cur_ctrl[j].def.value,
                               cur_ctrl[j].choices.list[cur_ctrl[j].def.value].name);
                break;
            case CC_TYPE_BOOLEAN:
                printf("Boolean, default %d\n", cur_ctrl[j].def.value);
                break;
            case CC_TYPE_RAW:
                printf("Raw\n");
                break;
            default:
                printf("Unknown control type!\n");
                break;
            }
            printf("---------------------\n");
        }
        printf("******* End Controls for %s *********\n\n", cdevs[i].name);

        num_controls[i] = count;
    }

    return 0;

    /**
     * Error handling
     */
cleanup_controls_arr:
    for (int i = 0; i < num_cams; i++) {
        if (controls[i]) {
            free(controls[i]);
        }
    }
cleanup_devhandles:
    for (int i = 0; i < num_cams; i++) {
        c_close_device(dev_handles[i]);
    }
    free(dev_handles);
cleanup_num_controls:
    free(num_controls);
cleanup_controls:
    free(controls);
cleanup_devices:
    free(cdevs);

    c_cleanup();

    return -1;
}

//--------------------------------------------------------------
int cam_ctl_linux::set_simple_control(int cam_id, enum simple_control scid, int value)
{
    assert(sizeof(int) == 4);
    switch (scid) {
    case SC_BRIGHTNESS:
        return _set_simple_control(cam_id, CC_BRIGHTNESS, value);
    case SC_GAIN:
        return _set_simple_control(cam_id, CC_GAIN, value);
    case SC_SATURATION:
        return _set_simple_control(cam_id, CC_SATURATION, value);
    case SC_HUE:
        return _set_simple_control(cam_id, CC_SATURATION, value);
    case SC_EXPOSURE_TIME_ABSOLUTE:
        return _set_simple_control(cam_id, CC_EXPOSURE_TIME_ABSOLUTE, value);
    case SC_SHARPNESS:
        return _set_simple_control(cam_id, CC_SHARPNESS, value);
    default:
        printf("Unknown simple control\n");
    }

    return -1;
}

//--------------------------------------------------------------
CControl * cam_ctl_linux::get_control_struct(int cam_id, CControlId cid)
{
    CControl *ctrls = controls[cam_id];

    printf("Num controls %d %d\n", cam_id, num_controls[cam_id]);

    for (int i = 0; i < num_controls[cam_id]; i++) {
        printf("Got control with id %d\n", ctrls[i].id);
        if (ctrls[i].id == cid) {
            printf("Found ctrl!\n");
            return &ctrls[i];
        }
    }

    printf("Didn't find control!\n");
    return NULL;
}

//--------------------------------------------------------------
int cam_ctl_linux::_set_simple_control(int cam_idx, CControlId cid, int32_t value)
{
    CHandle dev_handle;
    CControlValue control_value;
    CControl *ctrl = NULL;
    int result;
    int32_t max = 0;
    int32_t min = 0;
    int step = 0;

    if (cam_idx < 0) {
        cam_idx = default_cam_idx;
    }

    dev_handle = dev_handles[cam_idx];

    ctrl = get_control_struct(cam_idx, cid);

    if (!ctrl) {
        printf("Failed to get control struct!\n");
        return -1;
    }

    /* Check that control type is valid */
    switch (ctrl->type) {
    case CC_TYPE_BYTE:
    case CC_TYPE_WORD:
    case CC_TYPE_DWORD:
        break;
    default:
        printf("Wrong type for simple control!\n");
        return -1;
    }

    /* Get current value */
    if (C_SUCCESS != c_get_control(dev_handle, cid, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    max = ctrl->max.value;
    min = ctrl->min.value;
    step = ctrl->step.value;

    /* Interpolate value */
    int32_t scaled_value = (int32_t) round(value * ((float) (max - min) / 100));
    scaled_value = scaled_value - (scaled_value % step);
    scaled_value += min;

    /* Check that value is within bounds */
    if (scaled_value < min || scaled_value > max) {
            printf("Value is not within bounds!\n");
            return -1;
    }

    printf("Control info: type = %d id = %d value = %d, scaled = %d\n",
           control_value.type, ctrl->id, control_value.value, scaled_value);

    /* Alter value */
    switch (control_value.type) {
    case CC_TYPE_BYTE:
        control_value.value = (int8_t) scaled_value;
        break;
    case CC_TYPE_WORD:
        control_value.value = (int16_t) scaled_value;
        break;
    case CC_TYPE_DWORD:
        control_value.value = scaled_value;
        break;
    default:
        printf("Wrong type for simple control!\n");
        return -1;
    }

    /* Now actually set the value */
    if (C_SUCCESS != (result = c_set_control(dev_handle, cid, &control_value))) {
            printf("Failed to set control %d!\n", result);
            return -1;
    }

    return 0;
}

int cam_ctl_linux::set_choice_control(int cam_id, enum choice_control ccid)
{
    CHandle dev_handle;
    CControlValue control_value;
    CControl *ctrl = NULL;
    CControlId cid = CC_POWER_LINE_FREQUENCY;
    unsigned int ctrl_idx = UINT_MAX;

    if (cam_id < 0) {
        cam_id = default_cam_idx;
    }

    if(_CHOICE_PLF_START < ccid && ccid < _CHOICE_PLF_END) {
        cid = CC_POWER_LINE_FREQUENCY;
    } else if (_CHOICE_EXPOSURE_START < ccid && ccid  < _CHOICE_EXPOSURE_START) {
        cid = CC_AUTO_EXPOSURE_MODE;
    } else {
        printf("Bad choice control id\n");
        return -1;
    }

    dev_handle = dev_handles[cam_id];

    ctrl = get_control_struct(cam_id, cid);

    if (!ctrl) {
        printf("Failed to get control struct!\n");
        return -1;
    }

    if (ctrl->type != CC_TYPE_CHOICE) {
        printf("Wrong type for control %d!\n", ctrl->type);
        return -1;
    }

    /* Look for the correct index to set */
    for (unsigned int i = 0; i < ctrl->choices.count; i++) {
        char *name = ctrl->choices.list[i].name;
        switch (ccid) {
        case CHOICE_PLF_50_HZ:
            if (strncmp(name, CHOICE_STRING_PLF_50, strlen(CHOICE_STRING_PLF_50)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        case CHOICE_PLF_60_HZ:
            if (strncmp(name, CHOICE_STRING_PLF_60, strlen(CHOICE_STRING_PLF_60)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        case CHOICE_PLF_DISABLED:
            if (strncmp(name, CHOICE_STRING_PLF_DISABLED, strlen(CHOICE_STRING_PLF_DISABLED)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        case CHOICE_EXPOSURE_AUTO:
            if (strncmp(name, CHOICE_STRING_EXPOSURE_AUTO, strlen(CHOICE_STRING_EXPOSURE_AUTO)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        case CHOICE_EXPOSURE_MANUAL:
            if (strncmp(name, CHOICE_STRING_EXPOSURE_MANUAL, strlen(CHOICE_STRING_EXPOSURE_MANUAL)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        case CHOICE_EXPOSURE_SHUTTER:
            if (strncmp(name, CHOICE_STRING_EXPOSURE_SHUTTER, strlen(CHOICE_STRING_EXPOSURE_SHUTTER)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        case CHOICE_EXPOSURE_APERTURE:
            if (strncmp(name, CHOICE_STRING_EXPOSURE_APERTURE, strlen(CHOICE_STRING_EXPOSURE_APERTURE)) == 0) {
                ctrl_idx = i;
                break;
            }
            break;
        default:
            printf("Warning! Unmapped choice %s\n", name);
            break;
        }
    }

    /* Check of we found a valid index */
    if (ctrl_idx >= ctrl->choices.count) {
        printf("Failed to find index to set!\n");
        return -1;
    }

    /**
     * Now set the control
     */
    /* Get current value */
    if (C_SUCCESS != c_get_control(dev_handle, cid, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    control_value.value = ctrl_idx;
    printf("Setting choice control index %d\n", ctrl_idx);

    /* Get current value */
    if (C_SUCCESS != c_set_control(dev_handle, cid, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    return 0;
}

int cam_ctl_linux::set_boolean_control(int cam_id, enum boolean_control bcid, bool value)
{
    CHandle dev_handle;
    CControlValue control_value;
    CControl *ctrl = NULL;
    CControlId cid = CC_POWER_LINE_FREQUENCY;

    if (cam_id < 0) {
        cam_id = default_cam_idx;
    }

    switch (bcid) {
    case BC_EXPOSURE_AUTO:
        cid = CC_AUTO_EXPOSURE_PRIORITY;
        break;
    case BC_DISABLE_VIDEOP:
        cid = CC_LOGITECH_DISABLE_PROCESSING;
        break;
    case BC_WB_TEMP_AUTO:
        cid = CC_AUTO_WHITE_BALANCE_TEMPERATURE;
        break;
    default:
        printf("Unknown boolean control %d\n", bcid);
        break;
    }

    dev_handle = dev_handles[cam_id];

    ctrl = get_control_struct(cam_id, cid);

    if (!ctrl) {
        printf("Failed to get control struct!\n");
        return -1;
    }

    if (ctrl->type != CC_TYPE_BOOLEAN) {
        printf("Wrong type for control %d!\n", ctrl->type);
        return -1;
    }

    /**
     * Now set the control
     */
    /* Get current value */
    if (C_SUCCESS != c_get_control(dev_handle, cid, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    if (value) {
        control_value.value = 1;
    } else {
        control_value.value = 0;
    }

    printf("Setting boolean control %d\n", control_value.value);

    /* Get current value */
    if (C_SUCCESS != c_set_control(dev_handle, cid, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    return 0;
}
int cam_ctl_linux::default_all_controls(int cam_id)
{
    CHandle dev_handle;
    CControl *cur_ctrl;
    CControl *ctrls;
    int result = 0;

    if (cam_id < 0) {
        cam_id = default_cam_idx;
    }

    ctrls = controls[cam_id];
    dev_handle = dev_handles[cam_id];

    for (int i = 0; i < num_controls[cam_id]; i++) {
        cur_ctrl = &ctrls[i];

        switch(cur_ctrl->type) {
        case CC_TYPE_BOOLEAN:
        case CC_TYPE_BYTE:
        case CC_TYPE_WORD:
        case CC_TYPE_DWORD:
        case CC_TYPE_CHOICE:
        case CC_TYPE_RAW:
            /* Set default value */
            if (C_SUCCESS != c_set_control(dev_handle, cur_ctrl->id, &cur_ctrl->def)) {
                result = -1;
                printf("Failed to set control %d!\n", cur_ctrl->id);
                continue;
            }
            break;
        default:
            printf("Unknown control type! %d\n", cur_ctrl->type);
            result = -1;
            break;
        }
    }

    return result;
}

int cam_ctl_linux::get_choice_control(int cam_idx, CControlId cid)
{
    CHandle dev_handle;
    CControlValue control_value;

    if (cam_idx < 0) {
        cam_idx = default_cam_idx;
    }

    dev_handle = dev_handles[cam_idx];

    /* Now actually set the value */
    if (C_SUCCESS != c_get_control(dev_handle, cid, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    printf("Got value %d\n", control_value.value);

    return control_value.value;
}

#endif /* __linux__ */
