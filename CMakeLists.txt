
FILE(GLOB SRCS *.cpp)
FILE(GLOB HDRS *.h)

ADD_EXECUTABLE(gdscroll
	${SRCS}
	${HDRS}
	)

target_link_libraries(gdscroll
	libgdemu
	)
