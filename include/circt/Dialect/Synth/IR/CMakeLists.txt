add_circt_dialect(Synth synth)

set(LLVM_TARGET_DEFINITIONS Synth.td)

add_circt_dialect_doc(Synth synth)

# test: copied from sv
mlir_tablegen(SynthAttributes.h.inc -gen-attrdef-decls --attrdefs-dialect=synth)
mlir_tablegen(SynthAttributes.cpp.inc -gen-attrdef-defs --attrdefs-dialect=synth)
add_public_tablegen_target(MLIRSynthAttributesIncGen)
add_dependencies(circt-headers MLIRSynthAttributesIncGen)