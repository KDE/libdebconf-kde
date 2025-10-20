add_executable(debconf-kde-helper main.cpp)
target_link_libraries(debconf-kde-helper
    KF6::CoreAddons
    KF6::I18n
    DebconfKDE::Main
)

# Clients should expect the helper in /usr/libexec when installed system-wide,
# and most do. However for backwards-compatibility we install it in /usr/bin
# even though it isn't a tool users usually would want to run.
install(TARGETS debconf-kde-helper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
