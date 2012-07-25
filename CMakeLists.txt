
FILE(GLOB SRCS *.cpp)
FILE(GLOB HDRS *.h)

ADD_EXECUTABLE(gdjw
	${SRCS}
	${HDRS}
	)

target_link_libraries(gdjw
	libgdemu
	)
