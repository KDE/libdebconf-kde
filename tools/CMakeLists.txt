kde4_add_executable(debkonf main.cpp)
target_link_libraries(debkonf
    debconf-kde ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} ${QT_QTCORE_LIBRARY}
)
