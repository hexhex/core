/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

#pragma warning (disable: 4251)

/* Activates assertions and debug logging */
#define DEBUG /**/

/* Path of DLVDB executable */
/* #undef DLVDBPATH */

/* Version number of DLVHEX_ABI */
#define DLVHEX_ABI_VERSION 7.3.0

/* Major version number of DLVHEX_ABI */
#define DLVHEX_ABI_VERSION_MAJOR 7

/* Micro version number of DLVHEX_ABI */
#define DLVHEX_ABI_VERSION_MICRO 0

/* Minor version number of DLVHEX_ABI */
#define DLVHEX_ABI_VERSION_MINOR 3

/* Turns on benchmarking timers and counters */
#define DLVHEX_BENCHMARK /**/

/* Version number of DLVHEX */
#define DLVHEX_VERSION 2.3.0

/* Major version number of DLVHEX */
#define DLVHEX_VERSION_MAJOR 2

/* Micro version number of DLVHEX */
#define DLVHEX_VERSION_MICRO 0

/* Minor version number of DLVHEX */
#define DLVHEX_VERSION_MINOR 3

/* Path of DLV executable */
#define DLVPATH "dlv.exe"

/* Define to 1 if we compile with Python support. */
#define HAVE_PYTHON 1

/* Define to 1 if you have the `argz_add' function. */
#define HAVE_ARGZ_ADD 1

/* Define to 1 if you have the `argz_append' function. */
#define HAVE_ARGZ_APPEND 1

/* Define to 1 if you have the `argz_count' function. */
#define HAVE_ARGZ_COUNT 1

/* Define to 1 if you have the `argz_create_sep' function. */
#define HAVE_ARGZ_CREATE_SEP 1

/* Define to 1 if you have the <argz.h> header file. */
#define HAVE_ARGZ_H 1

/* Define to 1 if you have the `argz_insert' function. */
#define HAVE_ARGZ_INSERT 1

/* Define to 1 if you have the `argz_next' function. */
#define HAVE_ARGZ_NEXT 1

/* Define to 1 if you have the `argz_stringify' function. */
#define HAVE_ARGZ_STRINGIFY 1

/* Defined if the requested minimum BOOST version is satisfied */
#define HAVE_BOOST 1

/* Define to 1 if you have <boost/algorithm/string.hpp> */
#define HAVE_BOOST_ALGORITHM_STRING_HPP 1

/* Define to 1 if you have <boost/date_time/posix_time/posix_time.hpp> */
#define HAVE_BOOST_DATE_TIME_POSIX_TIME_POSIX_TIME_HPP 1

/* Define to 1 if you have <boost/filesystem/path.hpp> */
#define HAVE_BOOST_FILESYSTEM_PATH_HPP 1

/* Define to 1 if you have <boost/graph/adjacency_list.hpp> */
#define HAVE_BOOST_GRAPH_ADJACENCY_LIST_HPP 1

/* Define to 1 if you have <boost/iostreams/device/file_descriptor.hpp> */
#define HAVE_BOOST_IOSTREAMS_DEVICE_FILE_DESCRIPTOR_HPP 1

/* Define to 1 if you have <boost/program_options.hpp> */
#define HAVE_BOOST_PROGRAM_OPTIONS_HPP 1

/* Define to 1 if you have <boost/scoped_ptr.hpp> */
#define HAVE_BOOST_SCOPED_PTR_HPP 1

/* Define to 1 if you have <boost/shared_ptr.hpp> */
#define HAVE_BOOST_SHARED_PTR_HPP 1

/* Define to 1 if you have <boost/system/error_code.hpp> */
#define HAVE_BOOST_SYSTEM_ERROR_CODE_HPP 1

/* Define to 1 if you have <boost/test/unit_test.hpp> */
#define HAVE_BOOST_TEST_UNIT_TEST_HPP 1

/* Define to 1 if you have <boost/thread.hpp> */
#define HAVE_BOOST_THREAD_HPP 1

/* Define to 1 if you have <boost/tokenizer.hpp> */
#define HAVE_BOOST_TOKENIZER_HPP 1

/* Define to 1 if you have the `closedir' function. */
#define HAVE_CLOSEDIR 1

/* Define to 1 if you have the declaration of `cygwin_conv_path', and to 0 if
   you don't. */
/* #undef HAVE_DECL_CYGWIN_CONV_PATH */

/* Define to 1 if you have the <dirent.h> header file. */
//#define HAVE_DIRENT_H 1

/* Define if you have the GNU dld library. */
/* #undef HAVE_DLD */

/* Define to 1 if you have the <dld.h> header file. */
/* #undef HAVE_DLD_H */

/* Define to 1 if you have the `dlerror' function. */
#define HAVE_DLERROR 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Defined if we found dlv. */
#define HAVE_DLV 1

/* Defined if we found dlvdb. */
/* #undef HAVE_DLVDB */

/* Define to 1 if you have the <dl.h> header file. */
/* #undef HAVE_DL_H */

/* Define if you have the _dyld_func_lookup function. */
/* #undef HAVE_DYLD */

/* Define to 1 if the system has the type `error_t'. */
#define HAVE_ERROR_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
/* #define HAVE_INTTYPES_H 1 */

/* Defined if we have --with-libclasp. */
#define HAVE_LIBCLASP 1
#define GRINGO3 1	// msvc does not support the c++0x syntax used in gringo 4

/* Defined if we have --with-libclingo. */
/* #undef HAVE_LIBCLINGO */

