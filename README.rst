===================
PythonVM in mongodb
===================

Just like that! This is a scripting module that brings the Python Virtual Machine into mongodb. This is under development and some features are missing. I certainly wouldn't use it as global scripting engine but it would be nice to write m/r operations in python.


Installation
============

* Copy the engine_python.cpp and engine_python.h files into the mongodb scripting folder
* Edit the SConstruct file adding the following options:

  * 118: boostLibs = [ "thread" , "filesystem" , "program_options", "python" ]
  * 392: scriptingFiles = [ "scripting/engine.cpp" , "scripting/utils.cpp" , "scripting/bench.cpp", Glob("scripting/*python*.cpp")  ]
  * 932: myCheckLib([ "python", "python2.7" ], True)

