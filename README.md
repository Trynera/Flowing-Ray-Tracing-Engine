# FREE (Flowing Rendering Engine)
A Ray Tracing Engine that doesn't require RT Hardware, written in Vulkan (cuz why not).

It's deprecated for now as I'm going to work on v2a which will be a rework of this,
since I took alot of the code from a tutorial for Vulkan which probably was a terrible decision!

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
Problems with that: I don't know how that could be implemented
