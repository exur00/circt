add_circt_dialect_library(CIRCTSynthTransforms
  GenerateLeakageContract.cpp

  DEPENDS
  CIRCTSynthTransformsIncGen

  LINK_LIBS PUBLIC
  CIRCTComb
  CIRCTEmit
  CIRCTHW
  CIRCTSeq
  CIRCTSupport
  CIRCTSV
  CIRCTSynth
  MLIRIR
  MLIRPass
  MLIRTransformUtils
)