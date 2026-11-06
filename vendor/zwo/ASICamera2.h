/*******************************************************************************
 * ASICamera2.h
 * Precision C-API Declarations for ZWO ASI Camera SDK
 * Matches exports in ASICamera2.dll
 ******************************************************************************/

#ifndef ASICAMERA2_H
#define ASICAMERA2_H

#ifdef _WIN32
#define ASICAM_API __cdecl
#else
#define ASICAM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ASI_BAYER_PATTERN {
    ASI_BAYER_RG = 0,
    ASI_BAYER_BG = 1,
    ASI_BAYER_GR = 2,
    ASI_BAYER_RB = 3
} ASI_BAYER_PATTERN;

typedef enum _ASI_IMG_TYPE {
    ASI_IMG_RAW8 = 0,
    ASI_IMG_RGB24 = 1,
    ASI_IMG_RAW16 = 2,
    ASI_IMG_Y8 = 3,
    ASI_IMG_END = -1
} ASI_IMG_TYPE;

typedef enum _ASI_GUIDE_DIRECTION {
    ASI_GUIDE_NORTH = 0,
    ASI_GUIDE_SOUTH = 1,
    ASI_GUIDE_EAST = 2,
    ASI_GUIDE_WEST = 3
} ASI_GUIDE_DIRECTION;

typedef enum _ASI_FLIP_STATUS {
    ASI_FLIP_NONE = 0,
    ASI_FLIP_HORIZ = 1,
    ASI_FLIP_VERT = 2,
    ASI_FLIP_BOTH = 3
} ASI_FLIP_STATUS;

typedef enum _ASI_CAMERA_MODE {
    ASI_MODE_NORMAL = 0,
    ASI_MODE_TRIG_SOFT_EDGE = 1,
    ASI_MODE_TRIG_RISE_EDGE = 2,
    ASI_MODE_TRIG_FALL_EDGE = 3,
    ASI_MODE_TRIG_SOFT_LEVEL = 4,
    ASI_MODE_TRIG_HIGH_LEVEL = 5,
    ASI_MODE_TRIG_LOW_LEVEL = 6,
    ASI_MODE_END = -1
} ASI_CAMERA_MODE;

typedef enum _ASI_TRIG_OUTPUT_PIN {
    ASI_TRIG_OUTPUT_PINA = 0,
    ASI_TRIG_OUTPUT_PINB = 1,
    ASI_TRIG_OUTPUT_NONE = -1
} ASI_TRIG_OUTPUT_PIN;

typedef enum _ASI_ERROR_CODE {
    ASI_SUCCESS = 0,
    ASI_ERROR_INVALID_INDEX,
    ASI_ERROR_INVALID_ID,
    ASI_ERROR_INVALID_CONTROL_TYPE,
    ASI_ERROR_CAMERA_CLOSED,
    ASI_ERROR_CAMERA_REMOVED,
    ASI_ERROR_INVALID_PATH,
    ASI_ERROR_INVALID_FILEFORMAT,
    ASI_ERROR_INVALID_SIZE,
    ASI_ERROR_INVALID_IMGTYPE,
    ASI_ERROR_OUTOF_BOUNDARY,
    ASI_ERROR_TIMEOUT,
    ASI_ERROR_INVALID_SEQUENCE,
    ASI_ERROR_BUFFER_TOO_SMALL,
    ASI_ERROR_VIDEO_MODE_ACTIVE,
    ASI_ERROR_EXPOSURE_IN_PROGRESS,
    ASI_ERROR_GENERAL_ERROR,
    ASI_ERROR_INVALID_MODE,
    ASI_ERROR_END
} ASI_ERROR_CODE;

typedef enum _ASI_BOOL {
    ASI_FALSE = 0,
    ASI_TRUE = 1
} ASI_BOOL;

typedef enum _ASI_CONTROL_TYPE {
    ASI_GAIN = 0,
    ASI_EXPOSURE,
    ASI_GAMMA,
    ASI_WB_R,
    ASI_WB_B,
    ASI_BRIGHTNESS,
    ASI_OFFSET = 5,
    ASI_BANDWIDTHOVERLOAD,
    ASI_OVERCLOCK,
    ASI_TEMPERATURE, // 10*temp
    ASI_FLIP,
    ASI_AUTO_MAX_GAIN,
    ASI_AUTO_MAX_EXP,
    ASI_AUTO_MAX_BRIGHTNESS,
    ASI_HARDWARE_BIN,
    ASI_HIGH_SPEED_MODE,
    ASI_COOLER_POWER_PERC,
    ASI_TARGET_TEMP,
    ASI_COOLER_ON,
    ASI_MONO_BIN,
    ASI_FAN_ON,
    ASI_PATTERN_ADJUST,
    ASI_ANTI_DEW_HEAT,
    ASI_FAN_ADJUST
} ASI_CONTROL_TYPE;

