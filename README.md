# This project is deprecated for now, if you're that interessted, seek therapy honestly lmfao



# FREE (Flowing Ray Tracing Engine)
A Ray Tracing Engine that doesn't require RT Hardware, written in Vulkan (cuz why not).

It's deprecated for now as I'm going to work on 2.0 which will be a rework of this,
since I took alot of the code from a Vulkan tutorial which probably was a terrible decision!

I'll go back to Vulkan once I have gained enough knowledge about Vulkan myself

This is just a Path Tracer I have created for fun. I also want to look into ease of use, when using this once I'm finished
and am also looking into implementing stuff like:

- [ ] ReSTIR GI
- [ ] Denoiser (A-SVGF)
- [ ] HDR Support
- [ ] BVH
- [x] AABB (Axis-Aligned Bounding Box)
- [x] Direct Illumination
- [x] Indirect Illumination
- [x] Mesh Importing (not pushed yet)
- [x] Mesh creation after importing Triangle  (not pushed yet)
- [ ] Texture support

and more!

### Ideas for Optimization:
- I could try using the Rasterizer Hardware in the GPU to know which Triangles can be hit the first time the Rays are shot. That way you don't need to check every Triangle but only the ones that are actually visible
Problems with that: I don't know why I should even do that since the performance gain wouldn't be very large from that lol + it would make alot of things way too complex!

If you want to compile it for yourself, make sure the VulkanSDK from LunarG is installed and do as follows:

### Linux

First make sure that you have all the git submodules installed by doing:
```
git submodule init
git submodule update
```

Then go ahead and create a Build directory and cd into it like this:
```
mkdir build
cd ./build
```

Make sure shaders are compiled (even though CMake should handle that for you) by doing:
```
sh ../shaders/compile.sh
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

I have 12 Threads on my Processor, so I will use -j12 to use 12 threads:
```
make -j12
```

The actual application should be in the bin folder!

### Windows

For Windows, I recommend using Visual Studio 2022, as that's what I used to develop this.
To compile it with Windows, I will use CMake's GUI Tool, instead of using the Terminal.

First in the root of the repo, create a folder named Build.

Then Open CMake GUI.
In CMake GUI Select "Browse Source...":

![image](https://user-images.githubusercontent.com/96610933/229748141-f254a008-f692-4cca-a5a0-a65e37edfc16.png)

Select the root of the repo (whereever you placed that thing).

Next in CMake GUI, select Browse Build:

![image](https://user-images.githubusercontent.com/96610933/229749035-a125a6c5-601c-4a46-b2b7-d4d6f28aa544.png)

This time you select the newly created Build folder:

![image](https://user-images.githubusercontent.com/96610933/229749336-9151f681-ca1a-4c88-b1c0-3b510cdffa88.png)

Once you've done that, press Configure on the bottom left of the window:

![image](https://user-images.githubusercontent.com/96610933/229749497-6b75ccf6-737f-4d49-9b53-42db57014791.png)

This window should pop up:

![image](https://user-images.githubusercontent.com/96610933/229749802-e2801685-1c50-41c1-aabc-a6e72acfc9fa.png)

Here you can select the compiler you want to use. Once you've chosen that you can just press on finish.
It should start configuring everything now.

Once it finished configuring, go ahead and press generate:

![image](https://user-images.githubusercontent.com/96610933/229750812-0efdf4ab-492a-46cb-a343-68b2bb45faf0.png)

Once you've done that, you can open the solution file in the Build folder (if you chose Visual Studio idk about other IDEs).

In Visual Studio you need to do 1 more thing before you run the project!
In the Project Folder Explorer, right click "flowing-rendering-engine" and press Set as default

![image](https://user-images.githubusercontent.com/96610933/229752047-60f85e50-f3b6-4123-90d8-6a4f8b17417f.png)

Now you can go to the main.cpp file and compile using F5
