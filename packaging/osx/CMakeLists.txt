# packaging/osx/CMakeLists.txt
# Produces ScanTailor.app bundle (via macdeployqt) and an optional .dmg installer.

FIND_PROGRAM(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")
IF(NOT MACDEPLOYQT_EXECUTABLE)
    MESSAGE(FATAL_ERROR "macdeployqt not found. Set QT_BIN_DIR or add Qt5 bin dir to PATH.")
ENDIF()

# 'macos_bundle' runs automatically after every build.
# It calls macdeployqt to copy Qt frameworks and plugins into the .app bundle.
ADD_CUSTOM_TARGET(
    macos_bundle ALL
    COMMAND "${MACDEPLOYQT_EXECUTABLE}"
        "$<TARGET_BUNDLE_DIR:scantailor>"
        -always-overwrite
    DEPENDS scantailor compile_translations
    COMMENT "Running macdeployqt to bundle Qt frameworks..."
    VERBATIM
)

# 'dmg' is an opt-in target: cmake --build . --target dmg
# It creates a drag-to-Applications disk image named ScanTailor-<VERSION>.dmg.
ADD_CUSTOM_TARGET(
    dmg
    COMMAND "${MACDEPLOYQT_EXECUTABLE}"
        "$<TARGET_BUNDLE_DIR:scantailor>"
        -dmg
        -always-overwrite
    COMMAND "${CMAKE_COMMAND}" -E rename
        "${CMAKE_BINARY_DIR}/ScanTailor.dmg"
        "${CMAKE_BINARY_DIR}/ScanTailor-${VERSION}.dmg"
    DEPENDS scantailor compile_translations
    COMMENT "Building ${CMAKE_BINARY_DIR}/ScanTailor-${VERSION}.dmg ..."
    VERBATIM
)