typedef enum _ASI_EXPOSURE_STATUS {
    ASI_EXP_IDLE = 0,
    ASI_EXP_WORKING,
    ASI_EXP_SUCCESS,
    ASI_EXP_FAILED
} ASI_EXPOSURE_STATUS;

typedef struct _ASI_CAMERA_INFO {
    char Name[64];
    int CameraID;
    long MaxHeight;
    long MaxWidth;
    ASI_BOOL IsColorCam;
    ASI_BAYER_PATTERN BayerPattern;
    int SupportedBins[16];
    ASI_IMG_TYPE SupportedVideoFormat[8];
    double PixelSize; // in um
    ASI_BOOL MechanicalShutter;
    ASI_BOOL ST4Port;
    ASI_BOOL IsCoolerCam;
    ASI_BOOL IsUSB3Host;
    ASI_BOOL IsUSB3Camera;
    float ElecPerADU;
    int BitDepth;
    ASI_BOOL IsTriggerCam;
    char Unused[16];
} ASI_CAMERA_INFO;

typedef struct _ASI_CONTROL_CAPS {
    char Name[64];
    char Description[128];
    long MaxValue;
    long MinValue;
    long DefaultValue;
    ASI_BOOL IsAutoSupported;
    ASI_BOOL IsWritable;
    ASI_CONTROL_TYPE ControlType;
    char Unused[32];
} ASI_CONTROL_CAPS;

typedef struct _ASI_ID {
    unsigned char id[8];
} ASI_ID;

typedef struct _ASI_SUPPORTED_MODE {
    ASI_CAMERA_MODE SupportedCameraMode[16];
} ASI_SUPPORTED_MODE;

/* Functions explicitly requested and core SDK APIs */
int ASICAM_API ASIGetNumOfConnectedCameras();
ASI_ERROR_CODE ASICAM_API ASIGetCameraProperty(ASI_CAMERA_INFO *pCameraInfo, int iCameraIndex);
ASI_ERROR_CODE ASICAM_API ASIOpenCamera(int iCameraID);
ASI_ERROR_CODE ASICAM_API ASIInitCamera(int iCameraID);
ASI_ERROR_CODE ASICAM_API ASICloseCamera(int iCameraID);

ASI_ERROR_CODE ASICAM_API ASIGetNumOfControls(int iCameraID, int *piNumberOfControls);
ASI_ERROR_CODE ASICAM_API ASIGetControlCaps(int iCameraID, int iControlIndex, ASI_CONTROL_CAPS *pControlCaps);
ASI_ERROR_CODE ASICAM_API ASIGetControlValue(int iCameraID, int ControlType, long *plValue, ASI_BOOL *pbAuto);
ASI_ERROR_CODE ASICAM_API ASISetControlValue(int iCameraID, int ControlType, long lValue, ASI_BOOL bAuto);

ASI_ERROR_CODE ASICAM_API ASIGetROIFormat(int iCameraID, int *piWidth, int *piHeight, int *piBin, ASI_IMG_TYPE *pImg_type);
ASI_ERROR_CODE ASICAM_API ASISetROIFormat(int iCameraID, int iWidth, int iHeight, int iBin, ASI_IMG_TYPE Img_type);
ASI_ERROR_CODE ASICAM_API ASIGetStartPos(int iCameraID, int *piStartX, int *piStartY);
ASI_ERROR_CODE ASICAM_API ASISetStartPos(int iCameraID, int iStartX, int iStartY);
ASI_ERROR_CODE ASICAM_API ASIGetDroppedFrames(int iCameraID, int *piDropFrames);

ASI_ERROR_CODE ASICAM_API ASIStartVideoCapture(int iCameraID);
ASI_ERROR_CODE ASICAM_API ASIStopVideoCapture(int iCameraID);
ASI_ERROR_CODE ASICAM_API ASIGetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms);

ASI_ERROR_CODE ASICAM_API ASIStartExposure(int iCameraID, ASI_BOOL bIsDark);
ASI_ERROR_CODE ASICAM_API ASIStopExposure(int iCameraID);
ASI_ERROR_CODE ASICAM_API ASIGetExpStatus(int iCameraID, ASI_EXPOSURE_STATUS *pExpStatus);
ASI_ERROR_CODE ASICAM_API ASIGetDataAfterExp(int iCameraID, unsigned char* pBuffer, long lBuffSize);

ASI_ERROR_CODE ASICAM_API ASIGetID(int iCameraID, ASI_ID *pID);
ASI_ERROR_CODE ASICAM_API ASISetID(int iCameraID, ASI_ID ID);
ASI_ERROR_CODE ASICAM_API ASIGetGainOffset(int iCameraID, int *offset_highest_DR, int *offset_unity_gain, int *gain_lowest_RN, int *offset_lowest_RN);
const char* ASICAM_API ASIGetSDKVersion();

#ifdef __cplusplus
}
#endif

#endif // ASICAMERA2_H
