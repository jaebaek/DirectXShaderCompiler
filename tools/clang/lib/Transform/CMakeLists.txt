set(CLANGDELTA_SRC "ClangDelta")
set(CLANGDELTA_INC "../../include/clang/Transform/ClangDelta")

set(LLVM_LINK_COMPONENTS
  Support
  )

include_directories(${CLANGDELTA_INC}/)

add_clang_library(clangTransform
  ${CLANGDELTA_SRC}/RewriteUtils.cpp
  ${CLANGDELTA_SRC}/SimplifyIf.cpp
  ${CLANGDELTA_SRC}/Transformation.cpp
  ${CLANGDELTA_SRC}/TransformationManager.cpp
  TransFormAction.cpp

  LINK_LIBS
  clangAST
  clangBasic
  clangFrontend
  clangLex
  )

