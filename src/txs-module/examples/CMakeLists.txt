build_lib_example(
  NAME basic-example
  SOURCE_FILES basic-example.cc
  LIBRARIES_TO_LINK
    ${libtxs-module}
)

build_lib_example(
  NAME mu-ul-example
  SOURCE_FILES mu-ul-example.cc
  LIBRARIES_TO_LINK
    ${libtxs-module}
)

build_lib_example(
  NAME triggered-txs-mode-1-example
  SOURCE_FILES triggered-txs-mode-1-example.cc
  LIBRARIES_TO_LINK
    ${libtxs-module}
)