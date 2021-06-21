add_library(spinaltap STATIC "")
add_library(spinaltap::spinaltap ALIAS spinaltap)
target_include_directories(spinaltap PUBLIC include)

target_sources(spinaltap
    PUBLIC
        include/spinaltap.hpp
        include/spinaltap/logging.hpp
        include/spinaltap/iso7816/iso7816.hpp
        include/spinaltap/iso7816/registers.hpp

        include/spinaltap/pwm/pwm.hpp
        include/spinaltap/pwm/registers.hpp
    PRIVATE
        src/iso7816.cpp
        src/spinaltap.cpp
        src/pwm.cpp
)

target_link_libraries(spinaltap
    Microsoft.GSL::GSL
    spdlog::spdlog
    libusb++::libusb++
)
