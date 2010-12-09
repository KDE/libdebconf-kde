kde4_add_executable(debconf-kde-helper main.cpp)
target_link_libraries(debconf-kde-helper
    debconf-kde ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} ${QT_QTCORE_LIBRARY}
)

install(TARGETS debconf-kde-helper ${INSTALL_TARGETS_DEFAULT_ARGS})