# GlueAddon
Author: Riordan (Upakai)
[Twitter](https://twitter.com/RiordanCallil)
[Ko-Fi](https://ko-fi.com/upakai)

This is a Godot 4.x addon for simulated pre-fractured structural destruction.

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/mpDvrJ0uqWQ/0.jpg)](https://youtu.be/mpDvrJ0uqWQ)



-------

### Features:
* Performant code up to ~300 node structures
* Anchor points
* Small readable codebase

### Future:
* Perform fracturing on a thread to remove lag and scale to larger graphs

### Installing
Downloading this repo and copy the ```addons/glue``` folder into your Godot 4.x project ```addons``` folder. Enabled in your project settings and use.

### Building
This addon comes with libraries for windows using the Godot 4.x master branch API. If you need to build your own follow the guide found inside the ```glue/``` directory.

### Testing
I've added the glue_test.tscn to the scenes/ folder. I used this scene to make the YT vid, plus a massive wall which shows the current limits of the system.
<br>The controls are:
<br>W,A,S,D = fly around
<br>Left mouse button = Detach shape from body
<br>Right mouse button = Launch grenade
<br>R = Restart scene
<br>Enter = Toggle slow motion