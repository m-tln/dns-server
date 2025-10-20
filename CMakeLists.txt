cmake_minimum_required(VERSION 3.15)
project(SimpleDnsServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN  -D_CRT_SECURE_NO_WARNINGS)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
    endif()
endif()

find_package(nlohmann_json 3.2.0 QUIET)

if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json via package manager")
else()
    message(STATUS "nlohmann_json not found, using FetchContent")
    
    include(FetchContent)
    
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    
    FetchContent_GetProperties(nlohmann_json)
    if(NOT nlohmann_json_POPULATED)
        FetchContent_Populate(nlohmann_json)
        add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

add_executable(dns_server
    src/main.cpp
    src/simple_dns_server.cpp
    src/config_loader.cpp
)

target_include_directories(dns_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(dns_server
    ${PLATFORM_LIBS}
)

if(TARGET nlohmann_json)
    target_link_libraries(dns_server nlohmann_json)
elseif(nlohmann_json_FOUND)
    target_link_libraries(dns_server nlohmann_json::nlohmann_json)
else()
    target_include_directories(dns_server PRIVATE ${nlohmann_json_SOURCE_DIR}/include)
endif()

if(MSVC)
    target_compile_options(dns_server PRIVATE /W4)
else()
    target_compile_options(dns_server PRIVATE -Wall -Wextra -pedantic)
endif()

# ============================================================================
# STATIC ANALYSIS AND CODE QUALITY TOOLS
# ============================================================================

option(ENABLE_CODE_ANALYSIS "Enable static code analysis" ON)
option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis" ON)
option(ENABLE_CPPCHECK "Enable cppcheck analysis" ON)
option(ENABLE_FORMAT "Enable clang-format targets" ON)

if(ENABLE_CODE_ANALYSIS)

    if(ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-14 clang-tidy-13 clang-tidy-12)
        if(CLANG_TIDY_EXE)
            message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")

            set(CLANG_TIDY_CHECKS
                -checks=*,
                -google-*,
                -readability-*,
                -modernize-*,
                -performance-*,
                -portability-*,
                -cppcoreguidelines-*,
                -bugprone-*,
                -clang-analyzer-*
            )

            set(CLANG_TIDY_OPTIONS
                -header-filter='${CMAKE_CURRENT_SOURCE_DIR}/*'
                -extra-arg=-std=c++17
                -warnings-as-errors=*
            )

            set(CMAKE_CXX_CLANG_TIDY
                ${CLANG_TIDY_EXE}
                ${CLANG_TIDY_CHECKS}
                ${CLANG_TIDY_OPTIONS}
            )
        else()
            message(WARNING "clang-tidy not found, disable with -DENABLE_CLANG_TIDY=OFF")
        endif()
    endif()

    if(ENABLE_CPPCHECK)
        find_program(CPPCHECK_EXE NAMES cppcheck)
        if(CPPCHECK_EXE)
            message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")

            set(CPPCHECK_OPTIONS
                --enable=all
                --inconclusive
                --suppress=missingInclude
                --suppress=unusedFunction
                --std=c++23
                --language=c++
                -I${CMAKE_CURRENT_SOURCE_DIR}/include
                --template= "[{severity}] [{id}] {message} {callstack} (On {file} : {line})"
                --error-exitcode=1
                --inline-suppr
                --quiet
            )

            add_custom_target(cppcheck
                COMMAND ${CPPCHECK_EXE} ${CPPCHECK_OPTIONS}
                        ${CMAKE_CURRENT_SOURCE_DIR}/src
                        ${CMAKE_CURRENT_SOURCE_DIR}/include
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Running cppcheck static analysis"
            )
        else()
            message(WARNING "cppcheck not found, disable with -DENABLE_CPPCHECK=OFF")
        endif()
    endif()

endif()

# ============================================================================
# CODE FORMATTING (Google C++ Style Guide)
# ============================================================================

if(ENABLE_FORMAT)
    find_program(CLANG_FORMAT_EXE NAMES clang-format clang-format-14 clang-format-13 clang-format-12)

    if(CLANG_FORMAT_EXE)
        message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")

        file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format
"---
Language: Cpp
BasedOnStyle: Google
AccessModifierOffset: -1
AlignAfterOpenBracket: Align
AlignConsecutiveMacros: None
AlignConsecutiveAssignments: None
AlignConsecutiveDeclarations: None
AlignEscapedNewlines: Left
AlignOperands: Align
AlignTrailingComments: true
AllowAllArgumentsOnNextLine: true
AllowAllConstructorInitializersOnNextLine: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: Never
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: InlineOnly
AllowShortLambdasOnASingleLine: All
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterDefinitionReturnType: None
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: false
AlwaysBreakTemplateDeclarations: Yes
BinPackArguments: true
BinPackParameters: true
BraceWrapping:
  AfterCaseLabel: false
  AfterClass: false
  AfterControlStatement: Never
  AfterEnum: false
  AfterFunction: false
  AfterNamespace: false
  AfterObjCDeclaration: false
  AfterStruct: false
  AfterUnion: false
  AfterExternBlock: false
  BeforeCatch: false
  BeforeElse: false
  IndentBraces: false
  SplitEmptyFunction: false
  SplitEmptyRecord: false
  SplitEmptyNamespace: false
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeInheritanceComma: false
BreakInheritanceList: BeforeColon
BreakBeforeTernaryOperators: true
BreakConstructorInitializers: BeforeColon
BreakAfterJavaFieldAnnotations: false
BreakStringLiterals: true
ColumnLimit: 80
CommentPragmas: '^ IWYU pragma:'
CompactNamespaces: false
ConstructorInitializerAllOnOneLineOrOnePerLine: false
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: false
DisableFormat: false
ExperimentalAutoDetectBinPacking: false
FixNamespaceComments: true
ForEachMacros:
  - foreach
  - Q_FOREACH
  - BOOST_FOREACH
IncludeBlocks: Preserve
IncludeCategories:
  - Regex: '^\"(llvm|llvm-c|clang|clang-c)/'
    Priority: 2
  - Regex: '^(<|\"(gtest|gmock|isl|json)/)'
    Priority: 3
  - Regex: '.*'
    Priority: 1
IncludeIsMainRegex: '(Test)?$'
IndentCaseLabels: false
IndentPPDirectives: None
IndentWidth: 2
IndentWrappedFunctionNames: false
JavaScriptQuotes: Leave
JavaScriptWrapImports: true
KeepEmptyLinesAtTheStartOfBlocks: true
MacroBlockBegin: ''
MacroBlockEnd: ''
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
ObjCBinPackProtocolList: Auto
ObjCBlockIndentWidth: 2
ObjCSpaceAfterProperty: false
ObjCSpaceBeforeProtocolList: true
PenaltyBreakAssignment: 2
PenaltyBreakBeforeFirstCallParameter: 19
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyBreakTemplateDeclaration: 10
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 60
PointerAlignment: Left
ReflowComments: true
SortIncludes: true
SortUsingDeclarations: true
SpaceAfterCStyleCast: false
SpaceAfterLogicalNot: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeCpp11BracedList: false
SpaceBeforeCtorInitializerColon: true
SpaceBeforeInheritanceColon: true
SpaceBeforeParens: ControlStatements
SpaceBeforeRangeBasedForLoopColon: true
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles: false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Cpp11
TabWidth: 8
UseTab: Never
...
")

        add_custom_target(format-check
            COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Checking code formatting compliance"
        )


        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Applying clang-format to source files"
        )

        # Create a combined analysis target
        add_custom_target(analyze)
        if(TARGET cppcheck)
            add_dependencies(analyze cppcheck)
        endif()
        if(TARGET format-check)
            add_dependencies(analyze format-check)
        endif()

    else()
        message(WARNING "clang-format not found, disable with -DENABLE_FORMAT=OFF")
    endif()
endif()

# ============================================================================
# TESTING AND QUALITY TARGETS
# ============================================================================


find_program(IWYU_EXE NAMES include-what-you-use iwyu)
if(IWYU_EXE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXE})
endif()


add_custom_target(quality
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running all quality checks"
)


enable_testing()


find_program(VALGRIND_EXE NAMES valgrind)
if(VALGRIND_EXE)
    add_custom_target(memory-check
        COMMAND ${VALGRIND_EXE} --leak-check=full --track-origins=yes ./dns_server
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS dns_server
        COMMENT "Running memory check with valgrind"
    )
endif()

# ============================================================================
# DOCUMENTATION
# ============================================================================

find_program(DOXYGEN_EXE NAMES doxygen)
if(DOXYGEN_EXE)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif()


message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
