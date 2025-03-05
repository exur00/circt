//===- FooWires.cpp - Replace all wire names with myfoo ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
//
// Replace all wire names with myfoo.
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/Synth/SynthPasses.h"
#include "circt/Dialect/HW/HWTypes.h"
#include "mlir/Pass/Pass.h"

namespace circt {
namespace synth {
#define GEN_PASS_DEF_SYNTHGENERATELEAKAGECONTRACT
#include "circt/Dialect/Synth/SynthPasses.h.inc"
} // namespace synth
} // namespace circt

using namespace circt;
using namespace synth;

namespace {
// A test pass that simply replaces all wire names with myfoo_<n>
struct SynthLeakageContractPass : public circt::synth::impl::SynthGenerateLeakageContractBase<SynthLeakageContractPass> {
  void runOnOperation() override;
};
} // namespace

void SynthLeakageContractPass::runOnOperation() {
  size_t nWires = 0; // Counts the number of wires modified
  getOperation().walk(
      [&](hw::WireOp wire) { // Walk over every wire in the module
        wire.setName("myfoo_" + std::to_string(nWires++)); // Rename said wire
      });
}

std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass() {
  return std::make_unique<SynthLeakageContractPass>();
}