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
    # Correct Flatpak paths for miniupnpc
    include_directories(/app/include/miniupnpc)
    link_directories(/app/lib)
    set(CMAKE_INSTALL_RPATH "/app/lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
else()
    include_directories(/usr/include/miniupnpc)
endif()

# NOTE COPY THE PIECESETS DIRECTORY INSIDE THE FLATPAK

if(FLATPAK_BUILD)
    # ============================================
      # 1. MINIUPNPC
      # ============================================
      set(MINIUPNPC_SOURCE_DIR "${CMAKE_SOURCE_DIR}/miniupnpc_src")

      if(EXISTS "${MINIUPNPC_SOURCE_DIR}/Makefile")
          message(STATUS "Building miniupnpc...")

          # Build
          execute_process(
              COMMAND make -j${CMAKE_SYSTEM_PROCESSOR_COUNT}
              WORKING_DIRECTORY ${MINIUPNPC_SOURCE_DIR}
              RESULT_VARIABLE MINIUPNPC_MAKE_RESULT
          )

          if(MINIUPNPC_MAKE_RESULT)
              message(FATAL_ERROR "miniupnpc build failed")
          endif()

          # Install
          execute_process(
              COMMAND make install PREFIX=/app
              WORKING_DIRECTORY ${MINIUPNPC_SOURCE_DIR}
              RESULT_VARIABLE MINIUPNPC_INSTALL_RESULT
          )

          if(MINIUPNPC_INSTALL_RESULT)
              message(FATAL_ERROR "miniupnpc install failed")
          endif()
      endif()

      # ============================================
      # 2. STOCKFISH
      # ============================================
      set(STOCKFISH_SOURCE_DIR "${CMAKE_SOURCE_DIR}/stockfish_src/src")

      if(EXISTS "${STOCKFISH_SOURCE_DIR}/Makefile")
          message(STATUS "Building Stockfish...")

          # Detect architecture
          if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
              set(STOCKFISH_ARCH "x86-64")
          elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
              set(STOCKFISH_ARCH "armv8")
          else()
              set(STOCKFISH_ARCH "x86-64")
          endif()

          message(STATUS "Stockfish ARCH: ${STOCKFISH_ARCH}")

          # Build
          execute_process(
              COMMAND make -j${CMAKE_SYSTEM_PROCESSOR_COUNT} build ARCH=${STOCKFISH_ARCH}
              WORKING_DIRECTORY ${STOCKFISH_SOURCE_DIR}
              RESULT_VARIABLE STOCKFISH_MAKE_RESULT
          )

          if(STOCKFISH_MAKE_RESULT)
              message(FATAL_ERROR "Stockfish build failed")
          endif()

          # Install
          execute_process(
              COMMAND make install BINDIR=${CMAKE_INSTALL_PREFIX}/bin
              WORKING_DIRECTORY ${STOCKFISH_SOURCE_DIR}
              RESULT_VARIABLE STOCKFISH_INSTALL_RESULT
          )

          if(STOCKFISH_INSTALL_RESULT)
              message(FATAL_ERROR "Stockfish install failed")
          endif()
      endif()

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

if(FLATPAK_BUILD)
    # Install desktop file
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Pawns.desktop"
            DESTINATION "share/applications")

    # Install metainfo file
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Pawns.metainfo.xml"
            DESTINATION "share/metainfo")

    # Install app icon from root directory
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Pawns.png"
            DESTINATION "share/icons/hicolor/512x512/apps"
            RENAME "io.github.alamahant.Pawns.png")

    # Install icons from favicon directory
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon/android-chrome-192x192.png"
            DESTINATION "share/icons/hicolor/192x192/apps"
            RENAME "io.github.alamahant.Pawns.png")

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon/favicon-16x16.png"
            DESTINATION "share/icons/hicolor/16x16/apps"
            RENAME "io.github.alamahant.Pawns.png")

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon/favicon-32x32.png"
            DESTINATION "share/icons/hicolor/32x32/apps"
            RENAME "io.github.alamahant.Pawns.png")

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon/apple-touch-icon.png"
            DESTINATION "share/icons/hicolor/180x180/apps"
            RENAME "io.github.alamahant.Pawns.png")

        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/piecesets"
                        DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Pawns)
endif()
