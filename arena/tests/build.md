# BUILD
### How to build?

Enter these commands in the shell, which is usually bash in Linux.
```shell
mkdir build && cd build
cmake ..
make
```

Error? Don't worry, Maybe you didn't install cmake, just
```shell
sudo apt install -y cmake
```
or
```shell
sudo yum install -y cmake
```


Then you can re-enter the commands as described above.

After this, the test programs have been constructed successfully, and you can run these programs as
```shell
./testJSON
./testLua

...
```
Usually you'll get results as follows:
```shell
(base) myo@Myo ~/rocoarena/arena/tests $ cd build/
(base) myo@Myo ~/rocoarena/arena/tests/build $ ls
CMakeCache.txt  CMakeFiles  Makefile  cmake_install.cmake  config.hpp  testJSON  testLua
(base) myo@Myo ~/rocoarena/arena/tests/build $ ./testJSON 
[20:36:54 INFO][JSONLoader] Loaded JSON file: ../testcases/test.json
(base) myo@Myo ~/rocoarena/arena/tests/build $ ./testLua 
[20:36:59 INFO][Scripter] Lua initialized.
lua load success!
[20:36:59 INFO][Scripter] Loaded script: /home/myo/rocoarena/arena/tests/testcases/test.lua
[20:36:59 ERROR][Scripter] Error running code string: lua: error: [string "printf(1111)"]:1: attempt to call a nil value (global 'printf')
680
[20:36:59 INFO][Scripter] Executed code string.
```
myo is my username and Myo is the machine name, this will be different on other computers.

The output includes colored portions, which are provided by the logger. Meanwhile, clever you must find that there's an error at testLua in outputs. You may doubt why it compiles successfully but throws error. Actually, I executed the statement `scr.runString("printf(1111)");` in `tests/core/testLua.cpp`, but Lua doesn't have a `printf` function. You can see the implementation of the `runString(...)` function in `src/core/scripter/scripter.cpp`, which includes error handling; this is why the error occurred. These are just a few examples designed to help those who might have questions about the test output. If you think there are areas for improvement in this project's code, please feel free to submit issues to discuss them.