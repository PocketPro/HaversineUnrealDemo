# UnrealHaversineDemo

## Overview
This repository provides a demonstration of the **SuperTagKitPlugin** for Unreal Engine 5.5. The demo showcases how to:
- Discover nearby SuperTags (called satellites in this library) 
- Fetch golf swing data from SuperTags
- Parse swing metadata (club type, user ID)
- Generate golf swing metrics

This serves as both a working example and a starting point for building Unreal applications that integrate with Haversine satellite devices.

## Installation

### Prerequisites
- Unreal Engine 5.5
- CMake 3.16 or later
- C++20 compatible compiler (handled by Unreal)

### Steps
1. Clone the repository:
   ```bash
   git clone git@github.com:SkyhawkeTechnologies/unreal-sdk-supertagdemoapp.git
   ```

2. Initialize all submodules (including nested ones):
   ```bash
   cd unreal-sdk-supertagdemoapp
   git submodule update --init --recursive
   ```

3. Install CMake if not already installed:
   - **macOS**: `brew install cmake`
   - **Windows**: Download from https://cmake.org/download/
   - **Linux**: `sudo apt-get install cmake` (or equivalent)

4. Generate Unreal project files:
   - **macOS**: Right-click `UnrealHaversineDemo.uproject` → "Generate Xcode Project"
   - **Windows**: Right-click `UnrealHaversineDemo.uproject` → "Generate Visual Studio project files"

5. Open `UnrealHaversineDemo.uproject` in Unreal Editor
   - The CMake library will be built automatically on first compilation
   - Build the project (Cmd+B on Mac, Ctrl+B on Windows)

### Notes
- The HaversineSatelliteLibrary_CPP CMake build is automated by Unreal's build system
- CMake must be installed and available in your system PATH

## Project Structure

```
UnrealHaversineDemo/
├── Source/                           
│   └── UnrealHaversineDemo/          
|       ├── HaversineDemoSubsystem.cpp  # Example integration of the SuperTagKitPlugin
├── Plugins/
│   └── SuperTagKitPlugin/            
│       ├── Source/                   # Primarily an Unreal C++ interface to HaversineSatelliteLibrary_CPP.
│       │   └── SuperTagKitPlugin/   
│       ├── HaversineSatelliteLibrary_CPP/  # The generic C++ SDK used to communicate with satellites
│       │   ├── frameworks/           # Native frameworks for various platforms that handle actual BLE communication
│       │   │   ├── macos/            
│       │   │   └── windows/          
│       │   └── CMakeLists.txt        # CMake build configuration
└── UnrealHaversineDemo.uproject      # Unreal project file
```

### Key Components

**HaversineDemoSubsystem.cpp** 
- This file contains the example integration of SuperTagKit plugin. 
- It's also intended to serve as a starting point for an production project.

**HaversineSatelliteLibrary_CPP** 
- Our C++ SDK. Not Unreal Engine specific

**SuperTagKitPlugin**
- Provides an Unreal Engine interface to HaversineSatelliteLibrary_CPP.
- Provides helpers to create GolfSwing objects.
- Also automatically handles authentication with SkyGolf API for SuperTags.


## Quick Start

### Running the Demo

1. Launch the Unreal Editor (double-click `UnrealHaversineDemo.uproject`)
2. Open the level: `Content/FirstPerson/Maps/FirstPersonMap`
3. Click **Play** (Alt+P or Play button)
4. The `UHaversineSatelliteSubsystem` will automatically:
   - Start scanning for nearby SuperTags
   - Authenticate discovered satellites with the SkyGolf API
   - Parse metadata (club type, user ID)
   - Log satellite information to the Output Log

### Viewing Logs

- Open **Window > Developer Tools > Output Log** 
- Filter by `LogHaversineSatellite` category
- Look for messages about satellite discovery, authentication, and metadata

### Understanding the Demo
- Read through HaversineDemoSubsystem.cpp and the documentation in that file
