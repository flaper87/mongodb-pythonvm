===================
PythonVM in mongodb
===================

Just like that! This is a scripting module that brings the Python Virtual Machine into mongodb. This is under development and some features are missing. I certainly wouldn't use it as global scripting engine but it would be nice to write m/r operations in python.


Compilation / Installation
==========================

* Copy the engine_python.cpp and engine_python.h files into the mongodb scripting folder
* Edit the SConstruct file adding the following options:

  * 118: boostLibs = [ "thread" , "filesystem" , "program_options", "python" ]
  * 392: scriptingFiles = [ "scripting/engine.cpp" , "scripting/utils.cpp" , "scripting/bench.cpp", Glob("scripting/*python*.cpp")  ]
  * 932: myCheckLib([ "python", "python2.7" ], True)

Notes
=====

* So far there's just a simple unittest that runs when mongodb starts. That unittest has to be light an test some basic functionalities. If you'd like to avoid the unittest simply put a return at the beggining of the run function.

* The SConstruct line numbers are based on the latest mongodb version found on github at the moment of this commit.

* The Python version used so far is 2.7 (I'll implemente 3.x too)

Contributing
============

If you think this project might be useful or simply want to have fun playing with mongodb and python then feel free to write me or simply fork the project and send pull requests.