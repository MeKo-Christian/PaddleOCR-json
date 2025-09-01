# OpenCV installation utility functions

# collect_opencv_lib()
# @parameters
#   in_opencv_lib_dir               [input][str]  User input OPENCV_DIR
#   in_cmake_shared_lib_suffix      [input][str]  CMAKE_SHARED_LIBRARY_SUFFIX
#   in_opencv_lib_name              [input][str]  Current linked OpenCV library name, such as opencv_core, opencv_imgcodecs
#   out_opencv_miss_lib             [output][bool] Whether the input OpenCV library file was found
#   out_opencv_lib_regular_file     [output][str]  Absolute path of found OpenCV library regular file
#   out_opencv_lib_symlink          [output][list] Found OpenCV library regular file and library symlink names, used for subsequent symlink creation
macro(collect_opencv_lib
        in_opencv_lib_dir
        in_cmake_shared_lib_suffix
        in_opencv_lib_name
        out_opencv_miss_lib
        out_opencv_lib_regular_file
        out_opencv_lib_symlink
    )
    # Find library files
    file(GLOB_RECURSE __opencv_libs_found "${in_opencv_lib_dir}/**/*${in_opencv_lib_name}*${in_cmake_shared_lib_suffix}*")
    
    # Check if found
    if("${__opencv_libs_found}" STREQUAL "")
        message(STATUS "Missing independent shared library: ${in_opencv_lib_name}")
        set(${out_opencv_miss_lib} TRUE)
    endif()
    
    # Split regular files and symbolic links
    foreach(__item ${__opencv_libs_found})
        if(IS_SYMLINK ${__item})
            get_filename_component(__item_name ${__item} NAME)
            list(APPEND __symlink_libs ${__item_name})
        else()
            set(__regular_libs ${__item})
        endif()
    endforeach()
    
    # Output
    set(${out_opencv_lib_regular_file} ${__regular_libs})
    if(DEFINED __symlink_libs AND NOT "${__symlink_libs}" STREQUAL "")
        get_filename_component(__regular_libs_name "${__regular_libs}" NAME)
        set(${out_opencv_lib_symlink} ${__regular_libs_name} ${__symlink_libs})
    endif()
    
    # Clean up cached variables
    unset(__opencv_libs_found)
    unset(__item)
    unset(__item_name)
    unset(__symlink_libs)
    unset(__regular_libs)
    unset(__regular_libs_name)
endmacro(collect_opencv_lib)
