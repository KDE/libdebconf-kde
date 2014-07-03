add_executable(debconf-kde-helper main.cpp)
target_link_libraries(debconf-kde-helper
    debconf-kde
    KF5::CoreAddons
    KF5::I18n
    DebconfKDE
)

install(TARGETS debconf-kde-helper ${INSTALL_TARGETS_DEFAULT_ARGS})
