#!python
import os, subprocess

opts = Variables([], ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Define our options
opts.Add(EnumVariable('target', "Compilation target", 'debug', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('platform', "Compilation platform", '', ['', 'windows', 'x11', 'linux', 'osx', 'ios']))
opts.Add(EnumVariable('p', "Compilation target, alias for 'platform'", '', ['', 'windows', 'x11', 'linux', 'osx', 'ios']))
opts.Add("arch", "Platform-dependent architecture (arm/arm64/x86/x64/mips/...)", "")
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", 'no'))
opts.Add(PathVariable('target_path', 'The path where the lib is installed.', 'gdnative/'))
opts.Add(PathVariable('godot_headers', 'The path godot headers can be found.', '../'))
opts.Add(PathVariable('target_name', 'The library name.', 'ggraph', PathVariable.PathAccept))
opts.Add('IPHONEPATH', "Path to iPhone toolchain", "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain")
opts.Add("IPHONESDK", "Path to the iPhone SDK", "")

# Updates the environment with the option variables.
opts.Update(env)

# Local dependency paths, adapt them to your setup
cpp_bindings_path = env['godot_headers'] + '/'
godot_headers_path = env['godot_headers'] + "/godot-headers/"
cpp_library = "libgodot-cpp"
print("CPP BINDINGS PATH: " + env['godot_headers'])
print("GODOT_HEADERS PATH: " + godot_headers_path)

# only support 64 at this time..
bits = 64

# Process some arguments
if env['use_llvm']:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'

if env['p'] != '':
    env['platform'] = env['p']

if env['platform'] == '':
    print("No valid target platform selected.")
    quit();

# Check our platform specifics
if env['platform'] == "osx":
    env['target_path'] += 'osx/'
    cpp_library += '.osx'
    print("Building for macOS 10.15+")

    if env['target'] in ('debug', 'd'):
        print("debug build...")
        env.Append(CCFLAGS = ['-g','-O2', '-std=c++17'])

        #env.Append(ASFLAGS=["-arch", "arm64", "-mmacosx-version-min=10.15"])
        #env.Append(CCFLAGS=["-arch", "arm64", "-mmacosx-version-min=10.15"])
        #env.Append(LINKFLAGS=["-arch", "arm64", "-mmacosx-version-min=10.15"])
        #env.Append(CCFLAGS = ['-g','-O2', '-arch', 'x86_64', '-std=c++17'])
    else:
        print("build release...")
        env.Append(CCFLAGS = ['-g','-O3', '-arch', 'x86_64', '-std=c++17'])
    
    if env["arch"] == "arm64":
        print("platform arm64...")
        env.Append(CCFLAGS = ['-arch', 'arm64'])
        env.Append(LINKFLAGS = ['-arch', 'arm64'])
    else:
        print("platform x86_64...")
        env.Append(CCFLAGS = ['-arch', 'x86_64'])
        env.Append(LINKFLAGS = ['-arch', 'x86_64'])


# iOS
if env['platform'] == "ios":
    env['target_path'] += 'ios/'
    cpp_library += '.ios'

    if env['arch'] == 'arm64':
        sdk_name = 'iphoneos'
        env.Append(CCFLAGS=['-miphoneos-version-min=10.0'])
    else:
        sdk_name = 'iphonesimulator'
        env.Append(CCFLAGS=['-mios-simulator-version-min=10.0'])

    try:
        sdk_path = subprocess.check_output(['xcrun', '--sdk', sdk_name, '--show-sdk-path']).strip().decode()
    except (subprocess.CalledProcessError, OSError):
        raise ValueError("Failed to find SDK path while running xcrun --sdk {} --show-sdk-path.".format(sdk_name))

    compiler_path = env['IPHONEPATH'] + '/usr/bin/'
    env['ENV']['PATH'] = env['IPHONEPATH'] + "/Developer/usr/bin/:" + env['ENV']['PATH']

    env['CC'] = compiler_path + 'clang'
    env['CXX'] = compiler_path + 'clang++'
    env['AR'] = compiler_path + 'ar'
    env['RANLIB'] = compiler_path + 'ranlib'

    env.Append(CCFLAGS=['-g', '-std=c++14', '-arch', env['arch'], '-isysroot', sdk_path])
    env.Append(LINKFLAGS=[
        '-arch',
        env['arch'],
        '-framework',
        'Cocoa',
        '-Wl,-undefined,dynamic_lookup',
        '-isysroot', sdk_path,
        '-F' + sdk_path
    ])

    if env['target'] == 'debug':
        env.Append(CCFLAGS=['-Og', '-g'])
    elif env['target'] == 'release':
        env.Append(CCFLAGS=['-O3'])
    
    env.Append(CPPDEFINES=["NEED_LONG_INT"])
    env.Append(CPPDEFINES=["LIBYUV_DISABLE_NEON"])

    # Temp fix for ABS/MAX/MIN macros in iPhone SDK blocking compilation
    env.Append(CCFLAGS=["-Wno-ambiguous-macro"])

    """
    # IphoneSDK
    if env['IPHONESDK'] == "":
        sdk_name = "iphoneos"
        try:
            env['IPHONESDK'] = subprocess.check_output(["xcrun", "--sdk", sdk_name, "--show-sdk-path"]).strip().decode()
        except (subprocess.CalledProcessError, OSError):
            print("Failed to find SDK version while running xcrun --sdk {} --show-sdk-version.".format(sdk_name))
    
    print(f"Using iOS SDK at: {env['IPHONESDK']}")

    # Setup iphoneos SDK compilers
    env["ENV"]["PATH"] = env["IPHONEPATH"] + "/Developer/usr/bin/:" + env["ENV"]["PATH"]
    compiler_path = "$IPHONEPATH/usr/bin/${ios_triple}"
    s_compiler_path = "$IPHONEPATH/Developer/usr/bin/"
    env["CC"] = compiler_path + "clang"
    env["CXX"] = compiler_path + "clang++"
    env["S_compiler"] = s_compiler_path + "gcc"
    env["AR"] = compiler_path + "ar"
    env["RANLIB"] = compiler_path + "ranlib"
    
    env.Append(ASFLAGS=["-miphoneos-version-min=10.0"])
    env.Append(CCFLAGS=["-miphoneos-version-min=10.0"])
    env.Append(LINKFLAGS=["-miphoneos-version-min=10.0"])

    # LTO (Link Time Optimization)
    env.Append(CCFLAGS=["-flto"])
    env.Append(LINKFLAGS=["-flto"])
    
    # Temp fix for ABS/MAX/MIN macros in iPhone SDK blocking compilation
    env.Append(CCFLAGS=["-Wno-ambiguous-macro"])

    if env["arch"] == "arm":
        env.Append(LINKFLAGS=["-arch", "armv7", "-Wl,-dead_strip"])
    if env["arch"] == "arm64":
        env.Append(LINKFLAGS=["-arch", "arm64", "-Wl,-dead_strip"])

    env.Append(
        LINKFLAGS=[
            "-isysroot",
            "$IPHONESDK",
        ]
    )

    env.Append(
        CPPPATH=[
           "$IPHONESDK/usr/include/",
           "$IPHONESDK/System/Library/Frameworks/OpenGLES.framework/Headers",
           "$IPHONESDK/System/Library/Frameworks/AudioUnit.framework/Headers",
        ]
    )

    #env["ENV"]["CODESIGN_ALLOCATE"] = "/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/codesign_allocate"

    env.Append(CPPDEFINES=["IPHONE_ENABLED", "UNIX_ENABLED", "GLES_ENABLED", "COREAUDIO_ENABLED"])

    if env['target'] in ('debug', 'd'):
        if env["arch"] == "arm64":
            print("Building for iOS 10+, platform arm64.")
            env.Append(CCFLAGS = ['-g','-O2', '-arch', 'arm64', '-std=c++17'])
            env.Append(LINKFLAGS = ['-arch', 'arm64'])

            # Stolen from Godot platforms/iphone/detect.py
            env.Append(
                CCFLAGS="-fobjc-arc -arch arm64 -fmessage-length=0 -fno-strict-aliasing -fdiagnostics-print-source-range-info -fdiagnostics-show-category=id -fdiagnostics-parseable-fixits -fpascal-strings -fblocks -fvisibility=hidden -MMD -MT dependencies -isysroot $IPHONESDK".split()
            )
            env.Append(ASFLAGS=["-arch", "arm64"])
            env.Append(CPPDEFINES=["NEED_LONG_INT"])
            env.Append(CPPDEFINES=["LIBYUV_DISABLE_NEON"])
        else:
            env.Append(CCFLAGS = ['-g','-O2', '-arch', 'x86_64', '-std=c++17'])
            env.Append(LINKFLAGS = ['-arch', 'x86_64'])
    else:
        env.Append(CCFLAGS = ['-g','-O3', '-arch', 'x86_64', '-std=c++17'])
        env.Append(LINKFLAGS = ['-arch', 'x86_64'])

    """

elif env['platform'] in ('x11', 'linux'):
    env['target_path'] += 'x11/'
    cpp_library += '.linux'
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS = ['-fPIC', '-g3','-Og', '-std=c++17'])
    else:
        env.Append(CCFLAGS = ['-fPIC', '-g','-O3', '-std=c++17'])

elif env['platform'] == "windows":
    env['target_path'] += 'win64/'
    cpp_library += '.windows'
    # This makes sure to keep the session environment variables on windows,
    # that way you can run scons in a vs 2017 prompt and it will find all the required tools
    env.Append(ENV = os.environ)

    env.Append(CCFLAGS = ['-DWIN32', '-D_WIN32', '-D_WINDOWS', '-W3', '-GR', '-D_CRT_SECURE_NO_WARNINGS', '/std:c++17', '/MDd'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS = ['-EHsc', '-D_DEBUG', '-MDd'])
    else:
        env.Append(CCFLAGS = ['-O2', '-EHsc', '-DNDEBUG', '-MD'])

if env['target'] in ('debug', 'd'):
    cpp_library += '.debug'
else:
    cpp_library += '.release'

cpp_library += '.' + str(bits)

# make sure our binding library is properly includes
env.Append(CPPPATH=['.', godot_headers_path, cpp_bindings_path + 'include/', cpp_bindings_path + 'include/core/', cpp_bindings_path + 'include/gen/'])
env.Append(LIBPATH=[cpp_bindings_path + 'bin/'])
env.Append(LIBS=[cpp_library])

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=['gdnative/src/'])
sources = Glob('gdnative/src/*.cpp')

if env['platform'] == 'ios':
    library = env.StaticLibrary(target=env['target_path'] + env['target_name'] , source=sources)
else:
    library = env.SharedLibrary(target=env['target_path'] + env['target_name'] , source=sources)

Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
