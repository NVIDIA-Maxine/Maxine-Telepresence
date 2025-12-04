# Application

This page details the application structure. 
Please see documentation of the Application class for its interface.

## Extending Application

Below shows sample code for how to extend the application with your own MyApplication class:

\snippet src/Samples/MyApplication/MyApplication.cpp Simple application sample

In the main function, create an instance of your application and run it:

\snippet src/Samples/MyApplication/MyApplication.cpp Run MyApplication

## Execution order

All applications extending the nv3dvc::core::application::Application class will execute certain key functions in a specific order.
When extending modules and systems, it is important to note this order for correct application execution. The order of operations is listed below.

Operation              | Purpose
-------------          | -------------
Initialize CUDA device | CUDA is initialized first so that all Modules and Systems that use the GPU can access the CUDA context
Initialize Modules     | The `Initialize()` method of each registered Module is called. Note, that at this point, properties are not yet loaded.
Initialize Systems     | The `Initialize()` method of each registered System is called. Note, that at this point, properties are not yet loaded.
Load Scene             | A scene is loaded. This is when all property values of Components, and Systems get set according to the scene configuration.
Start up               | The `OnLoadScene()` method of each Module and each System is called on the main thread. Note, that at this point, properties have been set and are available.
Run                    | Main loop: Events are broadcast on the main thread, Run() is called for all Systems, Update() is called for all Modules on the main thread.
Close                  | All threads are joined, Run is no longer called in a loop
ShutDown               | `OnUnLoadScene()` is called for every System
Uninitialize           | The `Uninitialize()` method of each System and each Module is called.
