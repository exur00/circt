set(LLVM_TARGET_DEFINITIONS SynthPasses.td)
mlir_tablegen(SynthPasses.h.inc -gen-pass-decls)
add_public_tablegen_target(CIRCTSynthTransformsIncGen)
add_dependencies(circt-headers CIRCTSynthTransformsIncGen)
add_circt_doc(SynthPasses SynthPasses -gen-pass-doc)

add_subdirectory(IR)