set(libdebconf_SRCS
    debconf.cpp
    DebconfElement.cpp
    DebconfBoolean.cpp
    DebconfError.cpp
    DebconfMultiselect.cpp
    DebconfNote.cpp
    DebconfPassword.cpp
    DebconfProgress.cpp
    DebconfSelect.cpp
    DebconfString.cpp
    DebconfText.cpp
    DebconfGui.cpp
)

kde4_add_ui_files(libdebconf_SRCS
    DebconfBoolean.ui
    DebconfError.ui
    DebconfMultiselect.ui
    DebconfNote.ui
    DebconfPassword.ui
    DebconfProgress.ui
    DebconfSelect.ui
    DebconfString.ui
    DebconfText.ui
    DebconfGui.ui
)

kde4_add_library(debconf-kde SHARED ${libdebconf_SRCS})
target_link_libraries(debconf-kde
    ${QT_QTCORE_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${KDE4_KDEUI_LIBS}
)

set_target_properties(debconf-kde PROPERTIES
                      VERSION ${DEBCONF_LIB_VERSION}
                      SOVERSION ${DEBCONF_LIB_SOVERSION}
)

install(TARGETS debconf-kde ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES DebconfGui.h
    DESTINATION ${INCLUDE_INSTALL_DIR})
