# glm
option(GLM_BUILD_LIBRARY "Build dynamic/static library" OFF)
option(GLM_BUILD_TESTS "Build the test programs" OFF)
option(GLM_ENABLE_CXX_98 "Enable C++ 98" OFF)
option(GLM_ENABLE_CXX_11 "Enable C++ 11" OFF)
option(GLM_ENABLE_CXX_14 "Enable C++ 14" OFF)
option(GLM_ENABLE_CXX_17 "Enable C++ 17" OFF)
option(GLM_ENABLE_CXX_20 "Enable C++ 20" ON)
option(GLM_ENABLE_LANG_EXTENSIONS "Enable language extensions" OFF)
option(GLM_DISABLE_AUTO_DETECTION "Disable platform, compiler, arch and C++ language detection" OFF)
add_subdirectory(glm)

# rapidobj
option(RAPIDOBJ_BuildTests "Build Tests." OFF)
option(RAPIDOBJ_BuildTools "Build Tools." OFF)
option(RAPIDOBJ_BuildExamples "Build Examples." OFF)
add_subdirectory(rapidobj)

# spdlog
option(SPDLOG_BUILD_EXAMPLE "Build spdlog examples" OFF)
option(SPDLOG_BUILD_EXAMPLE_HO "Build spdlog header-only examples" OFF)
option(SPDLOG_BUILD_TESTS "Build spdlog tests" OFF)
option(SPDLOG_BUILD_TESTS_HO "Build spdlog header-only tests" OFF)
option(SPDLOG_BUILD_BENCH "Build spdlog benchmark" OFF)
option(SPDLOG_INSTALL "Install spdlog" OFF)
add_subdirectory(spdlog)