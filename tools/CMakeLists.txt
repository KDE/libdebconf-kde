kde4_add_executable(debkonf main.cpp)
target_link_libraries(debkonf
    debconf-kde
)

install(TARGETS debkonf ${INSTALL_TARGETS_DEFAULT_ARGS})
