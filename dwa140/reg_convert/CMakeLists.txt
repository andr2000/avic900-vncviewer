cmake_minimum_required(VERSION 2.8)

set(BINARY_NAME "reg_convert")

set(SOURCES
	main.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -Wall")

add_executable(${BINARY_NAME}
	${SOURCES}
)

install(TARGETS ${BINARY_NAME} DESTINATION bin)
