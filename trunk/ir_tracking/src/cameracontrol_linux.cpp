#ifdef __linux__
#include "cameracontrol_linux.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <assert.h>

//--------------------------------------------------------------
cam_ctl_linux::cam_ctl_linux() : cdevs(0), controls(0), dev_handles(0),
    num_cams(0), num_controls(0), default_cam_idx(0)
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
            printf("Control info %d id = %d name = %s type = %d \n", i,
                cur_ctrl[j].id, cur_ctrl[j].name, cur_ctrl[j].type);

           if (cur_ctrl[j].type == CC_TYPE_DWORD) {
               printf("Range: min = %d max = %d step = %d\n",
                      cur_ctrl[j].min.value, cur_ctrl[j].max.value, cur_ctrl[j].step.value);
           }
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




    #if 0
    /* Enumerate available controls */

    /* Beef up exposure */
    if (C_SUCCESS != c_get_control(dev_handle, CC_BRIGHTNESS, &control_value)) {
            printf("Failed to get control!\n");
    }

    printf("Control info: type %d value %d\n", control_value.type, control_value.value);

    /* Beef up exposure */
    control_value.value = 255;
    if (C_SUCCESS != (result = c_set_control(dev_handle, CC_BRIGHTNESS, &control_value))) {
            printf("Failed to set control %d!\n", result);
    }

    if (C_SUCCESS != c_get_control(dev_handle, CC_BRIGHTNESS, &control_value)) {
            printf("Failed to get control!\n");
    }

    printf("Control info: type %d value %d\n", control_value.type, control_value.value);

    //c_close_device(dev_handle);

    //c_cleanup();

    printf("Enumerated devices!\n");
    #endif

    return 0;
}

//--------------------------------------------------------------
int cam_ctl_linux::set_brightness(int cam_id, int value)
{
    assert(sizeof(int) == 4);
    return set_simple_control(cam_id, CC_BRIGHTNESS, value);
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
int cam_ctl_linux::set_simple_control(int cam_idx, CControlId cid, int32_t value)
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

    ctrl = get_control_struct(cam_idx, CC_BRIGHTNESS);

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

    /* Now actually set the value */
    if (C_SUCCESS != c_get_control(dev_handle, CC_BRIGHTNESS, &control_value)) {
            printf("Failed to get control!\n");
            return -1;
    }

    printf("Control info: type %d value %d, scaled %d\n", control_value.type, control_value.value, scaled_value);

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

    if (C_SUCCESS != (result = c_set_control(dev_handle, CC_BRIGHTNESS, &control_value))) {
            printf("Failed to set control %d!\n", result);
            return -1;
    }

    return 0;
}

#endif /* __linux__ */
