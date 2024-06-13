# GlueAddon

This is a Godot 3.x addon for simulated pre-fractured structural destruction.

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/z3tJwcHUo0o/0.jpg)](https://www.youtube.com/watch?v=z3tJwcHUo0o)

---

### Features:

* Performant code up to ~300 node structures
* Anchor points
* Small readable codebase

### Future:

* Perform fracturing on a thread to remove lag and scale to larger graphs

### Installing

Downloading this repo and copy the ``addons/glue`` folder into your Godot 3.x project ``addons`` folder. Enabled in your project settings and use.

### Building

This addon comes with libraries for windows using the Godot 3.x master branch API. If you need to build your own follow the guide found inside the ``glue/`` directory.

### Testing

I've added the glue_test.tscn to the scenes/ folder. I used this scene to make the YT vid, plus a massive wall which shows the current limits of the system.
The controls are:
W,A,S,D = fly around
Left mouse button = Detach shape from body
Right mouse button = Launch grenade
R = Restart scene
Enter = Toggle slow motion

### Issues

* After you glue and produce a graph resource it must be saved to disk. A graph resource saved inside the scene seems to break stuff.

## Author

Glue is developed and maintained by [Riordan Callil](https://twitter.com/RiordanCallil)
[Ko-Fi](https://ko-fi.com/upakai)
