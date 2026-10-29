# List all .h/.hxx files in this directory
FILE(GLOB LIBPFS_H *.h)
FILE(GLOB LIBPFS_HXX *.hxx)
# List all .cpp files in this directory
FILE(GLOB LIBPFS_CPP *.cpp)

ADD_SUBDIRECTORY(manip)
ADD_SUBDIRECTORY(utils)
ADD_SUBDIRECTORY(tm)
ADD_SUBDIRECTORY(exif)
ADD_SUBDIRECTORY(colorspace)
ADD_SUBDIRECTORY(io)

ADD_LIBRARY(pfs STATIC ${LIBPFS_H} ${LIBPFS_HXX} ${LIBPFS_CPP})
# pfs uses Exiv2, OpenEXR, LibTIFF, libjpeg, LibRaw, LCMS, CFITSIO and the
# other image libraries in LIBS: declare them so every program or test that
# links pfs (in whatever order) also gets them
# no Qt: the image library is plain C++ (groundwork for the Mac app)
TARGET_LINK_LIBRARIES(pfs librtprocess ${LIBS})

SET(LUMINANCE_MODULES_GUI ${LUMINANCE_MODULES_GUI} pfs PARENT_SCOPE)
SET(LUMINANCE_MODULES_CLI ${LUMINANCE_MODULES_CLI} pfs PARENT_SCOPE)
