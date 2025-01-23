include "dependencies.lua"

workspace "gameboy-emulator"
    architecture "x64"
    startproject "emulator"
    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"

group "dependencies"
    include "ext/glfw"
    include "ext/imgui"

group ""
    include "emulator"
    include "renderer"