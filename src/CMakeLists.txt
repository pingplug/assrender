add_library(assrender SHARED render.c sub.c timecodes.c assrender.c)
target_link_libraries(assrender ${LIBASS_LDFLAGS} ${EXPAT_LIBRARY} ${ZLIB_LIBRARY} avisynth.lib)
