SETLOCAL
CALL SetupEnv.bat

:: Default values, loopback on localhost
SET input_video=../../../nv3dvc_resources/webcam/FaceTrack.mp4

:: Arguments to batch file, if provided
IF [%~1] NEQ [] (SET "input_video=%~1")

:: Replace \ with / so we can embed the path in the JSON
SET "input_video=%input_video:\=/%"

SET additional_scene_arg= ^
{ ^
  ""scene"" : { ^
    ""entities"" : { ^
      ""049e18eb-dd87-41cc-a104-7d33e8530ec6"" : { ^
        ""components"" : { ^
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

.\MagicMirrorLocalApplication.exe AppConfig.json MagicMirrorLocal_scene.json "%additional_scene_arg%"
