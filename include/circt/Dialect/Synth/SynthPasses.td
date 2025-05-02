//===-- SynthPasses.td - Synth pass definition file ----------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the passes that work on the Synth dialect.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_SYNTHPASSES_TD
#define CIRCT_DIALECT_SYNTH_SYNTHPASSES_TD

include "mlir/Pass/PassBase.td"

/**
 * 
 */
def SynthGenerateLeakageContract : Pass<"synth-generate-leakage-contract", "hw::HWModuleOp"> {
  let summary = "TODO";
  let description = [{
    TODO
  }];
  let constructor = "circt::synth::createGenerateLeakageContractPass()";
  let dependentDialects = ["circt::hw::HWDialect"];//, "circt::synth::SynthDialect"]; //TODO: why does this cause error, and why does it work without?
    let options = [
    Option<"processorModule", "processor-module", "std::string",
           /*default=*/"\"test\"",
           "Name of the processor module to analyse.">,
    Option<"instruction", "instruction", "std::string",
           /*default=*/"\"test\"",
           "Name of the instruction to analyse.">,
  ];
}

#endif // CIRCT_DIALECT_SYNTH_SYNTHPASSES_TD
