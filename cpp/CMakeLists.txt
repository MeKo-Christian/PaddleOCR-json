# cmake_minimum_required(VERSION 3.15...3.30)
cmake_minimum_required(VERSION 3.14)
# Originally wanted to set a version range for cmake_minimum_required,
# but found that when changing the minimum cmake version to 3.15, Windows MSVC would show
# a lot of linking errors, basically all unresolved external symbol errors for some libc low-level functions
# and symbol redefinition errors. The specific cause is unknown, even disabling cmake 3.15's new policies
# didn't help. Don't know what change in 3.15 caused this. But using 3.14 works fine now.
# 
# cmake version range:
# https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html
# https://github.com/jbeder/yaml-cpp/pull/1211#issuecomment-1681352550


project(PaddleOCR-json CXX C)

# Eliminate CMake Policy CMP0048 warning
# Let CMake handle policy CMP0048 in new mode
# https://cmake.org/cmake/help/latest/policy/CMP0048.html
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif ()


set(DEMO_NAME "PaddleOCR-json") 

# Make CMake put the final executable in build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Compilation-related parameters
option(WITH_MKL          "Use MKL or OpenBlas, default use MKL."                                  ON)
option(WITH_GPU          "Use GPU or CPU, default use CPU."                                      OFF)
option(WITH_STATIC_LIB   "Compile into static library or shared library, default compile into static library."   ON)
option(WITH_TENSORRT     "Use TensorRT, default off."                                         OFF)

# Add cross-compilation and static build options
option(BUILD_STATIC_BINARY "Build fully static binary with musl" OFF)
option(ENABLE_C_API "Build C API library" ON)
option(ENABLE_CROSS_COMPILE "Enable cross-compilation support" OFF)
set(CROSS_COMPILE_PREFIX "" CACHE STRING "Cross-compilation prefix (e.g., aarch64-linux-gnu-)")

if (UNIX AND NOT APPLE) # Linux
    # Cannot compile when using `WITH_STATIC_LIB=ON` in Linux environment
    # https://github.com/PaddlePaddle/PaddleOCR/issues/10602
    # Here we forcibly set it to `OFF`
    set(WITH_STATIC_LIB OFF)
endif ()

# Dependency library path-related parameters
SET(PADDLE_LIB "" CACHE PATH "Path to paddle_inference")
SET(OPENCV_DIR "" CACHE PATH "Path to library. On Linux, if already installed in system, no need to specify.")
SET(CUDA_LIB "" CACHE PATH "Path to library")
SET(CUDNN_LIB "" CACHE PATH "Path to library")
SET(TENSORRT_DIR "" CACHE PATH "Use TensorRT compilation and set its path")

# Feature-related parameters
option(ENABLE_CLIPBOARD         "Enable clipboard function. Default off."        OFF)
option(ENABLE_REMOTE_EXIT       "Enable remote shutdown server command. Default on."  ON)
option(ENABLE_JSON_IMAGE_PATH   "Enable json command image_path. Default on." ON)

# CMake feature-related parameters
option(INSTALL_WITH_TOOLS       "Include tool files during CMake installation. Default on."      ON)
option(BUILD_TESTS              "Build unit tests. Default off."                                  OFF)


# compiler flags

# Detect if running under WSL, if so set compiler flag: -DUNDER_WSL
if (UNIX AND NOT APPLE) # Linux
    execute_process (
        COMMAND "uname" "-a"
        OUTPUT_VARIABLE UNAME_RESULT
    )
    string(STRIP "${UNAME_RESULT}" UNAME_RESULT)
    if (UNAME_RESULT MATCHES ".*WSL.*")
        message(STATUS "Compiling under WSL")
        add_definitions(-DUNDER_WSL)
    endif()
endif ()

# If clipboard is enabled on Windows, set a compiler flag: -DENABLE_CLIPBOARD
if (ENABLE_CLIPBOARD AND WIN32)
    add_definitions(-DENABLE_CLIPBOARD)
elseif () # Disable ENABLE_CLIPBOARD on other systems
    set(ENABLE_CLIPBOARD OFF)
endif()

# Enable remote shutdown server command, set compiler flag: -DENABLE_REMOTE_EXIT
if (ENABLE_REMOTE_EXIT)
    add_definitions(-DENABLE_REMOTE_EXIT)
endif()

