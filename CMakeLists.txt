cmake_minimum_required (VERSION 2.6)
project (camera-multitouch)

find_package( OpenCV REQUIRED )

add_executable(camera-multitouch main.cpp)
target_link_libraries(camera-multitouch ${OpenCV_LIBS})

