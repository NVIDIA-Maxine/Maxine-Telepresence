SETLOCAL
CALL SetupEnv.bat

:: Default values, loopback on localhost
SET their_ip=127.0.0.1
SET source_port=6500
SET sink_port=6500
SET input_video=../../../nv3dvc_resources/webcam/FaceTrack.mp4

:: Arguments to batch file, if provided
IF [%~1] NEQ [] (SET "their_ip=%~1")
IF [%~2] NEQ [] (SET "source_port=%~2")
IF [%~3] NEQ [] (SET "sink_port=%~3")
IF [%~4] NEQ [] (SET "input_video=%~4")

:: Replace \ with / so we can embed the path in the JSON
SET "input_video=%input_video:\=/%"

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
          ""AudioInputComponent"": { ^
            ""capture_api"" : ""WEB_CAMERA"" ^
          }, ^
          ""StreamSinkComponent"": { ^
            ""rtsp_path"": ""/3dvc"", ^
            ""rtsp_port"": %sink_port%, ^
            ""send_audio"": true, ^
            ""sink_type"": ""RTSP_SERVER"" ^
          }, ^
          ""TransformComponent"" : { ^
            ""rotation"": [0.0, 1.0, 0.0, 0.0], ^
            ""translation"": [0.0, 0.0, 0.0] ^
          }, ^
          ""WebCameraComponent"" : { ^
            ""camera_descriptor"" : { ^
              ""camera_device_path"" : ""%input_video%"" ^
            }, ^
            ""capture_api"": ""MULTIMEDIA_FILE"" ^
          } ^
        } ^
      } ^
    } ^
  } ^
}

.\LocalVideoConferencingApplication.exe AppConfigWindowed.json LocalVideoConferencing_scene.json "%additional_scene_arg%"