# Enable json command image_path, set compiler flag: -DENABLE_JSON_IMAGE_PATH
if (ENABLE_JSON_IMAGE_PATH)
    add_definitions(-DENABLE_JSON_IMAGE_PATH)
endif()

macro(safe_set_static_flag)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
endmacro()

if (WITH_MKL)
    ADD_DEFINITIONS(-DUSE_MKL)
endif()

if(NOT DEFINED PADDLE_LIB)
    message(FATAL_ERROR "please set PADDLE_LIB with -DPADDLE_LIB=/path/paddle/lib")
endif()

if(NOT DEFINED OPENCV_DIR)
    message(FATAL_ERROR "please set OPENCV_DIR with -DOPENCV_DIR=/path/opencv")
endif()


# Output feature settings
message(STATUS "Features:")
message(STATUS "    ENABLE_CLIPBOARD: ${ENABLE_CLIPBOARD}")
message(STATUS "    ENABLE_REMOTE_EXIT: ${ENABLE_REMOTE_EXIT}")
message(STATUS "    ENABLE_JSON_IMAGE_PATH: ${ENABLE_JSON_IMAGE_PATH}")
# Output CMake feature settings
message(STATUS "CMake Features:")
message(STATUS "    INSTALL_WITH_TOOLS: ${INSTALL_WITH_TOOLS}")


option(USE_OPENCV_PKGCONFIG "Use pkg-config to find OpenCV (musl builds)" OFF)

# Load dependencies

# OpenCV
# Try to find OpenCV from manually provided OpenCV path
# Please provide a path containing "OpenCVConfig.cmake" or "opencv-config.cmake"
# Or provide the OpenCV compiled installation directory

set(OPENCV_COMPONENTS_TO_LOAD
    core
    imgproc
    imgcodecs
)

if(USE_OPENCV_PKGCONFIG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OPENCV REQUIRED opencv4)
    message(STATUS "Using OpenCV via pkg-config: include=${OPENCV_INCLUDE_DIRS}")
    include_directories(${OPENCV_INCLUDE_DIRS})
    link_directories(${OPENCV_LIBRARY_DIRS})
    set(OpenCV_LIBS ${OPENCV_LIBRARIES})
else()
    # Provided by CMake parameter (-DOPENCV_DIR)
    if(DEFINED OPENCV_DIR AND NOT "${OPENCV_DIR}" STREQUAL "")
        message(STATUS "Setting OPENCV_DIR: ${OPENCV_DIR}")
        find_package(OpenCV REQUIRED PATHS ${OPENCV_DIR} COMPONENTS ${OPENCV_COMPONENTS_TO_LOAD} NO_DEFAULT_PATH)
        set(CUSTOM_OPENCV_DIR ${OPENCV_DIR})
    # No OpenCV path provided, let CMake find it itself
    else()
        find_package(OpenCV REQUIRED COMPONENTS ${OPENCV_COMPONENTS_TO_LOAD})
    endif()
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()


# PaddleOCR
include_directories("${PADDLE_LIB}/paddle/include")
link_directories("${PADDLE_LIB}/paddle/lib")


if (WIN32)
    add_definitions("/DGOOGLE_GLOG_DLL_DECL=")
    if(WITH_MKL)
        set(FLAG_OPENMP "/openmp")
    endif()
    set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} /bigobj /MTd ${FLAG_OPENMP}")
    set(CMAKE_C_FLAGS_RELEASE  "${CMAKE_C_FLAGS_RELEASE} /bigobj /MT ${FLAG_OPENMP}")
    set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} /bigobj /MTd ${FLAG_OPENMP}")
    set(CMAKE_CXX_FLAGS_RELEASE   "${CMAKE_CXX_FLAGS_RELEASE} /bigobj /MT ${FLAG_OPENMP}")
    if (WITH_STATIC_LIB)
        safe_set_static_flag()
        add_definitions(-DSTATIC_LIB)
    endif()
    message(STATUS "cmake c debug flags:" ${CMAKE_C_FLAGS_DEBUG})
    message(STATUS "cmake c release flags:" ${CMAKE_C_FLAGS_RELEASE})
    message(STATUS "cmake cxx debug flags:" ${CMAKE_CXX_FLAGS_DEBUG})
    message(STATUS "cmake cxx release flags:" ${CMAKE_CXX_FLAGS_RELEASE})
