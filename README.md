use bytebeat expressions to drive a CV interaface

![image](https://user-images.githubusercontent.com/4634469/172029579-6000f5a3-b21f-47c0-9aba-77f57c4522aa.png)

new theme:
![image](https://user-images.githubusercontent.com/4634469/173176133-e7aa6db3-5b65-4fc3-9278-9a46d0384c07.png)


# How to Build



- On Windows with Visual Studio's IDE

Use the provided project file (.vcxproj). Add to solution (imgui_examples.sln) if necessary.

- On Windows with Visual Studio's CLI

```
set SDL2_DIR=path_to_your_sdl2_folder
cl /Zi /MD /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp /FeDebug/example_sdl_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
#          ^^ include paths                  ^^ source files                                                                                   ^^ output exe                    ^^ output dir   ^^ libraries
# or for 64-bit:
cl /Zi /MD /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp /FeDebug/example_sdl_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x64 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
```

- On Linux and similar Unixes

```
c++ `sdl2-config --cflags` -I .. -I ../.. -I ../../backends main.cpp ../../backends/imgui_impl_sdl.cpp ../../backends/imgui_impl_opengl3.cpp ../../imgui*.cpp `sdl2-config --libs` -lGL -ldl
```

- On Mac OS X

```
brew install sdl2
c++ `sdl2-config --cflags` -I .. -I ../.. -I ../../backends main.cpp ../../backends/imgui_impl_sdl.cpp ../../backends/imgui_impl_opengl3.cpp ../../imgui*.cpp `sdl2-config --libs` -framework OpenGl -framework CoreFoundation
```
