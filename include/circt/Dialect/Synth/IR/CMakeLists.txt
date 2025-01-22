add_circt_dialect(Synth synth)
add_circt_dialect_doc(Synth synth)
add_dependencies(circt-headers MLIRSynthIncGen)

set(LLVM_TARGET_DEFINITIONS Synth.td)
mlir_tablegen(SynthEnums.h.inc -gen-enum-decls)
mlir_tablegen(SynthEnums.cpp.inc -gen-enum-defs)
add_public_tablegen_target(MLIRSynthEnumsIncGen)
add_dependencies(circt-headers MLIRSynthEnumsIncGen)

mlir_tablegen(SynthAttributes.h.inc -gen-attrdef-decls -attrdefs-dialect=synth)
mlir_tablegen(SynthAttributes.cpp.inc -gen-attrdef-defs -attrdefs-dialect=synth)
add_public_tablegen_target(MLIRSynthAttributesIncGen)
add_dependencies(circt-headers MLIRSynthAttributesIncGen)