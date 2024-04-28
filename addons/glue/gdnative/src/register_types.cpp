#include "ggraph.h"
#include "baked_fracture.h"

/** GDNative Initialize **/
extern "C" void GDN_EXPORT glue_gdnative_init(godot_gdnative_init_options *o) {
	godot::Godot::gdnative_init(o);
}

/** GDNative Terminate **/
extern "C" void GDN_EXPORT glue_gdnative_terminate(godot_gdnative_terminate_options *o) {
	godot::Godot::gdnative_terminate(o);
}

/** NativeScript Initialize **/
extern "C" void GDN_EXPORT glue_nativescript_init(void *handle) {
	godot::Godot::nativescript_init(handle);

	godot::register_class<Ggraph>();
	godot::register_class<BakedFracture>();
}
