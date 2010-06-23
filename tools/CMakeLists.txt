kde4_add_executable(debkonf main.cpp)
target_link_libraries(debkonf
    debconf-kde
)
