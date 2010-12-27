#include "stdafx.h"
#include "camcontrol.h"
#include "LVUVCPublic.h"

struct ControlInfo {
	long min;
	long max;
	long step;
	long def;
	long flags;
};

static IBaseFilter *pFilter;
static IAMCameraControl *pCameraControl[2];
static IAMVideoProcAmp *pCameraProcAmp[2];
static IKsPropertySet *pIKsPs[2];
static int num_found_cams = 0;

void inc_exposure(int cam_no) 
{
	HRESULT hr = 0;
	ControlInfo panInfo = { 0 };
	ControlInfo tiltInfo = { 0 };
	ControlInfo zoomInfo = { 0 };
	long value = 0, flags = 0;

	// Retrieve info on exposure time
	hr = (pCameraControl[cam_no])->Get(KSPROPERTY_CAMERACONTROL_EXPOSURE, &value, &flags);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to get exposure property\n");
		return;
	}

	printf("	Exposure: %d, Flags 0x%08X\n", value, flags);

	//Try to increase the exposure time
	hr = (pCameraControl[cam_no])->Set(CameraControl_Exposure, value + 1, CameraControl_Flags_Manual);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to get exposure property\n");
		return;
	}

	//Retrieve exposure time again
	// Retrieve info on exposure time
	hr = (pCameraControl[cam_no])->Get(CameraControl_Exposure, &value, &flags);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to get exposure property\n");
		return;
	}

	printf("	Exposure: %d, Flags 0x%08X\n", value, flags);
}
void dec_exposure(int cam_no) 
{
	HRESULT hr = 0;
	ControlInfo panInfo = { 0 };
	ControlInfo tiltInfo = { 0 };
	ControlInfo zoomInfo = { 0 };
	long value = 0, flags = 0;

	// Retrieve info on exposure time
	hr = (pCameraControl[cam_no])->Get(KSPROPERTY_CAMERACONTROL_EXPOSURE, &value, &flags);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to get exposure property\n");
		return;
	}
	printf("	Exposure: %d, Flags 0x%08X\n", value, flags);

	//Try to decrease the exposure time
	hr = (pCameraControl[cam_no])->Set(CameraControl_Exposure, value - 1, CameraControl_Flags_Manual);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to get exposure property\n");
		return;
	}

	//Retrieve exposure time again
	// Retrieve info on exposure time
	hr = (pCameraControl[cam_no])->Get(CameraControl_Exposure, &value, &flags);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to get exposure property\n");
		return;
	}

	printf("	Exposure: %d, Flags 0x%08X\n", value, flags);
}

void auto_exposure(int cam_no)
{
	HRESULT hr = 0;
	ControlInfo panInfo = { 0 };
	ControlInfo tiltInfo = { 0 };
	ControlInfo zoomInfo = { 0 };
	long value = 0, flags = 0;

	hr = (pCameraControl[cam_no])->Set(CameraControl_Exposure, 0, CameraControl_Flags_Auto);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to set automatic exposure\n");
		return;
	}
}

void lock_framerate(int cam_no)
{
	HRESULT hr = 0;
	ControlInfo panInfo = { 0 };
	ControlInfo tiltInfo = { 0 };
	ControlInfo zoomInfo = { 0 };
	long value = 0, flags = 0;

	hr = (pCameraControl[cam_no])->Set(KSPROPERTY_CAMERACONTROL_AUTO_EXPOSURE_PRIORITY, 
		0 /* frame rate must remain constant */, CameraControl_Flags_Manual);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to lock framerate\n");
		return;
	}
}

void unlock_framerate(int cam_no)
{
	HRESULT hr = 0;
	ControlInfo panInfo = { 0 };
	ControlInfo tiltInfo = { 0 };
	ControlInfo zoomInfo = { 0 };
	long value = 0, flags = 0;

	hr = (pCameraControl[cam_no])->Set(KSPROPERTY_CAMERACONTROL_AUTO_EXPOSURE_PRIORITY, 
		1 /* frame rate doesn't have to be constant */, CameraControl_Flags_Manual);

	if (hr != S_OK) 
	{
		fprintf(stderr, "Failed to unlock framerate\n");
		return;
	}
}
/*
 * Test a camera's pan/tilt properties
 *
 * See also:
 *
 * IAMCameraControl Interface
 *     http://msdn2.microsoft.com/en-us/library/ms783833.aspx
 * PROPSETID_VIDCAP_CAMERACONTROL
 *     http://msdn2.microsoft.com/en-us/library/aa510754.aspx
 */
