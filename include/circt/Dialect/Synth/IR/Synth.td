//===- Synth.td - Synth top-level definition -------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is the top level file for the Synth dialect.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_IR_SYNTH_TD
#define CIRCT_DIALECT_SYNTH_IR_SYNTH_TD

include "mlir/IR/OpBase.td"
include "mlir/IR/OpAsmInterface.td"

include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

include "circt/Dialect/Synth/IR/SynthDialect.td"
include "circt/Dialect/Synth/IR/SynthTypes.td"
include "circt/Dialect/Synth/IR/SynthOps.td"
include "circt/Dialect/Synth/IR/SynthAttributes.td"

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTH_TD