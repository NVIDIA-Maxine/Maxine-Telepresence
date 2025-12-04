# Engine

Please see documentation of the Engine class for its interface.

## Using the engine directly

Below shows sample code for how to use the Engine directly in your own applications or plugins without wrapping its usage within the Application class.
This gives more control and requires lower level access. For example, it enables direct control over the run-loop. This is a simplified example.

Building a simple scene:

\snippet src/Samples/MyPlugin/MyPlugin.cpp Simple scene building

Creating, configuring and running the engine:

\snippet src/Samples/MyPlugin/MyPlugin.cpp Using the engine directly
