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
public:
  SynthLeakageContractPass(std::string processorModuleName, llvm::raw_ostream &os) : os(os) {
    processorModule = processorModuleName;
  }
  void runOnOperation() override;
private:
  raw_ostream &os;


  DenseMap<size_t, hw::HWModuleOp> stages;
  DenseMap<size_t, seq::FirRegOp> pipelineRegisters; // TODO: suport more than just firreg?
  DenseMap<size_t, hw::WireOp> waitSignals;
  DenseMap<size_t, hw::WireOp> doneSignals;
  std::optional<SymbolTable> symTable;
  size_t nStages = 0;

public:

  bool isDataIndependent(mlir::Operation *op) {
    auto attr = op->getDiscardableAttr("synth.attributeEnum");
      if (attr) {
        synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
        if (synthAttr.getEnumAttr().getValue() == SynthEnumConst::DataSignal) {return false;}
        if (synthAttr.getEnumAttr().getValue() == SynthEnumConst::InstrSignal) {return true;}
      }
    for (Value operand : op->getOperands()) {
      return isDataIndependent(operand.getDefiningOp());
    }
    return true;
  }

  std::string analyseStage(size_t stageNumber) {
    os << "analyzing stage " << stageNumber << "\n";
    return "tmp"; //TODO: replace
  }

  void countModule(hw::HWModuleOp module) {
    auto tmp = module.getOperation()->getDiscardableAttr("synth.attributeEnum");
    if (tmp != nullptr) {
		synth::StageAttr attr = dyn_cast<synth::StageAttr>(tmp);
    	if (attr) {
      		size_t fromStage = attr.getFromStage();
            stages[fromStage] = module;
            if (fromStage > nStages) {nStages = fromStage;}
      		os << "registered module " << module.getName().str() << " as stage " << std::to_string(fromStage) << "\n";
      		return;
    	}
    }
    os << "registered module " << module.getName().str() << " is not a stage" << "\n";
  }

  void countAllModules(mlir::Block &block, std::string toplevelName) {
    for ( mlir::Operation &op : block.getOperations()) {
      hw::HWModuleOp moduleOp = dyn_cast<hw::HWModuleOp>(op);
      if (moduleOp && moduleOp.getName().str() != toplevelName) {
        countModule(moduleOp);
      }
    }
  }

  void registerAllModules(mlir::Operation &op) {
    symTable = std::optional(mlir::SymbolTable(&op));
  }
  };
} // namespace  

void SynthLeakageContractPass::runOnOperation() {
  if (getOperation().getName().str() != processorModule) {return;} // only keep the pass that runs on the processor operation

  countAllModules(*getOperation()->getBlock(), processorModule);

  registerAllModules(*getOperation().getOperation()->getParentOp());

  hw::HWModuleOp stage1 = stages.find((size_t)1)->second; // TODO: remove, simpele test stage access, werkt
  if (stage1) {auto a = stage1.getName().str();}

//  getOperation().walk(
//    [&](hw::HWModuleOp stage) {
//      mlir::Attribute attr = stage.getOperation()->getDiscardableAttr("synth.attributeEnum");
//      if (attr) {
//        synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
//        //if (!synthAttr) {return error() << "attributeEnum not of synth enum type";} //TODO: add check
//        size_t stageNumber = synthAttr.getFromStage();
//        stages[stageNumber] = stage;
//        if (stageNumber > nStages) {nStages = stageNumber;} // keep track of highest stage number seen
//      }
//    });

  // // walk over interstate registers
  // getOperation().walk(
  //   [&](seq::FirRegOp reg) {
  //     mlir::Attribute attr = reg.getOperation()->getDiscardableAttr("synth.attributeEnum");
  //     if (attr) {
  //       // TODO: check if present
  //       synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
  //       // TODO: check cast is okay
  //       size_t stageNumber = synthAttr.getFromStage();
  //       pipelineRegisters[stageNumber] = reg;
  //     }
  //   });

  // getOperation().walk( // walk over waitWires
  //   [&](hw::WireOp w) {
  //     mlir::Attribute attr = w.getOperation()->getDiscardableAttr("synth.attributeEnum");
  //     if (attr) {
  //       synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
  //       //if (!synthAttr) {return error() << "attributeEnum not of synth enum type";} //TODO: add check
  //       size_t stageNumber = synthAttr.getFromStage();
  //       waitSignals[stageNumber] = w;
  //     }
  //   });
    
  // now we have identified all pipeline stages and pipeline registers, we can analyse them 1 at a time
  std::string analysis = "";
  for (size_t s = 1; s <= nStages; s++) {
    hw::HWModuleOp stage = stages.find(s)->second;
    analysis += analyseStage(s);
    analysis += "\n";
  }
}

// Constructor with custom ostream
std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass(llvm::raw_ostream &os, std::string processorModule) {
  return std::make_unique<SynthLeakageContractPass>(processorModule, os);
}
// Basic constructor
std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass(std::string processorModule) {
  return std::make_unique<SynthLeakageContractPass>(processorModule, llvm::outs());
}