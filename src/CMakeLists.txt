SET(ASSRender_SRC 
    assrender.c
    assrender.h
    render.c
    render.h
    sub.c
    sub.h
    timecodes.c
    timecodes.h
    ASSRender.rc
    )

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(LIBASS REQUIRED libass>=0.12.0)

ADD_LIBRARY(assrender
    SHARED
    ${ASSRender_SRC}
    )

TARGET_LINK_LIBRARIES(assrender
    ${LIBASS_LDFLAGS}
    avisynth.lib
    )
