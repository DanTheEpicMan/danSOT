# DanSOT
SOT cheat for linux, below are tested install instructions for Kubuntu
## Setup
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
```
Examples:
```
./OverlayApp 0
./OverlayApp 1
./OverlayApp 2
```

## Possible Features
Some of these will probobly never be implemented because I am lazy

### Cannon Aimbot:
| Name            | Importance | Description                                                     | Implementation Info                                                                                                      |
|-----------------|------------|-----------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------|
| Auto Aim        | 3/5        | Automatically moves mouse to place for shooting                 | Not very hard to do yourself. Would be difficult as would rely on wasd for quick cannon movement (instead of just mouse) |
| More Ammo types | 3/5        | Would allow for ammo like scattershot and spears                | I dont know where the info for this is stored in game. Difficult.                                                        |
| Wave Prediction | N/A        | Incorporates waves into cannon prediction                       | Not possible on external I think.                                                                                        |
| Smart Targeting | 2/5        | Tells you were best to aim                                      | Would remove all decision making and is easy if your not braindead                                                       |
| &#9745;Player Hit      | 5/5        | Tells you were to aim to hit a player                           | Not to difficult to implement and would be good for combat                                                               |
| &#9745;HUD             | 4/5        | Gives info like, other ship damage, time to hit, inventory info | Easy to implement and would be good for decision making                                                                  |
### Player Aimbot:
| Name            | Importance | Description                                                                                               | Implementation Info                                                                |
|-----------------|------------|-----------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------|
| Auto Shoot      | 3/5        | Automatically shoots for you when over target                                                             | Already Half implemented                                                           |
| Auto Quickscope | 2/5        | Automatically quickscopes after shot                                                                      | Not very important and very circumstantial                                         |
| Throwing Knives | 3/5        | Allows aimbot to work with throwing knives                                                                | Dont know how to get info from the game to implement (unlike all other guns)       |
| &#9745;Ladder Shot     | 5/5        | Allows for aimbot to aim higher on a person when there on a ladder (specifically whe your guarding ladder) | Not to difficult, just need Local Players pitch, when its low, allow to aim higher |

### ESP:
| Name                      | Importance | Description                                           | Implementation Info                                                                          |
|---------------------------|------------|-------------------------------------------------------|----------------------------------------------------------------------------------------------|
| Radar Customization       | 3/5        | Allows user to move radar in settings.                | Very simple, just move local variables into global ones and maybe make a bool if to (center) |
| Local Radar Customization | 3/5        | Allows for customization of local (ie per ship) radar | Simple, just move local variables into class                                                 |
| Item ESP                  | 4/5        | Working ESP for loot and items                        | Mostly done, just cannot read accurate name                                                  |
| Player + Ship Names       | 4/5        | Displays player ans ship name                         | Already in the works                                                                         |
| Ship Trajectory           | 3/5        | Shows where ship will move                            | Would require overhaul of Overlay                                                            |

## Code Issues:
- Code to get player velocity and the way it determines if a player is on a ship is scuffed (and will fail when a boat is very close to land and a player is on island so its still rather reliable)
- CannonAimbot::GetShipComponents(...) does so by proximity with is lazy and suboptimal, can lead to incorrect aim when ships are very close. (Possible yet lazy solution is to do it based on the closest ship rather than all ships within 40m)
- 

___

# Old info just in case
 DanSOT
Run: cmake .

 Linux
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

 Building
cmake .

make -j14

#Running
sudo su
./danSOT

(in a different terminal)
./OverlayApp
(or if you need to select the monitor its on, type in number, range starts at 0, just do trial and error to figure it out)
./OverlayApp 1