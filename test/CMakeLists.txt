
FILE(GLOB SRCS *.cpp ../threadutil/*.cpp)
FILE(GLOB HDRS *.h ../threadutil/*.h)
SOURCE_GROUP("" FILES ${SRCS} ${HDRS})

INCLUDE_DIRECTORIES(
)

ADD_EXECUTABLE(threadutiltest
	${SRCS}
	${HDRS}
)

TARGET_LINK_LIBRARIES(threadutiltest
)