else()
    if(WITH_MKL)
        set(FLAG_OPENMP "-fopenmp")
    endif()
    # Cannot compile with `-g -o3` on Linux
    # https://github.com/PaddlePaddle/PaddleOCR/issues/4654#issuecomment-977398981
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -o3 ${FLAG_OPENMP} -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG_OPENMP} -std=c++17")
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    message(STATUS "cmake cxx flags:" ${CMAKE_CXX_FLAGS})
endif()

if (WITH_GPU)
    if (NOT DEFINED CUDA_LIB OR ${CUDA_LIB} STREQUAL "")
        message(FATAL_ERROR "please set CUDA_LIB with -DCUDA_LIB=/path/cuda-8.0/lib64")
    endif()
    if (NOT WIN32)
        if (NOT DEFINED CUDNN_LIB)
            message(FATAL_ERROR "please set CUDNN_LIB with -DCUDNN_LIB=/path/cudnn_v7.4/cuda/lib64")
        endif()
    endif(NOT WIN32)
endif()

# Load gflags

# Compile gflags as static library
set(GFLAGS_BUILD_STATIC_LIBS ON)

# Hide path names in help information, keep only the last filename
set(GFLAGS_HIDE_PATH_IN_HELP "Basename")

# Use shortest possible double precision
set(GFLAGS_USE_SHORTEST_DOUBLE_PRECISION ON)

# Set gflags C++ namespace to google
set(GFLAGS_NAMESPACE "google")

# Add gflags cmake project
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/third_party/gflags")
include_directories(${gflags_BINARY_DIR}/include)


# Load other third-party libraries from paddle_inference
include_directories("${PADDLE_LIB}/third_party/install/protobuf/include")
include_directories("${PADDLE_LIB}/third_party/install/glog/include")
include_directories("${PADDLE_LIB}/third_party/install/xxhash/include")
include_directories("${PADDLE_LIB}/third_party/install/zlib/include")
include_directories("${PADDLE_LIB}/third_party/install/onnxruntime/include")
include_directories("${PADDLE_LIB}/third_party/install/paddle2onnx/include")
include_directories("${PADDLE_LIB}/third_party/boost")
include_directories("${PADDLE_LIB}/third_party/eigen3")

include_directories("${CMAKE_SOURCE_DIR}/")

if (NOT WIN32)
    if (WITH_TENSORRT AND WITH_GPU)
        include_directories("${TENSORRT_DIR}/include")
        link_directories("${TENSORRT_DIR}/lib")
    endif()
endif(NOT WIN32)

link_directories("${PADDLE_LIB}/third_party/install/zlib/lib")

link_directories("${PADDLE_LIB}/third_party/install/protobuf/lib")
link_directories("${PADDLE_LIB}/third_party/install/glog/lib")
link_directories("${PADDLE_LIB}/third_party/install/xxhash/lib")
link_directories("${PADDLE_LIB}/third_party/install/onnxruntime/lib")
link_directories("${PADDLE_LIB}/third_party/install/paddle2onnx/lib")
link_directories("${PADDLE_LIB}/paddle/lib")


if(WITH_MKL)
    include_directories("${PADDLE_LIB}/third_party/install/mklml/include")
    if (WIN32)
        set(MATH_LIB ${PADDLE_LIB}/third_party/install/mklml/lib/mklml.lib
                    ${PADDLE_LIB}/third_party/install/mklml/lib/libiomp5md.lib)
    else ()
        set(MATH_LIB ${PADDLE_LIB}/third_party/install/mklml/lib/libmklml_intel${CMAKE_SHARED_LIBRARY_SUFFIX}
                    ${PADDLE_LIB}/third_party/install/mklml/lib/libiomp5${CMAKE_SHARED_LIBRARY_SUFFIX})
        # Add library search directory to find third_party/install/onednn/lib/libdnnl.so.3
        link_directories("${PADDLE_LIB}/third_party/install/onednn/lib")
    endif ()
    set(MKLDNN_PATH "${PADDLE_LIB}/third_party/install/mkldnn")
    if(EXISTS ${MKLDNN_PATH})
        include_directories("${MKLDNN_PATH}/include")
        if (WIN32)
            set(MKLDNN_LIB ${MKLDNN_PATH}/lib/mkldnn.lib)
        else ()
            set(MKLDNN_LIB ${MKLDNN_PATH}/lib/libmkldnn.so.0)
        endif ()
    endif()
    else()
    if (WIN32)
        set(MATH_LIB ${PADDLE_LIB}/third_party/install/openblas/lib/openblas${CMAKE_STATIC_LIBRARY_SUFFIX})
    else ()
        # Prefer static lib if available; otherwise fall back to shared
        if (EXISTS "${PADDLE_LIB}/third_party/install/openblas/lib/libopenblas${CMAKE_STATIC_LIBRARY_SUFFIX}")
            set(MATH_LIB ${PADDLE_LIB}/third_party/install/openblas/lib/libopenblas${CMAKE_STATIC_LIBRARY_SUFFIX})
        elseif (EXISTS "${PADDLE_LIB}/third_party/install/openblas/lib/libopenblas${CMAKE_SHARED_LIBRARY_SUFFIX}")
            set(MATH_LIB ${PADDLE_LIB}/third_party/install/openblas/lib/libopenblas${CMAKE_SHARED_LIBRARY_SUFFIX})
        else()
            # Last-resort: rely on system resolution if present
            set(MATH_LIB openblas)
        endif()
    endif ()
