# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Users\mario\AppData\Local\JetBrains\CLion 2019.2.1\bin\cmake\win\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Users\mario\AppData\Local\JetBrains\CLion 2019.2.1\bin\cmake\win\bin\cmake.exe" -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = Z:\ps-server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = Z:\ps-server\cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/logger.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/logger.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/logger.dir/flags.make

CMakeFiles/logger.dir/win/logger.c.obj: CMakeFiles/logger.dir/flags.make
CMakeFiles/logger.dir/win/logger.c.obj: ../win/logger.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=Z:\ps-server\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/logger.dir/win/logger.c.obj"
	C:\PROGRA~2\MINGW-~1\I686-8~1.0-P\mingw32\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles\logger.dir\win\logger.c.obj   -c Z:\ps-server\win\logger.c

CMakeFiles/logger.dir/win/logger.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/logger.dir/win/logger.c.i"
	C:\PROGRA~2\MINGW-~1\I686-8~1.0-P\mingw32\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E Z:\ps-server\win\logger.c > CMakeFiles\logger.dir\win\logger.c.i

CMakeFiles/logger.dir/win/logger.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/logger.dir/win/logger.c.s"
	C:\PROGRA~2\MINGW-~1\I686-8~1.0-P\mingw32\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S Z:\ps-server\win\logger.c -o CMakeFiles\logger.dir\win\logger.c.s

# Object files for target logger
logger_OBJECTS = \
"CMakeFiles/logger.dir/win/logger.c.obj"

# External object files for target logger
logger_EXTERNAL_OBJECTS =

logger.exe: CMakeFiles/logger.dir/win/logger.c.obj
logger.exe: CMakeFiles/logger.dir/build.make
logger.exe: CMakeFiles/logger.dir/linklibs.rsp
logger.exe: CMakeFiles/logger.dir/objects1.rsp
logger.exe: CMakeFiles/logger.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=Z:\ps-server\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable logger.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\logger.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/logger.dir/build: logger.exe

.PHONY : CMakeFiles/logger.dir/build

CMakeFiles/logger.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\logger.dir\cmake_clean.cmake
.PHONY : CMakeFiles/logger.dir/clean

CMakeFiles/logger.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" Z:\ps-server Z:\ps-server Z:\ps-server\cmake-build-debug Z:\ps-server\cmake-build-debug Z:\ps-server\cmake-build-debug\CMakeFiles\logger.dir\DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/logger.dir/depend

