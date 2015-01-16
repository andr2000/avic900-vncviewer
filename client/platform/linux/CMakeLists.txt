cmake_minimum_required(VERSION 2.8.11)

#get latest commit hash for this build
execute_process(
	COMMAND git rev-parse --short HEAD
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_COMMIT_ID
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

# compiler option
add_definitions("-DCOMMIT_ID=\"${GIT_COMMIT_ID}\"")

set(ARTIFACT vncviewer)

set(SOURCES
	client_gtk.cpp
	mutex_linux.cpp
	thread_linux.cpp
	vncviewer.cpp
	../../client.cpp
	../../config_storage.cpp
	../../thread.cpp
	../../inih/ini.c
	../../inih/cpp/INIReader.cpp
	../../../libvncserver/common/minilzo.c
	../../../libvncserver/libvncclient/cursor.c
	../../../libvncserver/libvncclient/rfbproto.c
	../../../libvncserver/libvncclient/sockets.c
	../../../libvncserver/libvncclient/tls_none.c
	../../../libvncserver/libvncclient/vncviewer.c
	../../zlib/adler32.c
	../../zlib/compress.c
	../../zlib/crc32.c
	../../zlib/deflate.c
	../../zlib/gzio.c
	../../zlib/infblock.c
	../../zlib/infcodes.c
	../../zlib/inffast.c
	../../zlib/inflate.c
	../../zlib/inftrees.c
	../../zlib/infutil.c
	../../zlib/trees.c
	../../zlib/uncompr.c
	../../zlib/zutil.c
	../../jpeg/jaricom.c
	../../jpeg/jcapimin.c
	../../jpeg/jcapistd.c
	../../jpeg/jcarith.c
	../../jpeg/jccoefct.c
	../../jpeg/jccolor.c
	../../jpeg/jcdctmgr.c
	../../jpeg/jchuff.c
	../../jpeg/jcinit.c
	../../jpeg/jcmainct.c
	../../jpeg/jcmarker.c
	../../jpeg/jcmaster.c
	../../jpeg/jcomapi.c
	../../jpeg/jcparam.c
	../../jpeg/jcprepct.c
	../../jpeg/jcsample.c
	../../jpeg/jctrans.c
	../../jpeg/jdapimin.c
	../../jpeg/jdapistd.c
	../../jpeg/jdarith.c
	../../jpeg/jdatadst.c
	../../jpeg/jdatasrc.c
	../../jpeg/jdcoefct.c
	../../jpeg/jdcolor.c
	../../jpeg/jddctmgr.c
	../../jpeg/jdhuff.c
	../../jpeg/jdinput.c
	../../jpeg/jdmainct.c
	../../jpeg/jdmarker.c
	../../jpeg/jdmaster.c
	../../jpeg/jdmerge.c
	../../jpeg/jdpostct.c
	../../jpeg/jdsample.c
	../../jpeg/jdtrans.c
	../../jpeg/jerror.c
	../../jpeg/jfdctflt.c
	../../jpeg/jfdctfst.c
	../../jpeg/jfdctint.c
	../../jpeg/jidctflt.c
	../../jpeg/jidctfst.c
	../../jpeg/jidctint.c
	../../jpeg/jmemmgr.c
	../../jpeg/jmemnobs.c
	../../jpeg/jquant1.c
	../../jpeg/jquant2.c
	../../jpeg/jutils.c
)

add_definitions(
	-DTARGET_LINUX
)

add_executable(${ARTIFACT} ${SOURCES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")

target_compile_options(${ARTIFACT}
	PRIVATE -Wall
	PRIVATE -fPIC
	PRIVATE -m32
	PRIVATE -Werror
)

target_include_directories(${ARTIFACT}
	PRIVATE .
	PRIVATE ${PROJECT_SOURCE_DIR}/../..
	PRIVATE ${PROJECT_SOURCE_DIR}/../../..
	PRIVATE ${PROJECT_SOURCE_DIR}/../../../libvncserver
	PRIVATE ${PROJECT_SOURCE_DIR}/../../../libvncserver/common
	PRIVATE ${PROJECT_SOURCE_DIR}/../../inih/cpp
	PRIVATE ${PROJECT_SOURCE_DIR}/../../jpeg
	PRIVATE ${PROJECT_SOURCE_DIR}/../../zlib
)

target_link_libraries(${ARTIFACT}
	pthread
)
