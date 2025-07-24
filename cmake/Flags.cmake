set(CMAKE_CXX_FLAGS_ASAN
    "-g -fsanitize=address,undefined -fno-sanitize-recover=all"
    CACHE STRING "Additional compiler flags in asan builds"
    FORCE
)

set(CMAKE_CXX_FLAGS_TSAN
    "-g -fsanitize=thread -fno-sanitize-recover=all"
    CACHE STRING "Additional compiler flags in asan builds"
    FORCE
)

set(CMAKE_CXX_FLAGS_DEBUG "-g"
    CACHE STRING "Additional compiler flags in debug builds"
    FORCE
)

set(CMAKE_CXX_FLAGS_RELEASE "-g"
    CACHE STRING "Additional compiler flags in debug builds"
    FORCE
)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror"
)
