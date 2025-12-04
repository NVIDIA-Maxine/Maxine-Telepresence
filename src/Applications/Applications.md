# Reference Applications {#Apps}

The reference applications open a video source containing a person, convert the person’s face from 2D-to-3D, and either send the encoded 3D face to a remote participant for rendering, or render it locally.

A complete test requires two machines on the same LAN:
- Windows client 1, with the reference application package built or downloaded
- Windows client 2, with the reference application package built or downloaded

## Requirements

Preferred GPU: NVIDIA RTX 4090

## LocalVideoConferencingApplication {#Apps_Local}

See detailed description in @ref LocalVideoConferencingApplication

Make sure to use headphones to avoid audio feedback!

*See test cases below*

### Local Loopback

- On Windows client 1:
  - Run: `.\run_VideoConferencing_Live.bat`
- Expected result:
  - The user should see a live preview of themselves
  - The view should update per the user's viewpoint
  - The preview should not be mirrored
  - Audio should be looped back from microphone to head phones
  - Received audio and rendered video should be in sync

### 1:1 - Video

- On Windows client 1:
  - Run (in terminal 1): `.\run_VideoConferencing_Video.bat 127.0.0.1 6500 6000`
  - Run (in terminal 2): `.\run_VideoConferencing_Video.bat 127.0.0.1 6000 6500`
- Expected result:
  - The user should see two previews of the video subject in windowed mode
  - The view should update per the subject in the source video's viewpoint
  - Audio should be received from both recorded videos to head phones
  - Received audio and rendered video should be in sync

### Local Preview - Video

- On Windows client 1:
  - Run (in terminal 1): `.\run_VideoConferencing_Video.bat 127.0.0.1 6500 6000`
  - Run (in terminal 2): `.\run_VideoConferencing_Live.bat 127.0.0.1 6000 6500`
- Expected result:
  - The user should see a preview of the video subject
  - The view should update per the user's viewpoint
  - Audio should be received from recorded video to head phones
  - Received audio and rendered video should be in sync

### Remote Preview - Video

- On Windows client 1:
  - Run: `.\run_VideoConferencing_Video.bat <client 2 IP address> 6500 6000`
- On Windows client 2:
  - Run: `.\run_VideoConferencing_Live.bat <client 1 IP address> 6000 6500`
- Expected result:
  - The user should see a preview of the video subject
  - The view should update per the user's viewpoint
  - Audio should be received from recorded video to head phones
  - Received audio and rendered video should be in sync

### Remote Live Teleconferencing

Requires two participants, one operating each client

- On Windows client 1:
  - Run: `.\run_VideoConferencing_Live.bat <client 2 IP address> 6500 6000`
- On Windows client 2:
  - Run: `.\run_VideoConferencing_Live.bat <client 1 IP address> 6000 6500`
- Expected result:
  - The users should see a live preview of the other subject
  - The view should update per the user's viewpoint
  - Audio should be received from the other user's microphone
  - Received audio and rendered video should be in sync

## MagicMirrorLocalApplication {#Apps_Mirror}

See detailed description in @ref MagicMirrorLocalApplication

*See test cases below*

### Live Mirror

- On Windows client 1:
  - Run: `.\run_MagicMirrorLocal.bat`
- Expected result:
  - The user should see a live preview of themselves
  - The view should update per the user's viewpoint
  - The app should simulate the effect of a mirror

### Video Mirror

- On Windows client 1:
  - Run: `.\run_MagicMirrorLocal_Video.bat`
- Expected result:
  - The user should see a preview of the video subject
  - The view should update per the subject in the source video's viewpoint

## PlaybackApplication {#Apps_Playback}

See detailed description in @ref PlaybackApplication

*See test cases below*

### Live Preview Playback

- On Windows client 1:
  - Run: `.\run_Playback_View.bat`
- Expected result:
  - The user should see a realistic live preview of the video subject
  - The view should update per the user's viewpoint

### Static Playback

- On Windows client 1:
  - Run: `.\run_Playback_Offline.bat`
- Expected result:
  - The user should see a preview of the video subject
  - The viewpoint should be static
  - A recording of the rendered video should be saved as Playback_recording.mp4

## Common {#Apps_Common}

Common elements shared between applications

### Editing App Config Using GUI

All public properties of the engine and its registered modules and systems can be edited from the GUI.
Some properties take immediate effect while others require a restart.

If the Application Config panel is not shown, make sure the following are checked
- Window -> Application Config

Any property can be edited in the GUI and the application config can be saved and reopened by restarting the application.

### Saving and Loading App Configs

#### Overwrite Current Application Config

