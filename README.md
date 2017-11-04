# Surfel renderer demo

This is a reasonably simple demo of a surfel renderer using OpenGL compute shaders.

## Build instructions

### Requirements

* Unix-like OS
* SDL2
* OpenGL 4.4+
* GCC, or Clang

### Instructions

In some directory:
```
git clone https://github.com/asgii/surfel-demo
make clang
```
`make gcc` works, too.

### Use

```
build/demo
```

W and S move the camera backward and forward. A and D rotate the camera.

If you have another suitable .pcd file (see 'Resources included', below), you can put it in resources/models/ and name it in the command-line:
```
build/demo ism_train_michael.pcd
```

## Description

### What's a surfel renderer?

In a traditional 3D pipeline, a mesh of vertices of contiguous triangles is processed, and each visible point of a triangle receives a colour value based on an interpolation between the values at each of the triangle's vertices. The triangles are thereby filled.

Surfel renderers don't bother with filling in shapes, but simply transform discrete points to screen. With enough points, a solid surface can be seen. (I didn't end up finding a dense enough model for this to be true, but the demo should work that way in principle!)

### This renderer's architecture

Instead of using vertex and fragment shaders (which are for the traditional 3D pipeline), this demo uses two compute shaders (which are meant for general-purpose computation, in large volumes).

The first, resources/shaders/surfelsToSamples.c.glsl, gets a point per invocation from a Shader Storage Buffer, and writes it to an image after transforming it using the traditional method specified in the OpenGL spec.

The shader uses imageAtomicMax() with the sample's depth at the start, so that the sample with the greatest depth value at a particular screen location will overwrite others; this produces a depth test. I got this idea from [a blog post by Timothy Lottes](https://timothylottes.github.io/20161121.html); in practice, the demo doesn't use the depth very much.*

The second, resources/shaders/samplesToPixels.c.glsl, is not much different from a traditional fragment shader. Each invocation is assigned a coordinate in screen-space and produces a colour by sampling crudely around that coordinate in the samples buffer.

*Note: if you did want to implement more of the ideas in that post, you would need a workaround for using atomic instructions for bit-widths greater than 32 (there are NV extensions for 64-bit atomics in GLSL, but otherwise no support).
You could use imageAtomicMax() at multiple places in the image with the same 8 bits for depth: zxya at one place, zrgb at another, say. Unfortunately that would still lead to conflicts if two values had the same 8-bit depth; then you might get the xya from one sample and the rgb from a completely different one.

## Resources included

Currently this demo only accepts ascii [Point Cloud Data (.pcd) files](http://pointclouds.org/documentation/tutorials/pcd_file_format.php). In resources/models/ I've included a sample file, taken from the [Point Cloud Library's dataset](https://github.com/PointCloudLibrary/data/tree/master/tutorials). It is licensed under the [BSD 3-clause licence](https://github.com/PointCloudLibrary/data/blob/master/LICENSE). Any of the files in that repository should work, as long as they're in ascii format.

Included in lib/ are files generated by glad, an OpenGL loader generator. These files are [in the public domain](https://github.com/Dav1dde/glad#whats-the-license-of-glad-generated-code-101). glad is hosted [here](https://github.com/Dav1dde/glad).

Also in lib/ is a submodule for [geom.h](https://github.com/asgii/geom.h), a small 3D geometry library. I've used it here mainly just to demonstrate that it works. It provides a subset of the kind of features a geometry library like [GLM](https://github.com/g-truc/glm) might. It uses GLSL-like syntax (vec3, mat3, etc.) so it shouldn't be too hard to follow in the source here.