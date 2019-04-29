project("RecorderKinectV1")
#set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -lSDL2main -lSDL2")

find_package(OpenCV REQUIRED)
find_package(libfreenect REQUIRED)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
file(GLOB project_sources
    "*.h"
    "*.cpp"
)

#include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
#include_directories (./c_sync)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("/usr/local/include/libfreenect")
include_directories("/usr/include/libusb-1.0/")
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${project_sources})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} ${FREENECT_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
