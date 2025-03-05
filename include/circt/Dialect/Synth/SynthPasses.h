//===- Passes.h - Synth pass entry points ------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This header file defines prototypes that expose pass constructors.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_SYNTHPASSES_H
#define CIRCT_DIALECT_SYNTH_SYNTHPASSES_H

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include <memory>

namespace circt {
namespace synth {
class FormalOp;
} // namespace synth
} // namespace circt

namespace circt {
namespace synth {

#define GEN_PASS_DECL_SYNTHGENERATELEAKAGECONTRACT
#include "circt/Dialect/Synth/SynthPasses.h.inc"

std::unique_ptr<mlir::Pass> createGenerateLeakageContractPass();

#define GEN_PASS_REGISTRATION
#include "circt/Dialect/Synth/SynthPasses.h.inc"

} // namespace synth
} // namespace circt

#endif // CIRCT_DIALECT_SYNTH_SYNTHPASSES_H