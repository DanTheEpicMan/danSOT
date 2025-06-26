# DanSOT

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