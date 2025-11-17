# Tea for God Open Source Release

Originally created by void room. This repo is an interim way to release portions of the game's source code to the public, as void room himself hasn't had the opportunity to publish his own repo yet. Once his is published with an official mirror of the game's source, this repo will be made private, archived, or deleted. As per conversations with void room, this code is licensed under MIT-0, with a copyright year of the year of the source release, 2025.

## Externals

Note that this repo will not build stock without the following external dependencies:

```txt
CURRENT LIST OF EXTERNALS
 bhaptics_android
 bhaptics_windows
 bzip2
 fmod
 fmod_android
 freetype
 glew
 openvr
 ovr
 ovrMobile ?
 ovrPlatform ?
 openxr
 sdl2
 sdl2_image
 sdl2_net
 steamworks
 viveport
 wavevr
```

Since a good deal of these are proprietary or otherwise forbid redistribution, you will have to obtain these outside of this repo. The maintainer of this mirror, N3rdL0rd, provides a repo containing those externals, and they assume all responsibility for the hosting and redistribution of these externals.

In order to use the externals repo, run:

```bash
rm externals/readme.txt && git clone https://github.com/N3rdL0rd/tfg_externals.git externals
```

This should work on both Windows and Linux, provided you have git installed.

## Assets

You'll need a preview build of Tea for God (ask in the Discord to get one of these, you'll need to show proof of purchase) in order to have the neccesary development assets to use with this code.

## Building

The easiest way to build Tea for God from source is to open the root solution, `aironoevl.sln`, in Visual Studio 2019 or 2022. First, before your first build, run `code/! setup.bat`, and then open the repo in VS.

Select your desired target platform at the build selection bar at the top of VS, and press <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd> to start a build.

You probably want:

<img width="369" height="36" alt="image" src="https://gist.github.com/user-attachments/assets/a85b5584-ea0f-4e2c-b624-740829e299eb" />

### Fixing Errors

You definitely just got an error. Here are some common ones and their fixes.

### The Windows SDK version 10.0.17763.0 was not found. Install the required version of Windows SDK...

<img width="1281" height="73" alt="image" src="https://gist.github.com/user-attachments/assets/90213bec-52cd-4a42-82b5-3bfcff85acd2" />

The easiest fix to this is the suggested one. Right-click the solution in the view and select `Retarget Solution`:

<img width="977" height="648" alt="image" src="https://gist.github.com/user-attachments/assets/acd6c62c-2b09-4bd1-b675-badf850b0aed" />

Leave it at its default to upgrade to your installed SDK version, then continue:

<img width="540" height="362" alt="image" src="https://gist.github.com/user-attachments/assets/5e711c96-d0ba-4833-83dc-57b86759d21c" />

## Running the Game

At this point, you should have a build at `runtime/x64/Dev` (or `runtime/x64/Release`, if you chose a release build). Now, you need to copy over your assets from your preview build of the game so that the game will load.

I have yet to figure out how to get the build system to copy over assets automatically. For now, though, you can just take the `library/`, `system/` and `settings/` folders from your preview build and copy them to `runtime/x64/Dev/`. Also, copy over all `*.xml` files in the preview build to the root of your dev build. Then, in the root of your build, run:

```cmd
del /s /q *.cx
```

Finally, you need to mount `voidroomdata.zip` to a virtual drive. First, pick a location for it - then, unzip the entirety of the extra data there. Then, run `subst V: "C:\path\to\your\folder"`, giving it the path to the unzipped `voidroomdata`.

Now, the game should detect the assets and boot correctly! Just run `teaForGodWin.exe` and it should launch.

## Linux and Ports

Since this repo is meant to be a snapshot of the source code accurate to the original snapshot released to the community, no modifications are made to the actual source tree here. Stay tuned for new community developments and potential ports to new platforms!
