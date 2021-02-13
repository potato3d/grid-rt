# grid-rt
GPU-accelerated ray tracing using GLSL and CUDA

Source code for the article [GPU-Accelerated Uniform Grid Construction for Ray Tracing](http://www.dbd.puc-rio.br/depto_informatica/09_14_ivson.pdf)

This repository implements the Qt ray-tracing application that uses the library from the [grid](https://github.com/potato3d/grid) repository.

# Description

We trace one ray per GPU thread through a uniform grid spatial acceleration structure using a 3D-DDA algorithm. We have two implementations for the ray tracing: GLSL-based and CUDA-based. At the time, the GLSL implementation was about 30% faster. Please check the companion project [grid](https://github.com/potato3d/grid) for the implementation of the GPU-based uniform grid construction.

The GLSL ray tracing implementation is in include/rtgl and src/rtgl directories.

The CUDA ray tracing implementation is in include/rtc and src/rtcuda directories.

This project also includes CPU-based ray tracing routines in include/rt, include/rtp, src/rtcore and src/rtplugins directories. Also includes CPU-based optimized kd-tree construction and traversal routines.

# Examples

These are some images generated with our algorithm for benchmark animated scenes:

![scenes](https://github.com/potato3d/grid/blob/main/imgs/scenes.png "Animated scenes")

These are the results in performance, compared to state of the art:

![speed](https://github.com/potato3d/grid/blob/main/imgs/speed.png "Performance results")

# Build

Visual Studio project files available in visualstudio directory.
