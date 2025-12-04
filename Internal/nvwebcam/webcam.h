#pragma once

#include <cuda.h>
#include <array>

struct ID3D11Device;

#ifndef DLL
#define DLL __declspec(dllimport)
#endif

enum WebcamErr
{
    ErrNoWebcamFound = 0,
    ErrOk,
    ErrFail,
    ErrNoSourceReader,
    ErrStreamNotFound,
    ErrNoFrameAvailable,
    ErrFrameNotReady,
    ErrIllegalArgument,
    ErrNoImplementationFound,
    ErrPreviewWindowNotSet,
    WebcamErrCount,
};

enum WebcamMode
{
    DEFAULT = 0,
    ASYNC,
    WebcamModeCount,
};

enum WebcamFormat
{
    H264 = 0,
    NV12,
    WebcamFormatCount,
};

enum WebcamOutput
{
    DX = 0,
    CUDA_ARRAY,
    CUDA_PITCH2D,
    WebcamOutputCount,
};

enum WebcamControlParameter
{
    CameraExposure = 0,
    CameraGain = 1,
    CameraGamma = 2,

};

struct ICaptureWebcamStream
{
    virtual ~ICaptureWebcamStream();

    WebcamFormat format;
    float fps;
    uint32_t width;
    uint32_t height;
};

struct ICaptureWebcamLaunchParams
{
    virtual ~ICaptureWebcamLaunchParams();

    ID3D11Device* d3dDevice;
    CUcontext cuContext;
    WebcamMode launchMode;
    WebcamOutput outputType;
    bool showPreviewWindow;
    uint32_t bufferSize;
};

struct ICaptureWebcamDesc
{
    virtual ~ICaptureWebcamDesc();

    // Populated by WC_FindWebcams. Helps user select the webcam they want
    std::array<char, 256> name; // friendly name
    
    // driver symbolic link, device instance path (hardware id # instance id # some non-global id \ global
    std::array<char, 512> guid;
    
    // Selected from WC_GetWebcamStreams
    ICaptureWebcamStream* stream;

    // Populated by user
    ICaptureWebcamLaunchParams params;
};

struct FrameDesc
{
    DLL virtual ~FrameDesc();

    union
    {
        // User-provided destination surfaces
        ID3D11Texture2D* m_texture;
        CUarray          m_cuArray;
        CUdeviceptr      m_cuPitch2D;
    };
    long long            m_timeStamp = 0LL;
    float                m_averageFrameTime = 0LL;
    uint32_t             m_numFrames = 0LL;
};

interface ICaptureWebcam
{
public:
    ICaptureWebcam();
    virtual ~ICaptureWebcam();

    virtual WebcamErr Init() = 0;

    virtual WebcamErr ReadFrame(FrameDesc& outFrame) = 0; // get the recorded frame if ready
    
    virtual void SetPreviewWindow(HWND windowHandle) = 0;
    virtual void SetPreviewWindow(const char* title, bool fullscreen = false) = 0;
    virtual HWND GetPreviewWindowHWND() = 0;
    virtual WebcamErr UpdatePreviewWindow() = 0;

    virtual ID3D11Device* GetD3DDevice() = 0;
    virtual uint32_t GetWidth() = 0;
    virtual uint32_t GetHeight() = 0;

    

    virtual void GetParameterRange(const WebcamControlParameter&param, int32_t &min, int32_t &max, int32_t& step, int32_t& defaultValue) = 0;
    virtual void GetParameterValue(const WebcamControlParameter& param, int32_t& value) = 0;
    virtual bool SetParameterValue(const WebcamControlParameter& param, const int32_t& value) = 0;
};

extern DLL WebcamErr WC_FindWebcams(ICaptureWebcamDesc**& outDescs, uint32_t& outSize);
extern DLL WebcamErr WC_GetWebcamStreams(ICaptureWebcamDesc* inDesc, ICaptureWebcamStream**& outStreams, uint32_t& outSize);
extern DLL WebcamErr WC_CreateICaptureWebcam(ICaptureWebcam*& cam, ICaptureWebcamDesc*& desc);
extern DLL void WC_DestroyDescs(ICaptureWebcamDesc** descs, uint32_t size);
extern DLL void WC_DestroyStreams(ICaptureWebcamStream** streams, uint32_t size);
extern DLL void WC_DestroyWebcam(ICaptureWebcam* cam);
