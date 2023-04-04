#!/bin/bash

glslc -fshader-stage=vert ../shaders/vert.glsl -o vert.spv
glslc -fshader-stage=frag ../shaders/frag.glsl -o frag.spv

sleep 5
