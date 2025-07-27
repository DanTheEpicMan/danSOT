# DanSOT
SOT cheat for linux, below are tested install instructions for Kubuntu
# Setup
1) Install linux (I used KUbuntu, whatever distro you choose it needs to use Wayland or have an option (like KUbuntu) to use wayland)
2) Git clone this repo info a folder
``` git clone https://github.com/DanTheEpicMan/danSOT.git ```
NOTE: Depending on the development stage of the project, there can be multiple branches, look at the most recently updated branch in github and switch to it (if you cant figure this out, ask GPT) 
3) Install dependencies
```
sudo apt-get install -y build-essential cmake pkg-config \
libwayland-dev wayland-protocols \
libcairo2-dev libpango1.0-dev libfontconfig1-dev

sudo apt-get install -y \
libwayland-dev \
libwayland-client0 \
libwayland-cursor0 \
libwayland-egl1 \
wayland-protocols \
libcairo2-dev \
pkg-config \
cmake \
g++

sudo apt install libgtk-4-dev build-essential
```
follow these instructions on installing the layer shell
https://github.com/wmww/gtk4-layer-shell

4) Build from the cmake file
```
cmake .
```
5) Build the project (run every time you update the code)
```
cmake -j14
```

## Using
1) Start the hack (after the game is started, this can be done at any point i.e. in the menu or on the ship)
```
sudo ./danSOT
```
2) Start the overlay
One monitor:
```
./OverlayApp
```
Multiple Monitors (NOTE: In my experience, monitor numbers stay the same and are numbered left to right):
```
./OverlayApp (monitor number here)

Examples:
./OverlayApp 0
./OverlayApp 1
./OverlayApp 2
```

# Old info
# DanSOT
Run: cmake .

## Linux
Please not this cheat only works on Linux. Use Wayland for the overlay not being detected. Install steam through flatpak so reading memory will not be detedcted.

NOTE: When overlay is present your mouse will not be captured by the game (ie game is borderless so mouse does not stay in the game)
You will have issue with the mouse clicking onto other applications,

How you fix this:
 - This is seemingly kinda random but heres the situation:
 - One some apps and the desltop, the mouse wont be captured by the game. On other apps, it will be captuered.
 - One of these apps is vscode.
 - Minimize every app exept for vscode and the game.
 - expand vscode to fullscreen on all montiros that arent the game.

OR

Use the script provided that changes the orientation of the montitors to make them far away from each other. Also solving the issue.

Launching & manual input:
- Needs to be launched through a script to make the processes on different XWayland instances. (helps with detection)



sudo apt-get install -y build-essential cmake pkg-config \
libwayland-dev wayland-protocols \
libcairo2-dev libpango1.0-dev libfontconfig1-dev

sudo apt-get install -y \
libwayland-dev \
libwayland-client0 \
libwayland-cursor0 \
libwayland-egl1 \
wayland-protocols \
libwlr-protocols-dev \
libcairo2-dev \
pkg-config \
cmake \
g++

# Building
cmake .

make -j14

#Running
sudo su
./danSOT

(in a different terminal)
./OverlayApp
(or if you need to select the monitor its on, type in number, range starts at 0, just do trial and error to figure it out)
./OverlayApp 1