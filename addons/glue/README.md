README
On windows you may need to run scons using the x64 Native Tools Command Prompt for VS 2019 if the Windows Cl compiler is not exposed to your command prompt.

## Compiling
1. Download the godot-cpp bindings. Use the 3.x branch. If added to a git managed project use git submodule add https://github.com/godotengine/godot-cpp.git -b 3.x. If it doesnt work remove the -b 3.x and then later in .gitmodules add the branch = 3.x entry.
2. Compile godot-cpp with the command (inside godot-cpp folder) scons platform=windows generate_bindings=yes. You can omit the platform and it will be inferred from your current machine.
3. Compile the addon with the command scons platform=windows godot_headers=<path to where you downloaded godot-cpp>

## Issues
1. When using a GDNative Script class in a tool the GDNative library resource cannot be set to reloadable and this will often crash the editor. 
References: https://github.com/godotengine/godot/issues/19686
2. 

Examples:
scons platform=windows godot_headers="../../../godot-cpp" target=debug