/* Define to 1 if you have a functional curl library. */
#define HAVE_LIBCURL 1

/* Define if you have the libdl library or equivalent. */
#define HAVE_LIBDL 1

/* Define if libdlloader will be built on this platform */
#define HAVE_LIBDLLOADER 1

/* Defined if we found libdlv. */
/* #undef HAVE_LIBDLV */

/* Defined if we have --with-libgringo. */
#define HAVE_LIBGRINGO 1

/* Define this if a modern libltdl is already installed */
#define HAVE_LTDL 1

/* Define to 1 if you have the <mach-o/dyld.h> header file. */
/* #undef HAVE_MACH_O_DYLD_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the `opendir' function. */
#define HAVE_OPENDIR 1

/* Define if libtool can extract symbol lists from object files. */
#define HAVE_PRELOADED_SYMBOLS 1

/* Define to 1 if you have the `readdir' function. */
#define HAVE_READDIR 1

/* Define if you have the shl_load function. */
/* #undef HAVE_SHL_LOAD */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcat' function. */
/* #undef HAVE_STRLCAT */

/* Define to 1 if you have the `strlcpy' function. */
/* #undef HAVE_STRLCPY */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/dl.h> header file. */
/* #undef HAVE_SYS_DL_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #define HAVE_UNISTD_H 1 */

/* This value is set to 1 to indicate that the system argz facility works */
#define HAVE_WORKING_ARGZ 1

/* Defined if libcurl supports AsynchDNS */
/* #undef LIBCURL_FEATURE_ASYNCHDNS */

/* Defined if libcurl supports IDN */
#define LIBCURL_FEATURE_IDN 1

/* Defined if libcurl supports IPv6 */
#define LIBCURL_FEATURE_IPV6 1

/* Defined if libcurl supports KRB4 */
/* #undef LIBCURL_FEATURE_KRB4 */

/* Defined if libcurl supports libz */
#define LIBCURL_FEATURE_LIBZ 1

/* Defined if libcurl supports NTLM */
#define LIBCURL_FEATURE_NTLM 1

/* Defined if libcurl supports SSL */
#define LIBCURL_FEATURE_SSL 1

/* Defined if libcurl supports SSPI */
/* #undef LIBCURL_FEATURE_SSPI */

/* Defined if libcurl supports DICT */
#define LIBCURL_PROTOCOL_DICT 1

/* Defined if libcurl supports FILE */
#define LIBCURL_PROTOCOL_FILE 1

/* Defined if libcurl supports FTP */
#define LIBCURL_PROTOCOL_FTP 1

/* Defined if libcurl supports FTPS */
#define LIBCURL_PROTOCOL_FTPS 1

/* Defined if libcurl supports HTTP */
#define LIBCURL_PROTOCOL_HTTP 1

/* Defined if libcurl supports HTTPS */
#define LIBCURL_PROTOCOL_HTTPS 1

/* Defined if libcurl supports IMAP */
#define LIBCURL_PROTOCOL_IMAP 1

/* Defined if libcurl supports LDAP */
#define LIBCURL_PROTOCOL_LDAP 1

/* Defined if libcurl supports POP3 */
#define LIBCURL_PROTOCOL_POP3 1

/* Defined if libcurl supports RTSP */
#define LIBCURL_PROTOCOL_RTSP 1

/* Defined if libcurl supports SMTP */
#define LIBCURL_PROTOCOL_SMTP 1

/* Defined if libcurl supports TELNET */
#define LIBCURL_PROTOCOL_TELNET 1

/* Defined if libcurl supports TFTP */
#define LIBCURL_PROTOCOL_TFTP 1

/* Define if the OS needs help to load dependent libraries for dlopen(). */
/* #undef LTDL_DLOPEN_DEPLIBS */

/* Define to the system default library search path. */
#define LT_DLSEARCH_PATH "/lib:/usr/lib:/usr/lib/i386-linux-gnu/mesa:/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu:/lib/i686-linux-gnu:/usr/lib/i686-linux-gnu:/usr/local/lib:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu/mesa:/lib32:/usr/lib32"

/* The archive extension */
#define LT_LIBEXT "a"

/* The archive prefix */
#define LT_LIBPREFIX "lib"

/* Define to the extension used for runtime loadable modules, say, ".so". */
#define LT_MODULE_EXT ".so"

/* Define to the name of the environment variable that determines the run-time
   module search path. */
#define LT_MODULE_PATH_VAR "LD_LIBRARY_PATH"

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to the shared library suffix, say, ".dylib". */
/* #undef LT_SHARED_EXT */

/* Turns off debug logging and assertions */
/* #undef NDEBUG */

/* Define if dlsym() requires a leading underscore in symbol names. */
/* #undef NEED_USCORE */

/* Name of package */
#define PACKAGE "dlvhex"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "dlvhex-devel@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "dlvhex"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "dlvhex 2.3.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "dlvhex"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.3.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

// #define POSIX 1
#define WIN32 1
#define DLLIMPORT 1
// #define HAVE_MLP 1
// #define POSIX 1
#define USER_PLUGIN_DIR "dlvhex/plugins"

/* Version number of package */
#define VERSION "2.3.0"

/* Define so that glibc/gnulib argp.h does not typedef error_t. */
/* #undef __error_t_defined */

/* Define curl_free() as free() if our version of curl lacks curl_free. */
/* #undef curl_free */

/* Define to a type to use for `error_t' if it is not otherwise available. */
/* #undef error_t */
