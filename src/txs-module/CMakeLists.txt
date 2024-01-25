set(gsl_libraries)
if(${GSL_FOUND})
  set(gsl_libraries
      ${GSL_LIBRARIES}
  )
endif()


build_lib(
  LIBNAME txs-module
  SOURCE_FILES 
    model/txs-ctrl-headers.cc
    model/txs-frame-exchange-manager.cc
    model/txs-multi-user-scheduler.cc
  HEADER_FILES 
    model/txs-ctrl-headers.h
    model/txs-frame-exchange-manager.h
    model/txs-multi-user-scheduler.h
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
  TEST_SOURCES
    test/txs-module-test-suite.cc
)
