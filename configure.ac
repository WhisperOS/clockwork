#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_CONFIG_SRCDIR(src)
AC_CONFIG_AUX_DIR(build/aux)
AC_CONFIG_HEADERS(config.h)

AC_INIT(clockwork, 0.2.8, bugs@niftylogic.com)
AM_INIT_AUTOMAKE(0.2.8)


# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB(ssl,
	SSL_library_init,,
	AC_MSG_ERROR(Missing OpenSSL libraries))

AC_CHECK_LIB(crypto,
	CRYPTO_lock,,
	AC_MSG_ERROR(Missing OpenSSL libraries))

AC_CHECK_LIB(pthread,
	pthread_create,,
	AC_MSG_ERROR(Missing pthread library))

AC_CHECK_LIB(readline,
	read_history,,
	AC_MSG_ERROR(Missing GNU readline library))

AC_CHECK_LIB(sqlite3,
	sqlite3_open,,
	AC_MSG_ERROR(Missing SQLite3 library))

AC_CHECK_LIB(augeas,
	aug_init,,
	AC_MSG_ERROR(Missing Augeas library))

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h shadow.h stddef.h stdint.h])
AC_CHECK_HEADERS([stdlib.h string.h sys/socket.h sys/time.h syslog.h])
AC_CHECK_HEADERS([termios.h unistd.h libxml/tree.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 gettimeofday memset mkdir rmdir select strcasecmp strdup strerror strtol strtoul strtoull uname])

AC_OUTPUT(Makefile)