endif()

# Note: libpaddle_inference_api.so/a must put before libpaddle_inference.so/a
if(WITH_STATIC_LIB)
    if(WIN32)
        set(DEPS
            ${PADDLE_LIB}/paddle/lib/paddle_inference${CMAKE_STATIC_LIBRARY_SUFFIX})
    else()
        set(DEPS
            ${PADDLE_LIB}/paddle/lib/libpaddle_inference${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
else()
    if(WIN32)
        set(DEPS
            ${PADDLE_LIB}/paddle/lib/paddle_inference${CMAKE_STATIC_LIBRARY_SUFFIX})
    else()
        set(DEPS
            ${PADDLE_LIB}/paddle/lib/libpaddle_inference${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()
endif(WITH_STATIC_LIB)

if (NOT WIN32)
    set(DEPS ${DEPS}
        ${MATH_LIB} ${MKLDNN_LIB}
        glog gflags protobuf z xxhash
        )
    if(EXISTS "${PADDLE_LIB}/third_party/install/snappystream/lib")
        set(DEPS ${DEPS} snappystream)
    endif()
    if (EXISTS "${PADDLE_LIB}/third_party/install/snappy/lib")
        set(DEPS ${DEPS} snappy)
    endif()
else() # WIN32
    set(DEPS ${DEPS}
        ${MATH_LIB} ${MKLDNN_LIB}
        glog gflags libprotobuf xxhash)
    set(DEPS ${DEPS} libcmt shlwapi)
    if (EXISTS "${PADDLE_LIB}/third_party/install/snappy/lib")
        set(DEPS ${DEPS} snappy)
    endif()
    if(EXISTS "${PADDLE_LIB}/third_party/install/snappystream/lib")
        set(DEPS ${DEPS} snappystream)
    endif()
endif(NOT WIN32)


if(WITH_GPU)
    if(NOT WIN32)
        if (WITH_TENSORRT)
            set(DEPS ${DEPS} ${TENSORRT_DIR}/lib/libnvinfer${CMAKE_SHARED_LIBRARY_SUFFIX})
            set(DEPS ${DEPS} ${TENSORRT_DIR}/lib/libnvinfer_plugin${CMAKE_SHARED_LIBRARY_SUFFIX})
        endif()
        set(DEPS ${DEPS} ${CUDA_LIB}/libcudart${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(DEPS ${DEPS} ${CUDNN_LIB}/libcudnn${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(DEPS ${DEPS} ${CUDA_LIB}/cudart${CMAKE_STATIC_LIBRARY_SUFFIX} )
        set(DEPS ${DEPS} ${CUDA_LIB}/cublas${CMAKE_STATIC_LIBRARY_SUFFIX} )
        set(DEPS ${DEPS} ${CUDNN_LIB}/cudnn${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
endif()


if (NOT WIN32)
    set(EXTERNAL_LIB "-ldl -lrt -lgomp -lz -lm -lpthread")
    set(DEPS ${DEPS} ${EXTERNAL_LIB})
endif()

# The modules to load have already been specified in find_package,
# so we can directly use OpenCV_LIBS here
set(DEPS ${DEPS} ${OpenCV_LIBS})

option(USE_AUTOLOG "Enable AutoLog dependency (downloads during configure)" OFF)
if(USE_AUTOLOG)
    include(FetchContent)
    include(external-cmake/auto-log.cmake)
    include_directories(${FETCHCONTENT_BASE_DIR}/extern_autolog-src)
else()
    message(STATUS "USE_AUTOLOG=OFF: using stub AutoLogger (no network)")
endif()

include_directories(include)
AUX_SOURCE_DIRECTORY(./src SRCS)
add_executable(${DEMO_NAME} ${SRCS})
target_include_directories(${DEMO_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${DEMO_NAME} ${DEPS})


# C API Library (if enabled)
if(ENABLE_C_API)
    # Create C API wrapper source
    set(C_API_SOURCES
        src/paddleocr_c_api.cpp
    )

    add_library(paddleocr_c SHARED ${C_API_SOURCES})
target_include_directories(paddleocr_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(paddleocr_c ${DEPS})

    # Install C API library and headers
    install(TARGETS paddleocr_c
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(FILES include/paddleocr_c_api.h
        DESTINATION include
    )
endif()


# Set up shared libraries to install

# PaddleOCR, directly find all shared libraries in its path
message(STATUS "Collecting PaddleOCR shared libraries")
file(GLOB_RECURSE PADDLE_LIB_FILES "${PADDLE_LIB}/**/*${CMAKE_SHARED_LIBRARY_SUFFIX}*")
foreach(ITEM ${PADDLE_LIB_FILES})
    list(APPEND LIBS_TO_INSTALL ${ITEM})
endforeach()

# OpenCV
include(cmake/opencv-install-utils.cmake)
if(DEFINED CUSTOM_OPENCV_DIR AND NOT "${CUSTOM_OPENCV_DIR}" STREQUAL "")
    message(STATUS "Collecting custom OpenCV shared libraries")
    set(OPENCV_MISS_LIBS FALSE)
    
    # Collect shared libraries and their symlink information
    collect_opencv_lib(${CUSTOM_OPENCV_DIR} ${CMAKE_SHARED_LIBRARY_SUFFIX} "opencv_core" opencv_core_miss_lib opencv_core_regular opencv_core_symlink)
    collect_opencv_lib(${CUSTOM_OPENCV_DIR} ${CMAKE_SHARED_LIBRARY_SUFFIX} "opencv_imgcodecs" opencv_imgcodecs_miss_lib opencv_imgcodecs_regular opencv_imgcodecs_symlink)
    collect_opencv_lib(${CUSTOM_OPENCV_DIR} ${CMAKE_SHARED_LIBRARY_SUFFIX} "opencv_imgproc" opencv_imgproc_miss_lib opencv_imgproc_regular opencv_imgproc_symlink)
    
    if(opencv_core_miss_lib OR opencv_imgcodecs_miss_lib OR opencv_imgproc_miss_lib)
        set(OPENCV_MISS_LIBS TRUE)
    endif()
    
    # If missing, try opencv_world
    if(DEFINED OPENCV_MISS_LIBS AND OPENCV_MISS_LIBS)
        message(STATUS "Missing OpenCV independent shared libraries, try opencv_world")
        collect_opencv_lib(${CUSTOM_OPENCV_DIR} ${CMAKE_SHARED_LIBRARY_SUFFIX} "opencv_world" opencv_world_miss_lib opencv_world_regular opencv_world_symlink)
        if(opencv_world_miss_lib)
            message(FATAL_ERROR "Cannot find OpenCV independent shared libraries or opencv_world. Please make sure your custom OpenCV directory contains the missing libraries.")
        else()
            message(STATUS "Found opencv_world")
            list(APPEND LIBS_TO_INSTALL ${opencv_world_regular})
            list(JOIN opencv_world_symlink "/" opencv_world_symlink_str)
            list(APPEND SYMS_TO_INSTALL ${opencv_world_symlink_str})
        endif()
    
    # If found, add to variables
    else()
        # Here because cmake cannot store list(list(str)), we synthesize the list into a str and APPEND to SYMS_TO_INSTALL,
        # then split it back when needed. The output str looks like this:
        # "${OpenCV library regular filename}/${OpenCV library symlink name1}/${OpenCV library symlink name2}/..."
        
        list(APPEND LIBS_TO_INSTALL ${opencv_core_regular})
        list(JOIN opencv_core_symlink "/" opencv_core_symlink_str)
        list(APPEND SYMS_TO_INSTALL ${opencv_core_symlink_str})
        
        list(APPEND LIBS_TO_INSTALL ${opencv_imgcodecs_regular})
        list(JOIN opencv_imgcodecs_symlink "/" opencv_imgcodecs_symlink_str)
        list(APPEND SYMS_TO_INSTALL ${opencv_imgcodecs_symlink_str})
        
        list(APPEND LIBS_TO_INSTALL ${opencv_imgproc_regular})
        list(JOIN opencv_imgproc_symlink "/" opencv_imgproc_symlink_str)
        list(APPEND SYMS_TO_INSTALL ${opencv_imgproc_symlink_str})
    endif()
endif()

# Finally copy all shared libraries to be installed to the TARGET output folder
# In the subsequent installation phase, they will be installed directly from this folder
foreach(ITEM ${LIBS_TO_INSTALL})
    add_custom_command(TARGET ${DEMO_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ITEM}
        "$<TARGET_FILE_DIR:${DEMO_NAME}>"
    )
endforeach()


# CMake TARGET installation path settings
# Reference OpenCV:
# https://github.com/opencv/opencv/blob/a03b81316782ae30038b288fd3568993fa1e3538/CMakeLists.txt#L141-L152
# https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT.html
# If CMake installation path is default
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Local compilation
    if(NOT CMAKE_TOOLCHAIN_FILE)
        if(WIN32)
            set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation path" FORCE)
        else()
            set(CMAKE_INSTALL_PREFIX "/usr/" CACHE PATH "Installation path" FORCE)
        endif()
    
    # Other cross-platform compilation
    else()
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation path" FORCE)
    endif()
endif()

# Set runtime environment path for installed files
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Install TARGET
install(
    TARGETS ${DEMO_NAME}
    CONFIGURATIONS Debug Release
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)
# Other installations
if(WIN32)
    # Install shared libraries
    install(
        DIRECTORY "$<TARGET_FILE_DIR:${DEMO_NAME}>/"
        DESTINATION "bin"
        FILES_MATCHING
        PATTERN "*${CMAKE_SHARED_LIBRARY_SUFFIX}*"
        PATTERN "${DEMO_NAME}*" EXCLUDE
    )
else()
    # Install shared libraries
    install(
        DIRECTORY "$<TARGET_FILE_DIR:${DEMO_NAME}>/"
        DESTINATION "lib"
        FILES_MATCHING
        PATTERN "*${CMAKE_SHARED_LIBRARY_SUFFIX}*"
        PATTERN "${DEMO_NAME}*" EXCLUDE
    )
    
    # Create symlinks for shared libraries
    foreach(ITEM_STR ${SYMS_TO_INSTALL})
        string(REPLACE "/" ";" ITEM_LIST ${ITEM_STR})
        
        # Here I originally wanted to use cmake 3.15's list(POP_FRONT) feature,
        # but raising the minimum cmake version to 3.15 causes many linking errors (see details at the top of the file)
        # So I implemented it using an old API
        # list(POP_FRONT ITEM_LIST ITEM_REGULAR_NAME)
        list(GET ITEM_LIST 0 ITEM_REGULAR_NAME)
        list(REMOVE_AT ITEM_LIST 0)
        
        foreach(ITEM_SYMLINK_NAME ${ITEM_LIST})
            # Here we add the following script to the cmake_install.cmake installation script:
            # Enter the ${CMAKE_INSTALL_PREFIX}/lib directory in the installation script environment
            # Then create the symlink
            install(CODE "\
                execute_process(COMMAND ${CMAKE_COMMAND} -E chdir \$\{CMAKE_INSTALL_PREFIX\}/lib \
                ${CMAKE_COMMAND} -E create_symlink ${ITEM_REGULAR_NAME} ${ITEM_SYMLINK_NAME})"
            )
        endforeach()
    endforeach()
    
    if (INSTALL_WITH_TOOLS)
        # Install startup script and installation script
        install(
            FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/linux_dist_tools/run.sh"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/linux_dist_tools/install_env.sh"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
            DESTINATION "."
        )
        # Install README.txt
        install(
            FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/linux_dist_tools/README.txt"
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ
            DESTINATION "."
        )
    endif()
endif()

# Add tests if enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
