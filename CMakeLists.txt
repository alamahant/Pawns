cmake_minimum_required(VERSION 3.16)

project(Pawns VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#flatpak
option(FLATPAK_BUILD "Build for Flathub/Flatpak" OFF)
if(FLATPAK_BUILD)
    add_compile_definitions(FLATPAK_BUILD)
endif()





find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Multimedia)

if(FLATPAK_BUILD)
    include_directories(/app/usr/include/miniupnpc)
    link_directories(/app/usr/lib)
    set(CMAKE_INSTALL_RPATH "/app/usr/lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
else()
    include_directories(/usr/include/miniupnpc)
endif()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Pawns
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
        chesspiece.h chesspiece.cpp
        chessboard.h chessboard.cpp
        Constants.h
        stockfishengine.h stockfishengine.cpp
        scenariobuilder.cpp scenariobuilder.h
        chatmanager.cpp chatmanager.h networkmanager.cpp networkmanager.h
        contactdialog.cpp contactdialog.h
        Constants.cpp
        newgamedialog.cpp newgamedialog.h
        upnpmanagerdialog.cpp upnpmanagerdialog.h
        replaydialog.h replaydiaolog.cpp
        sounds.h

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Pawns APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Pawns SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Pawns
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Pawns PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::Network Qt6::Multimedia miniupnpc)




# Copy piecesets directory to build output
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/piecesets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/piecesets"
)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Pawns)
endif()
set_target_properties(Pawns PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Pawns
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Pawns)
endif()
