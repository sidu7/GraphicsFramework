# IrradianceMapApp

A standalone command-line utility that reads an HDR environment map and computes a low-order spherical-harmonic irradiance map. It reads `skyDome.hdr` from its working directory and writes the corresponding `skyDome-irradiance.hdr` output.

This utility is separate from the GraphicsFramework CMake project and is not currently a CMake target. Its implementation and RGBE image helpers are in this directory.
