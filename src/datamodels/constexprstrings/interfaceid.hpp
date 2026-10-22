#pragma once

#include "constexprstringcat.hpp"

// "com.<PROJECT_NAME>.<Component>", baked at compile time into .rodata.
template<FixedString Component>
inline constexpr auto interface_iid =
    FixedString{"com."} + FixedString{PROJECT_NAME} + FixedString{"."} + Component;