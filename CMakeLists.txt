cmake_minimum_required (VERSION 2.6)
project (camera-multitouch)

find_package(OpenCV REQUIRED)
set(LIBS ${LIBS} ${OpenCV_LIBS})

find_package(X11)
set(LIBS ${LIBS} ${X11_LIBRARIES})

add_executable(camera-multitouch main.cpp input_x11.cpp)
target_link_libraries(camera-multitouch ${LIBS})

