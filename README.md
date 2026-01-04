# What is SongDetector?

I often listen to music on internet radio stations and find myself asking 'What was that song?'. SongDetector attempts to answer that question - while 
maintaining a small enough memory footprint that you can leave it running in the system stray. 

**SongDetector depends on PipeWire and is a Linux only application!**

# Building SongDetector

## Dependencies

* Qt (Core, Widgets, Multimedia & SVG) version 6.7 or higher
* **libpipewire-dev** I've built and tested with 1.4.7. Earlier versions probably work, but are untested.
* SongDetector uses the [**Vibra**](https://bayernmuller.github.io/blog/240206-shazam-client-vibra/) library to create an audio fingerprint . This currently doesn't have any installable packages, so it's included as a Git sub-module.
* Vibra requires **libcurl4-openssl-dev** and **libfftw3-dev**

## Building

### Build Vibra

```
cmake -S vibra -B vibra/build
cmake --build vibra/build
```

### Build SongDetector

```
mkdir build
cd build
cmake ..
make
```

## Using SongDetector

Start the application by running `SongDetector` from the `build` directory. SongDetector will start in the system tray in idle mode. Use the **Start Identify** right-click menu to start detection.
SongDetector will capture 10 seconds of audio, generate an audio fingerprint and look that up in the Shazam database. SongDetector will show a notification pop-up whether the song is found or not. 

## SongDetector settings

SongDetector has two settings:

* Audio device - currently a work-in-progress
* Force Dark Mode Icon - SongDetector tries to guess whether to use a light or dark icon, but sometimes gets it wrong. If that's the case, use this checkbox to force the dark mode icon

# Bugs & feature requests

Please raise any bugs or feature requests on [GitHub](https://github.com/MartinHignett/SongDetector/issues). Please check the list of existing issues before creating new ones.
