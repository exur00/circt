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
#include "circt/Dialect/Seq/SeqOps.h"
#include "circt/Dialect/Synth/SynthPasses.h"
#include "circt/Dialect/Synth/IR/SynthAttributes.h"
#include "circt/Dialect/HW/HWTypes.h"
#include "mlir/Pass/Pass.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Block.h"
#include "llvm/ADT/DenseMap.h"
#include "circt/Dialect/Verif/VerifOps.h"
#include "mlir/IR/Builders.h"

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

DenseMap<size_t, hw::HWModuleOp> stages;
DenseMap<size_t, seq::FirRegOp> pipelineRegisters; // TODO: suport more than just firreg?


void SynthLeakageContractPass::runOnOperation() {
  // size_t nWires = 0; // Counts the number of wires modified
  // getOperation().walk(
  //     [&](hw::WireOp wire) { // Walk over every wire in the module
  //       wire.setName("myfoo_" + std::to_string(nWires++)); // Rename said wire
  //       modules[nWires] = wire;
  //       hw::WireOp a = modules.at((size_t) 1);
  //     });

  // walk over modues (stages)
  size_t nStages = 0;

  getOperation().walk(
    [&](hw::HWModuleOp stage) {
      mlir::Attribute attr = stage.getOperation()->getDiscardableAttr("synth.attributeEnum");
      if (attr) {
        synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
        //if (!synthAttr) {return error() << "attributeEnum not of synth enum type";} //TODO: add check
        size_t stageNumber = synthAttr.getFromStage();
        stages[stageNumber] = stage;
        if (stageNumber > nStages) {nStages = stageNumber;} // keep track of highest stage number seen
      }
    });

  // walk over interstate registers
  getOperation().walk(
    [&](seq::FirRegOp reg) {
      mlir::Attribute attr = reg.getOperation()->getDiscardableAttr("synth.attributeEnum");
      if (attr) {
        // TODO: check if present
        synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
        // TODO: check cast is okay
        size_t stageNumber = synthAttr.getFromStage();
        pipelineRegisters[stageNumber] = reg;
      }
    });

  // now we have identified all pipeline stages and pipeline registers, we can analyse them 1 at a time
  for (size_t s = 1; s <= nStages; s++) {
    hw::HWModuleOp stage = stages[s];
    //Block stageBlock = stage.getBody().getBlocks().front();
    //auto stageBlock = stage->getRegions().front()->getBlocks().front();
    //auto stageBlock = stage.entry_block(stage);
    //auto stageBlock = stage.getRegion(0)->;
    auto stageBlock = stage.getOperation()->getRegions();
    
    auto builder = OpBuilder(stageBlock.front());
    //auto newModule = builder.create<verif::AssertOp>(stage.getLoc(), verif::AssertOp());
    //auto newModule = builder.create<hw::ConstantOp>(stage.getLoc(), IntegerAttr::get(I32Type, 5));
    auto test = builder.create<hw::ConstantOp>(stage.getLoc(), mlir::Builder(stage).getI8Attr(5));
    int a = 1+1;
    //auto builder = OpBuilder::atBlockEnd(stageBlock);

  }
}

std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass() {
  return std::make_unique<SynthLeakageContractPass>();
}