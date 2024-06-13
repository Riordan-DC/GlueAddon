# Glue for Godot 4
Glue is a Godot 4.x addon for simulated pre-fractured structural destruction. This branch only works with Godot >= 4.2, earlier versions may not work. <br>
For a 3.x version, use master branch. Note: 3.x requires Bullet Physics engine to function.

## Features:
* Uses GDExtension and C++ to support ~300 node structures
* Anchor points
* Small readable codebase
* Tested with Jolt physics and Godot's physics. <br>


## Video
I made this short video to explain the inspiration for this addon and how it was made. If you want to know more, read the code, the codebase is small.

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/z3tJwcHUo0o/0.jpg)](https://www.youtube.com/watch?v=z3tJwcHUo0o)
-------



## Future:
* Perform fracturing on a thread to remove lag and scale to larger graphs

## Installing
Downloading this repo and copy the ```addons/glue``` folder into your Godot 4.x project ```addons``` folder. Enabled in your project settings and use.

## Building
This addon comes with libraries for windows using the Godot 4.x master branch API. If you need to build your own follow the guide found inside the ```glue/``` directory.

## Testing
I've added the glue_test.tscn to the scenes/ folder. I used this scene to make the YT vid, plus a massive wall which shows the current limits of the system.
<br>The controls are:
<br>W,A,S,D = fly around
<br>Left mouse button = Detach shape from body
<br>Right mouse button = Launch grenade
<br>R = Restart scene
<br>Enter = Toggle slow motion

## Issues
* After you glue and produce a graph resource it must be saved to disk. A graph resource saved inside the scene seems to break stuff.

## TODO
- Refactor CPP to consider bullet_backend and jolt backend. There are slight differences. 

## Author
Glue is developed and maintained by [Riordan Callil](https://twitter.com/RiordanCallil)
[Ko-Fi](https://ko-fi.com/upakai)