# Read Me that explains the basics of the code to any developers modifying the source code.

## Overlay
Contained in the the directory `overlay/` is the source code for the overlay. 

The overlay is a "double buffer". This prevents flickering when the overlay is updated.
Basically the way it works is that when you are drawing stuff like ESP, you draw it to a hidden buffer. 
Then when you are done drawing, you copy the hidden buffer to the screen. This way the user never sees the lines being drawn (flickering).

### Functions
```
At the begining of the ESP loop, use the function

initialize("game_name"); // Call when program is started, to make the overlay

update(); // Call when you want to update the window size NOT content. Just resizes the window.

clearBackBuffer(); // Call on the start of the ESP loop to clear the back buffer and allow you to draw new stuff.

drawLine(...); drawText(...); drawBox(...); drawCircle(...); // Call these functions to draw stuff to the back buffer.

swapBuffers(); // Call this function to copy the back buffer to the screen. This is what prevents flickering. Also now everything you wrote to the backbuffer will be displayed in the front buffer.
// Start the loop again by calling clearBackBuffer() again to restart the cycle.
```

## Memory
(/memory)
Basically this class has a bunch of examples throughout the code. Basically the imporant thing for this is to set the ProcessId and BaseAddress variables before you try to read memeory as this files uses them