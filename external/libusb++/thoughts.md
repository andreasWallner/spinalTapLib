usb_error is derived from std::exception and not std::runtime_error as it typically needs different handling than
std::runtime_error (but can still be cought by catching std::exception), see also core guideline E.14
(https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#e14-use-purpose-designed-user-defined-types-as-exceptions-not-built-in-types)

Generation of Instances: while it look nice in the first place to have members that generate objects,
like device.open(), device_handle.interface() OTOH these preclude e.g. having dynamically allocated objects, objects in shared_ptr, etc.

network-ts has constructors that do not initialize the object under question, so that one can e.g. have a class
member that is not yet initialized - typically the (understandable) argument is that you want classes to always
be in a usable state - but in this case we have anyhow a few things that can happen so that we loose control of
a device, e.g. it getting unplugged which makes the object unusable?!

non-throwing variants & "incomplete objects" are needed in environments that can't handle exceptions...
do we want to support those? we anyhow have "incomplete objects" since we allow moves from the objects...

throwing variants of functions have to be implemented on top of non-throwing version, not the other way round - 
otherwise non of the functions can be used in an environment which has exceptions disabled

