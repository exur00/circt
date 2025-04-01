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
#include "circt/Dialect/LTL/LTLTypes.h"
#include "circt/Dialect/LTL/LTLOps.h"
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
DenseMap<size_t, hw::WireOp> waitSignals;
DenseMap<size_t, hw::WireOp> doneSignals;

hw::WireOp addExtraHardware(mlir::OpBuilder builder, size_t stageNum) {
  auto name = builder.getStringAttr("done" + std::to_string(stageNum));
  hw::WireOp waitSignal = waitSignals[stageNum];
  auto tmp = waitSignal.getOperation();
  auto tmpBlock = tmp->getBlock();
  hw::WireOp done = builder.create<hw::WireOp>(
    waitSignal.getLoc(), waitSignal, name//, innerSym
  );
  doneSignals[stageNum] = done;
  //TODO: donttouch attribuut nodig?
  return done;
}

void addAssumptions(mlir::OpBuilder builder) {

}

void addAssertions(mlir::OpBuilder builder, hw::WireOp done) {
  // sequence "##1 done" betekent dat het klaar is in 1 cycle -> ##1 not done om te testen
  //ltl::SequenceType seq = ltl::SequenceType::get(done.getContext());
  auto loc = done.getLoc();
  auto delay = builder.getIntegerAttr(Builder(done).getIntegerType(64, false), 1);
  auto length = builder.getIntegerAttr(Builder(done).getIntegerType(64, false), 0);
  ltl::DelayOp seqOp = builder.create<ltl::DelayOp>(loc, done.getResult(), delay, length);//builder.getUI32IntegerAttr(1), builder.getUI32IntegerAttr(0));  
  
  //ltl::SequenceType seq = seqOp.getResult();
  //verif::AssertOp assert = builder.create<verif::AssertOp>(
  //  done.getLoc(), 
  //);
}

void SynthLeakageContractPass::runOnOperation() {
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

  getOperation().walk(
    [&](hw::WireOp w) {
      mlir::Attribute attr = w.getOperation()->getDiscardableAttr("synth.attributeEnum");
      if (attr) {
        synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
        //if (!synthAttr) {return error() << "attributeEnum not of synth enum type";} //TODO: add check
        size_t stageNumber = synthAttr.getFromStage();
        waitSignals[stageNumber] = w;
      }
    });

  // now we have identified all pipeline stages and pipeline registers, we can analyse them 1 at a time
  for (size_t s = 1; s <= nStages; s++) {
    hw::HWModuleOp stage = stages[s];
    auto stageBlock = stage.getOperation()->getRegions();
    
    mlir::OpBuilder builder = OpBuilder(stageBlock.front());
    //auto test = builder.create<hw::ConstantOp>(stage.getLoc(), builder.getI8Type(), 0);
    //TODO: check everything is present for each state!
    hw::WireOp doneSignal = addExtraHardware(builder, s);
    doneSignals[s] = doneSignal;
    addAssumptions(builder);
    addAssertions(builder, doneSignal);
  }
}

std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass() {
  return std::make_unique<SynthLeakageContractPass>();
}