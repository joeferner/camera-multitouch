cmake_minimum_required (VERSION 2.6)
project (camera-multitouch)

find_package(OpenCV REQUIRED)
set(LIBS ${LIBS} ${OpenCV_LIBS})

set(SRC ${SRC} main.cpp)

IF (APPLE)
  set(SRC ${SRC} input_osx.cpp)
ELSE()
  find_package(X11)
  set(LIBS ${LIBS} ${X11_LIBRARIES})
  set(SRC ${SRC} input_x11.cpp)
ENDIF()

add_executable(camera-multitouch ${SRC})
target_link_libraries(camera-multitouch ${LIBS})

