
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBSNDFILE_PKGCONF sndfile)
endif(PKG_CONFIG_FOUND)

set (SNDFILE_WINDOWS_INSTALL "notSet" CACHE PATH "Where the files from mega-nerd is installed. Should have a 'bin', 'include', 'lib' directories")

find_path(SNDFILE_INCLUDE_DIR sndfile.h 
    HINTS ${LIBSNDFILE_PKGCONF_INCLUDE_DIRS} ${SNDFILE_WINDOWS_INSTALL}
    PATH_SUFFIXES include)

find_library(SNDFILE_LIBRARY sndfile libsndfile sndfile-1 libsndfile-1
    HINTS ${LIBSNDFILE_PKGCONF_INCLUDE_DIRS} ${SNDFILE_WINDOWS_INSTALL}
    PATH_SUFFIXES lib bin)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(SNDFILE DEFAULT_MSG SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR)

mark_as_advanced(SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR)

set(SNDFILE_LIBRARIES ${SNDFILE_LIBRARY})
set(SNDFILE_INCLUDE_DIRS ${SNDFILE_INCLUDE_DIR})

