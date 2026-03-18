INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	main.cpp TestContentSpanFinder.cpp
	TestSmartFilenameOrdering.cpp
	TestMatrixCalc.cpp
	TestFilterParams.cpp
	TestSettingsThreadSafety.cpp
	../ContentSpanFinder.cpp ../ContentSpanFinder.h
	../SmartFilenameOrdering.cpp ../SmartFilenameOrdering.h
)

SOURCE_GROUP("Sources" FILES ${sources})

ADD_EXECUTABLE(tests ${sources})
TARGET_LINK_LIBRARIES(tests PRIVATE
	fix_orientation page_split deskew select_content page_layout output
	stcore dewarping zones interaction imageproc math foundation
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	${Boost_PRG_EXECUTION_MONITOR_LIBRARY}
	Qt5::Widgets Qt5::Xml Qt5::Core
	${EXTRA_LIBS}
)
