add_executable(debconf-kde-helper main.cpp)
target_link_libraries(debconf-kde-helper
    KF6::CoreAddons
    KF6::I18n
    DebconfKDE::Main
)

# Clients expect the helper in /usr/libexec when installed system-wide, so we
# deliberately do not use KDE_INSTALL_LIBEXECDIR (which may be multiarch).
# For historic reasons and backwards-compatibility, we also provide a symlink
# in /usr/bin.
install(TARGETS debconf-kde-helper DESTINATION libexec)

file(RELATIVE_PATH _helper_rel_path "${KDE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_PREFIX}/libexec/debconf-kde-helper")
install(CODE "
    file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${KDE_INSTALL_FULL_BINDIR}\")
    file(CREATE_LINK \"${_helper_rel_path}\" \"\$ENV{DESTDIR}${KDE_INSTALL_FULL_BINDIR}/debconf-kde-helper\" SYMBOLIC)
    message(STATUS \"Installing: \$ENV{DESTDIR}${KDE_INSTALL_FULL_BINDIR}/debconf-kde-helper -> ${_helper_rel_path}\")
")
