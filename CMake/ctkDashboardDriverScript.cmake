#
# Included from a dashboard script, this cmake file will drive the configure and build
# steps of the different CTK sub-project (library, application or plugins)
#

# The following variable are expected to be define in the top-level script:
set(expected_variables
  CTEST_NOTES_FILES
  CTEST_SITE
  CTEST_DASHBOARD_ROOT
  CTEST_CMAKE_COMMAND
  CTEST_CMAKE_GENERATOR
  WITH_MEMCHECK
  WITH_COVERAGE
  CTEST_BUILD_CONFIGURATION
  CTEST_TEST_TIMEOUT
  CTEST_BUILD_FLAGS
  TEST_TO_EXCLUDE_REGEX
  CTEST_PROJECT_NAME
  CTEST_SOURCE_DIRECTORY
  CTEST_BINARY_DIRECTORY
  CTEST_BUILD_NAME
  SCRIPT_MODE
  CTEST_COVERAGE_COMMAND
  CTEST_MEMORYCHECK_COMMAND
  CTEST_GIT_COMMAND
  )

foreach(var ${expected_variables})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "Variable ${var} should be defined in top-level script !")
  endif()
endforeach()

# Should binary directory be cleaned?
set(empty_binary_directory FALSE)

# Attempt to build and test also if 'ctest_update' returned an error
set(force_build FALSE)

set(model "")

if (SCRIPT_MODE STREQUAL "experimental")
  set(empty_binary_directory FALSE)
  set(force_build TRUE)
  set(model Experimental)
elseif (SCRIPT_MODE STREQUAL "continuous")
  set(empty_binary_directory FALSE)
  set(force_build FALSE)
  set(model Continous)
elseif (SCRIPT_MODE STREQUAL "nightly")
  set(empty_binary_directory TRUE)
  set(force_build TRUE)
  set(model Nightly)
else()
  message(FATAL_ERROR "Unknown script mode: '${SCRIPT_MODE}'. Script mode should be either 'experimental', 'continuous' or 'nightly'")
endif()

message("script_mode:${SCRIPT_MODE}")
#message("model:${model}")
#message("empty_binary_directory:${empty_binary_directory}")
#message("force_build:${force_build}")

set(CTEST_USE_LAUNCHERS 1)

if(empty_binary_directory)
  message("Directory ${CTEST_BINARY_DIRECTORY} cleaned !")
  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
endif()

if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
  set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone git@github.com:pieper/CTK.git ${CTEST_SOURCE_DIRECTORY}")
endif()