HRESULT test_pan_tilt(IBaseFilter *pBaseFilter)
{
	HRESULT hr = 0;
	ControlInfo panInfo = { 0 };
	ControlInfo tiltInfo = { 0 };
	ControlInfo zoomInfo = { 0 };
	long value = 0, flags = 0;

	printf("    Reading pan/tilt property information ...\n");
	
	// Get a pointer to the IAMCameraControl interface used to control the camera
	hr = pBaseFilter->QueryInterface(IID_IAMCameraControl, (void **)&pCameraControl[num_found_cams]);
	if(hr != S_OK)
	{
		fprintf(stderr, "ERROR: Unable to access IAMCameraControl interface.\n");
		return hr;
	}

	printf("Got new Cam Control interface 0x%08X\n", (unsigned) pCameraControl[num_found_cams]);
	
	// Get a pointer to the IAMVideoProcAmp interface used to control the camera
	hr = pBaseFilter->QueryInterface(IID_IAMVideoProcAmp, (void **)&pCameraProcAmp[num_found_cams]);
	if(hr != S_OK)
	{
		fprintf(stderr, "ERROR: Unable to access IAMVideoProcAmp interface.\n");
		return hr;
	}

	// Get a pointer to the IAMVideoProcAmp interface used to control the camera
	hr = pBaseFilter->QueryInterface(IID_IKsPropertySet, (void **)&pIKsPs[num_found_cams]);
	if(hr != S_OK)
	{
		fprintf(stderr, "ERROR: Unable to access IKsPropertySet interface.\n");
		return hr;
	}

	/*KSPROPERTY_LP1_LED_S LED_Info = { 0, LVUVC_LP1_LED_MODE_AUTO, 0 };

	// Try to turn of the LED
	hr = pIKsPs->Set(PROPSETID_LOGITECH_PUBLIC1,
		KSPROPERTY_LP1_LED,
		NULL,
		0,
		&LED_Info,
		sizeof(KSPROPERTY_LP1_LED_S));

	if(hr != S_OK)
	{
		fprintf(stderr, "ERROR: Failed to make LED blinky\n");
		return hr;
	}*/




	

	return S_OK;
}


/*
 * Do something with the filter. In this sample we just test the pan/tilt properties.
 */
void process_filter(IBaseFilter *pBaseFilter)
{
	test_pan_tilt(pBaseFilter);
}

int enum_devices()
{
	HRESULT hr;

	printf("Enumerating video input devices ...\n");

	// Create the System Device Enumerator.
	ICreateDevEnum *pSysDevEnum = NULL;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if(FAILED(hr))
	{
		fprintf(stderr, "ERROR: Unable to create system device enumerator.\n");
		return hr;
	}

	// Obtain a class enumerator for the video input device category.
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

	if(hr == S_OK) 
	{
		// Enumerate the monikers.
		IMoniker *pMoniker = NULL;
		ULONG cFetched;
		while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
		{
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
				(void **)&pPropBag);
			if(SUCCEEDED(hr))
			{
				// To retrieve the filter's friendly name, do the following:
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr))
				{
					// Display the name in your UI somehow.
					wprintf(L"  Found device: %s\n", varName.bstrVal);
				}

				if (wcscmp(varName.bstrVal, L"Logitech Webcam 200") == 0)
				{
					hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
					(void**)&pFilter);
					printf("Found a Logitech cam, num found %d\n", num_found_cams);
					process_filter(pFilter);
					num_found_cams++;
				}
				VariantClear(&varName);

				// To create an instance of the filter, do the following:
				//IBaseFilter *pFilter;
				

				//Remember to release pFilter later.
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCat->Release();
	}
	pSysDevEnum->Release();

	return 0;
}