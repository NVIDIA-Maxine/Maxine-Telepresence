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
      ""5808d083-a951-4d35-989c-a00c010a0663"": { ^
        ""components"": { ^
          ""CameraCalibrationComponent"": {}, ^
          ""TrackedHeadComponent"": {}, ^
          ""TransformComponent"": { ^
            ""rotation"": [0.0, 0.9961947202682495, 0.08715573698282242, 0.0], ^
            ""translation"": [0.0, 0.2, 0.0] ^
          }, ^
          ""VideoFrameComponent"": {}, ^
          ""WebCameraComponent"": { ^
            ""camera_descriptor"": { ^
              ""camera_device_index"": 0 ^
            }, ^
            ""capture_api"": ""OPENCV_WEBCAM"" ^
          } ^
        }, ^
        ""name"": ""Webcam"", ^
        ""parent"": ""c2df86aa-96ce-4b08-b012-46210542c45c"" ^
      }, ^
      ""c2df86aa-96ce-4b08-b012-46210542c45c"": { ^
        ""components"": { ^
          ""StereoViewComponent"": {}, ^
          ""ViewExtensionBehavior"": {} ^
        }, ^
        ""name"": ""Display"" ^
      } ^
    } ^
  } ^
}

.\PlaybackApplication.exe AppConfig.json Playback_scene.json "%additional_scene_arg%"
