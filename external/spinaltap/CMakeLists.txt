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

        include/spinaltap/iomux/registers.hpp
        include/spinaltap/iomux/iomux.hpp

        include/spinaltap/gpio/gpio.hpp
        include/spinaltap/gpio/registers.hpp

        include/spinaltap/spi/spi.hpp
        include/spinaltap/spi/registers.hpp
    PRIVATE
        src/iso7816.cpp
        src/spinaltap.cpp
        src/pwm.cpp
        src/iomux.cpp
        src/gpio.cpp
        src/spi.cpp
)

target_link_libraries(spinaltap
    Microsoft.GSL::GSL
    spdlog::spdlog
    libusb++::libusb++
)
