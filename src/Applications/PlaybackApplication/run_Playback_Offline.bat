SETLOCAL
CALL SetupEnv.bat

:: Default values, loopback on localhost
SET input_video=../../../nv3dvc_resources/webcam/FaceTrack.mp4
SET output_video=Playback_recording.mp4

:: Arguments to batch file, if provided
IF [%~1] NEQ [] (SET "input_video=%~1")
IF [%~2] NEQ [] (SET "output_video=%~2")

:: Replace \ with / so we can embed the path in the JSON
SET "input_video=%input_video:\=/%"
SET "output_video=%output_video:\=/%"

SET additional_scene_arg= ^
{ ^
  ""scene"": { ^
    ""entities"": { ^
      ""1ff951d7-b941-425d-92dc-7507860316a5"": { ^
        ""components"": { ^
          ""WebCameraComponent"": { ^
            ""camera_descriptor"": { ^
              ""camera_device_path"": ""%input_video%"" ^
            } ^
          } ^
        } ^
      }, ^
      ""c2df86aa-96ce-4b08-b012-46210542c45c"": { ^
        ""components"": { ^
          ""RecordingBehavior"": { ^
            ""enable"": true, ^
            ""file_path"": ""%output_video%"" ^
          } ^
        }, ^
        ""name"": ""Display"" ^
      } ^
    } ^
  } ^
}

.\PlaybackApplication.exe AppConfig.json Playback_scene.json "%additional_scene_arg%"
