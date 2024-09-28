cmake_minimum_required(VERSION 3.0)

project(CQL)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/hdr/*.h")

# Add executable
add_executable(main.exe ${SOURCES} ${HEADERS} "src/grid-control/CellRange.h" "src/grid-control/GridCell.cpp" "src/grid-control/GridCell.h" "src/grid-control/GridCellBase.cpp" "src/grid-control/GridCellBase.h" "src/grid-control/GridCtrl.cpp" "src/grid-control/GridCtrl.h" "src/grid-control/GridDropTarget.cpp" "src/grid-control/GridDropTarget.h" "src/grid-control/InPlaceEdit.cpp" "src/grid-control/InPlaceEdit.h" "src/grid-control/MemDC.h" "src/grid-control/TitleTip.cpp" "src/grid-control/TitleTip.h")
