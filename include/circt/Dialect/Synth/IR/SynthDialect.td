//===- SynthDialect.td - Synth dialect definition --------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the Synth dialect.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_IR_SYNTHDIALECT_TD
#define CIRCT_DIALECT_SYNTH_IR_SYNTHDIALECT_TD

include "mlir/IR/DialectBase.td"

def SynthDialect : Dialect {
  let name = "synth";

  let summary = "TODO";
  let description = [{
    TODO
  }];

  let useDefaultTypePrinterParser = 1;
  let cppNamespace = "::circt::synth";

  let extraClassDeclaration = [{
    void registerTypes();
    void registerAttributes();
  }];
}

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHDIALECT_TD