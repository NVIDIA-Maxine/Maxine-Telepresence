@ECHO OFF
:: The app will look for env vars ARSDK, VFXSDK, AFXSDK (for model directory setup)
SET ARSDK=../ARSDK
SET VFXSDK=../VFXSDK
SET AFXSDK=../AFXSDK
:: Set USE_APP_PATH to avoid library .dll search in default location (under ProgramFiles), per proxy loaders.
:: We set the PATH variable below instead to find AR and VFX libraries (local installations)
SET NV_AR_SDK_PATH=USE_APP_PATH
SET NV_VIDEO_EFFECTS_PATH=USE_APP_PATH
SET PATH=%ARSDK%\bin;%PATH%
SET PATH=%ARSDK%\features\nvarfaceboxdetection\bin;%PATH%
SET PATH=%ARSDK%\features\nvarfaceexpressions\bin;%PATH%
SET PATH=%ARSDK%\features\nvarlandmarkdetection\bin;%PATH%
SET PATH=%ARSDK%\features\nvarvolumetricencoding\bin;%PATH%
SET PATH=%ARSDK%\features\nvarvolumetricrendering\bin;%PATH%
SET PATH=%VFXSDK%\bin;%PATH%
SET PATH=%VFXSDK%\features\nvvfxgreenscreen\bin;%PATH%
SET PATH=%AFXSDK%\bin;%PATH%
SET PATH=%AFXSDK%\features\nvafxaec\bin;%PATH%
SET PATH=%AFXSDK%\bin\external\cuda\bin;%PATH%
SET PATH=%AFXSDK%\bin\external\openssl\bin;%PATH%
SET PATH=..\NvWebcam\bin;%PATH%
SET PATH=..\External\HoloPlay\bin;%PATH%
SET PATH=..\External\OpenCV\x64\vc16\bin;%PATH%
SET PATH=..\External\sr\bin;%PATH%
SET PATH=..\External\TensorRT\lib;%PATH%
SET PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin;%PATH%
SET GST_PLUGIN_PATH_1_0=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\lib\gstreamer-1.0
