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
#include "circt/Dialect/HW/HWPasses.h"
#include "circt/Dialect/HW/HWTypes.h"
#include "mlir/Pass/Pass.h"

namespace circt {
namespace hw {
#define GEN_PASS_DEF_MYPASS
#include "circt/Dialect/HW/Passes.h.inc"
} // namespace hw
} // namespace circt

using namespace circt;
using namespace hw;

namespace {
// A test pass that simply replaces all wire names with myfoo_<n>
struct MyPass : circt::hw::impl::MyPassBase<MyPass> {
  void runOnOperation() override;
};
} // namespace

void MyPass::runOnOperation() {
  size_t nWires = 0; // Counts the number of wires modified
  getOperation().walk(
      [&](hw::WireOp wire) { // Walk over every wire in the module
        wire.setName("myfoo_" + std::to_string(nWires++)); // Rename said wire
      });
}

std::unique_ptr<mlir::Pass> circt::hw::createMyPass() {
  return std::make_unique<MyPass>();
}
