# Flowing-Rendering-Engine
A Ray Tracing Engine, written in Vulkan.

This is just a Path Tracer I have created for fun. I also want to look into ease of use, when using this once I'm finished
and am also looking into implementing stuff like:

[ ] ReSTIR
[ ] HDR Support
[ ] BVH
[ ] Direct Illumination
[ ] Indirect Illumination
[ ] BRDF
[ ] Texture support

and more!

If you want to compile it for yourself, do as follows:

### Linux

First make sure that you have all the gitsubmodules installed by doing:
```
git submodule init
git submodule update
```

Then go ahead and create a Build directory and cd into it like this:
```
mkdir build
cd ./build
```

After that you should Build it using CMake,
You can do that by first doing this in the build folder:
```
cmake -S ../ -B ./
```

Then use make to compile it:
```
make
```

I have 12 Threads on my Processor, so I will use -j12 to speed it up:
```
make -j12
```

Make sure shaders are compiled (even though CMake should handle that for you) by doing:
```
cd ../shaders/
./compile.sh
```

The actual application should be in the bin folder!

### Windows

For Windows, I recommend using Visual Studio 2022, as that's what I used to develop this.
To compile it with Windows, I will use CMake's GUI Tool, instead of using the Terminal.

First in the root of the repo, create a folder named Build.

Then Open CMake GUI.
In CMake GUI Select "Browse Source...":
![Screenshot 2023-04-04 112318](https://user-images.githubusercontent.com/96610933/229748141-f254a008-f692-4cca-a5a0-a65e37edfc16.png)
Select the root of the repo (whereever you placed that thing).

Next in CMake GUI, select Browse Build:
![image](https://user-images.githubusercontent.com/96610933/229749035-a125a6c5-601c-4a46-b2b7-d4d6f28aa544.png)

This time you select the newly created Build folder:
![image](https://user-images.githubusercontent.com/96610933/229749336-9151f681-ca1a-4c88-b1c0-3b510cdffa88.png)

Once you've done that, press Configure on the bottom left of the window:
![image](https://user-images.githubusercontent.com/96610933/229749497-6b75ccf6-737f-4d49-9b53-42db57014791.png)

This winodw should pop up:
![image](https://user-images.githubusercontent.com/96610933/229749802-e2801685-1c50-41c1-aabc-a6e72acfc9fa.png)
Here you can select the compiler you want to use. Once you've chosen that you can just press on finish.
It should start configuring everything now.

Once it finished configuring, go ahead and press generate:
![image](https://user-images.githubusercontent.com/96610933/229750812-0efdf4ab-492a-46cb-a343-68b2bb45faf0.png)

Once you've done that, you can open the solution file in the Build folder (if you chose Visual Studio idk about other IDEs).

In Visual Studio you need to do 1 more thing before you run the project!
In the Project Folder Explorer, right click "flowing-rendering-engine" and press Set as default
![Screenshot 2023-04-04 113608](https://user-images.githubusercontent.com/96610933/229752047-60f85e50-f3b6-4123-90d8-6a4f8b17417f.png)

Now you can go to the main.cpp file and compile using F5
