
qt_wrap_cpp(FILES_MOC)

ADD_EXECUTABLE(WhiteBalance
    WhiteBalanceMain.cpp
    ${FILES_MOC})

TARGET_LINK_LIBRARIES(WhiteBalance Qt${QT_MAJOR}::Core Qt${QT_MAJOR}::Gui Qt${QT_MAJOR}::Widgets)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(WhiteBalance ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(WhiteBalance -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(WhiteBalance ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
