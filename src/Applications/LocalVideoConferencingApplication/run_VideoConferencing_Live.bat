SETLOCAL
CALL SetupEnv.bat

:: Default values, loopback on localhost
SET their_ip=127.0.0.1
SET source_port=6500
SET sink_port=6500

:: Arguments to batch file, if provided
IF [%~1] NEQ [] (SET "their_ip=%~1")
IF [%~2] NEQ [] (SET "source_port=%~2")
IF [%~3] NEQ [] (SET "sink_port=%~3")

SET additional_scene_arg= ^
{ ^
  ""scene"" : { ^
    ""entities"" : { ^
      ""9a798ae5-8bce-4b22-bf6d-a42f3270d75f"": { ^
        ""components"": { ^
          ""StreamSourceComponent"": { ^
            ""receive_audio"": true, ^
            ""rtsp_host"": ""%their_ip%"", ^
            ""rtsp_path"": ""/3dvc"", ^
            ""rtsp_port"": %source_port% ^
          } ^
        } ^
      }, ^
      ""48c17a72-0907-4b1c-9c89-5d7aacd1d1b5"": { ^
        ""components"": { ^
          ""StreamSinkComponent"": { ^
            ""rtsp_path"": ""/3dvc"", ^
            ""rtsp_port"": %sink_port%, ^
            ""send_audio"": true, ^
            ""sink_type"": ""RTSP_SERVER"" ^
          }, ^
          ""WebCameraComponent"" : { ^
            ""capture_api"": ""OPENCV_WEBCAM"" ^
          } ^
        } ^
      } ^
    } ^
  } ^
}

.\LocalVideoConferencingApplication.exe AppConfig.json LocalVideoConferencing_scene.json "%additional_scene_arg%"
