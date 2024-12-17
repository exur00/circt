//===- SynthTypes.td - Synth types -----------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This describes the Synth types.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_IR_SYNTHTYPES_TD
#define CIRCT_DIALECT_SYNTH_IR_SYNTHTYPES_TD

include "circt/Dialect/Synth/IR/SynthDialect.td"
include "mlir/IR/AttrTypeBase.td"

class SynthTypeDef<string name> : TypeDef<SynthDialect, name>;

def SequenceType : SynthTypeDef<"Sequence"> {
 let summary = "handle to a sequence closure";
 let description = [{
   An SSA value of this type refers to an `synth.sequence` operation and the
   argument values it should be invoked with (if it has any).
 }];

 let mnemonic = "sequence";
 let assemblyFormat = "";
}

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHTYPES_TD