set(CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")

#
# run_ctest macro
#
MACRO(run_ctest)
  ctest_start(${model})
  ctest_update(SOURCE "${CTEST_SOURCE_DIRECTORY}" RETURN_VALUE res)
  #ctest_submit(PARTS Update Notes)

  # force a build if this is the first run and the build dir is empty
  if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
    message("First time build - Initialize CMakeCache.txt")
    set(res 1)

    # Write initial cache.
    file(WRITE "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt" "
CTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
QT_QMAKE_EXECUTABLE:FILEPATH=/home/jchris/Projects/qtsdk-2010.02/qt/bin/qmake
SUPERBUILD_EXCLUDE_CTKBUILD_TARGET:BOOL=TRUE
WITH_COVERAGE:BOOL=TRUE
")
  endif()
  
  if (res GREATER 0 OR force_build)
    message("Configure SuperBuild")
    
    set_property(GLOBAL PROPERTY SubProject SuperBuild)
    set_property(GLOBAL PROPERTY Label SuperBuild)
     
    ctest_configure(BUILD "${CTEST_BINARY_DIRECTORY}")
    ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")
    ctest_submit(PARTS Configure)
    ctest_submit(FILES "${CTEST_BINARY_DIRECTORY}/Project.xml")

    # Build top level
    message("Build SuperBuild")
    ctest_build(BUILD "${CTEST_BINARY_DIRECTORY}" APPEND)
    ctest_submit(PARTS Build)
    
    ctest_test(
      BUILD "${CTEST_BINARY_DIRECTORY}" 
      INCLUDE_LABEL "SuperBuild"
      PARALLEL_LEVEL 8
      EXCLUDE ${TEST_TO_EXCLUDE_REGEX})
    # runs only tests that have a LABELS property matching "${subproject}"
    ctest_submit(PARTS Test)
      
    set(ctk_build_dir "${CTEST_BINARY_DIRECTORY}/CTK-build")
    
    # To get CTEST_PROJECT_SUBPROJECTS definition
    include("${CTEST_BINARY_DIRECTORY}/CTestConfigSubProject.cmake")
    
    set(test_kits 
      CxxTests
      #PythonTests
      )
    
   foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
     set_property(GLOBAL PROPERTY SubProject ${subproject})
     set_property(GLOBAL PROPERTY Label ${subproject})
     message("Build ${subproject}")
    
     # Configure target
     ctest_configure(BUILD "${CTEST_BINARY_DIRECTORY}")
     ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")
     ctest_submit(PARTS Configure)
     
     # Build target
     set(CTEST_BUILD_TARGET "${subproject}")
     ctest_build(BUILD "${ctk_build_dir}" APPEND)
     
     # Loop over test kits and try to build the corresponding target
     foreach(test_kit ${test_kits})
       message("Build ${subproject}${test_kit}")
       set(CTEST_BUILD_TARGET "${subproject}${test_kit}")
       ctest_build(BUILD "${ctk_build_dir}" APPEND)
     endforeach()
     
     ctest_submit(PARTS Build)
    endforeach()
    
    # HACK Unfortunately ctest_coverage ignores the build argument, try to force it...
    file(READ ${ctk_build_dir}/CMakeFiles/TargetDirectories.txt ctk_build_coverage_dirs)
    file(APPEND "${CTEST_BINARY_DIRECTORY}/CMakeFiles/TargetDirectories.txt" "${ctk_build_coverage_dirs}")
    
    foreach(subproject ${CTEST_PROJECT_SUBPROJECTS})
      set_property(GLOBAL PROPERTY SubProject ${subproject})
      set_property(GLOBAL PROPERTY Label ${subproject})
      message("Test ${subproject}")

      ctest_test(
        BUILD "${ctk_build_dir}" 
        INCLUDE_LABEL "${subproject}"
        PARALLEL_LEVEL 8
        EXCLUDE ${test_to_exclude_regex})
      # runs only tests that have a LABELS property matching "${subproject}"
      ctest_submit(PARTS Test)

      # Coverage per sub-project (library, application or plugin)
      if (WITH_COVERAGE AND CTEST_COVERAGE_COMMAND)
        ctest_coverage(BUILD "${ctk_build_dir}" LABELS "${subproject}")
        ctest_submit(PARTS Coverage)
      endif ()

      #if (WITH_MEMCHECK AND CTEST_MEMORYCHECK_COMMAND)
      #  ctest_memcheck(BUILD "${ctk_build_dir}" INCLUDE_LABEL "${subproject}")
      #  ctest_submit(PARTS MemCheck)
      #endif ()

    endforeach()
    
    set_property(GLOBAL PROPERTY SubProject SuperBuild)
    set_property(GLOBAL PROPERTY Label SuperBuild)
    
    # Global coverage ... 
    if (WITH_COVERAGE AND CTEST_COVERAGE_COMMAND)
      ctest_coverage(BUILD "${ctk_build_dir}")
      ctest_submit(PARTS Coverage)
    endif ()
    
    # Global dynamic analysis ...
    if (WITH_MEMCHECK AND CTEST_MEMORYCHECK_COMMAND)
        ctest_memcheck(BUILD "${ctk_build_dir}")
        ctest_submit(PARTS MemCheck)
      endif ()
    
    # Note should be at the end
    ctest_submit(PARTS Notes)
  
  endif()
endmacro()

if(SCRIPT_MODE STREQUAL "continous")
  while(${CTEST_ELAPSED_TIME} LESS 68400)
    set(START_TIME ${CTEST_ELAPSED_TIME})
    run_ctest()
    # Loop no faster than once every 5 minutes
    message("Wait for 5 minutes ...")
    ctest_sleep(${START_TIME} 300 ${CTEST_ELAPSED_TIME})
  endwhile()
else()
  run_ctest()
endif()