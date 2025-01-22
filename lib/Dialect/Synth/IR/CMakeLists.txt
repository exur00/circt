add_circt_dialect_library(CIRCTSynth
  SynthDialect.cpp
  SynthOps.cpp
  SynthTypes.cpp
  SynthAttributes.cpp

  ADDITIONAL_HEADER_DIRS
  ${CIRCT_MAIN_INCLUDE_DIR}/circt/Dialect/Synth/IR

  DEPENDS
  MLIRSynthIncGen
  MLIRSynthEnumsIncGen
  MLIRSynthAttributesIncGen

  LINK_COMPONENTS
  Support

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRBytecodeOpInterface
  MLIRInferTypeOpInterface
  MLIRSideEffectInterfaces
)

add_dependencies(circt-headers MLIRSynthIncGen)