Save the application config by going to File -> Save Application Config.
This will overwrite the currently loaded application config.

Depending on the currently running app, the application config will either be called <\App name\>.json, or possibly
AppConfig.json depending on which batch file was used to run the application. The application config should be the first
argument to the execution of the application.

#### Save a New Application Config

To avoid overwriting any existing application config, instead use File -> Save Application Config As ...
This will open a file dialog, and a file name can be selected.

To run the application with the new application config, the first argument to the application execution should be the path to the new application config.

### Editing Scene Using GUI

All public properties of the components that are owned by the entities in the scene can be edited from the GUI.
Some properties take immediate effect while others require a restart.

If the Scene Hierarchy panel and the Properties panel is not shown, make sure the following are checked
- Window -> Scene Hierarchy
- Window -> Entity Properties

In the Scene Hierarchy, any entity can be selected. The tree view of its children (if any) can be expanded recursively.
The selected entity's components and properties will be shown in the separate Entity Properties panel

Any property can be edited in the GUI and the scene config can be saved and reopened

### Saving and Loading Scene Files

#### Overwrite Current Scene

Save the scene by going to File -> Save Scene.
This will overwrite the currently loaded scene.

#### Save a New Scene

To avoid overwriting any existing scene file, instead use File -> Save Scene As ...
This will open a file dialog, and a file name can be selected.

#### Loading Scenes

To load any scene, use File -> Load Scene ...
This will open a file dialog, and a file name can be selected.

To empty the scene, use File -> Empty Scene

To load the currently running application's default scene, use File -> Load Default Scene

### Direct File Edits

Sometimes it is beneficial to edit the application config files or scene files directly using a text editor.

Open any application config, or scene file, and make any edits to the existing properties, or make additional required changes to the hierarchy.

Load the new app config or scene using the above instructions.

### Manual Web Camera Calibration

One special case where editing the scene for your particular setup may be required is for manual calibration of the web camera properties used by the applicaitons.

A typical scene hierarchy consists of a digital double of your physical display + web camera system. If the virtual setup does not match your physical setup, the view plane reprojection will be incorrect and you may experience skewed or otherwise incorrectly calibrated rendering of the viewed subject.

The calibration consists of two steps; *intrinsics calibration* and *extrinsics calibration*.

#### Intrinsics Calibration

For this, you need to know either
1. The vertical field of view (vFOV) of your web camera
2. The vertical and horizontal focal length of your web camera (in pixels) and its principal point (in pixels) 

Typically, a simple calibration using the vFOV only suffices.

In the scene, look for the @ref WebCameraComponent and @ref CameraCalibration.

**Using vFOV**

Make sure *use_vfov* is on.

The *vfov* property can be set directly.

**Using intrinsics**

Make sure *use_vfov* is off.

The following parameters can be set directly:
- *image_width*
- *image_height*
- *fx*
- *fy*
- *cx*
- *cy*

#### Extrinsics Calibration

A physical web camera is typically attached to the top center portion of the display.
This is also true in the default scene configurations provided.
This means that the TransformComponent of the web camera entity defines the transformation from the camera's reference frame to the reference frame of the display.
This transformation needs to match that of your physical setup for the head tracking and the view reprojection to correctly map to the reference frame of your physical display.

To perform the manual calibration, look for the entity which has the @ref WebCameraComponent attached. It should also have a TransofrmComponent attached. We are interested in the rigid transform defined by the translation and the rotation component.

The translation component should define the translation (in meters) from the center of your display to the focus point of the camera.

The rotation component (which is a quaternion) should define the rotation along which the camera is pointing.
This is typically at first a 180 degree rotation to get the camera pointing along the display normal, followed by a slight downward rotation (between 5 and 10 degrees).
With the rotation component being represented as a quaternion, it may be tricky to know the right components.
Therefore it is a good practice to first fix the translation, and then rotate the virtual camera until your rendered face roughly coincides with where the reflection of your face will be on the display - provided the currently running application is the MagicMirrorLocalApplication, and that no additional transform has been added to the display or the rendered volumetric object, e.g. positional calibration.

### Additional GUI Control

#### Show and Hide GUI

The GUI system can be enabled and disabled using the F11 key

#### Positional Calibration

Press **C** to trigger positional calibration. This will center the rendered object head onto the view plane. See @ref PoseCalibrationBehaviorProperties for details.

The calibration can be undone using *trigger_uncalibrate*.

#### Manual Camera Control

WASD keys and mouse drag enables moving around the scene by changing the location of your virtual display.
See @ref CameraControlBehaviorProperties for details.
