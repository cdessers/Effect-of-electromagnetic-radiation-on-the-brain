/**
 * Define some colors:
 */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/**
 * @brief Custom assert function.
 * 
 * CMake adds -DNDEBUG to the CMAKE_C_FLAGS_{RELEASE, MINSIZEREL} 
 * by default. So, you can use #ifndef NDEBUG.
 */

#define KRED  "\x1B[31m"
#define KNRM  "\x1B[0m"

/**
 * CUSTOM ASSERT WHEN CMAKE IS IN DEBUG MODE
 */
#ifndef NDEBUG
    #define ASSERT(left,operator,right) { \
        if(!((left) operator (right))){ \
            printf("%sASSERT FAILED: " #left "(%lf) " #operator " " #right "(%lf) @ %s:%d.%s\n",\
                    KRED,(double)(left),(double)(right),__FILE__,__LINE__,KNRM); abort(); }\
        }
#else
    #define ASSERT(left,operator,right) 
#endif


/**
 * CUSTOM ABORT FUNCTION
 */
#ifdef MPI_COMM_WORLD
        #define ABORT_MPI(ARG) MPI_Abort(MPI_COMM_WORLD,ARG);
#else
        #define ABORT_MPI(ARG) abort();
#endif

/**
 * DISPLAY A MESSAGE AND ABORT
 */
#define DISPLAY_ERROR_ABORT(...){              \
        fprintf(stderr,"%sIn %s :: ERROR :: %s",      \
                    ANSI_COLOR_RED,                   \
                    __FUNCTION__,                     \
                    ANSI_COLOR_GREEN);                \
        fprintf(stderr,__VA_ARGS__);           \
        fprintf(stderr,"%s Aborting.%s\nIn %s:%d\n",   \
                    ANSI_COLOR_YELLOW,                \
                    ANSI_COLOR_RESET,                 \
                    __FILE__,                         \
                    __LINE__);                        \
        ABORT_MPI(-1);                                \
        }

/**
 * DISPLAY A WARNING. DO NOT ABORT.
 */
#define DISPLAY_WARNING(...){                       \
        fprintf(stderr,"%sIn %s :: WARNING :: %s",  \
                    ANSI_COLOR_YELLOW,              \
                    __FUNCTION__,                   \
                    ANSI_COLOR_GREEN);              \
        fprintf(stderr,__VA_ARGS__);                \
        fprintf(stderr,"%sIn %s:%d\n",              \
                    ANSI_COLOR_RESET,               \
                    __FILE__,                       \
                    __LINE__);                      \
        }

/**
 * CUSTOM SIZE_T DATATYPE FOR MPI COMMUNICATION.
 */
#include <stdint.h>
#include <limits.h>

#if SIZE_MAX == UCHAR_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "what is happening here?"
#endif