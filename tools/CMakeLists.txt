add_executable(debconf-kde-helper main.cpp)
target_link_libraries(debconf-kde-helper
    KF5::CoreAddons
    KF5::I18n
    DebconfKDE::Main
)

install(TARGETS debconf-kde-helper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
