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
  // for each stage t and t+1, pipeline[t] holds a vector containing all the registers 
  std::vector<std::vector<std::pair<seq::FirRegOp, synth::SynthEnumConst>>> pipelineRegisters;
  // for each instruction, instructions[i] holds a string 
  //DenseMap<std::string, std::string> instructions;
  hw::HWModuleOp processorModuleOp;
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

  bool isDataIndependent(size_t stageNumber, hw::WireOp value) {
    //std::vector<mlir::Operation> added = {}; // keep track of added op's to remove after test 
    addInstructionAssumptions(stageNumber);
    //addAssertion();
    return true;
  }

  void addInstructionAssumptions(size_t stageNumber) {
    hw::HWModuleOp moduleOp = stages.find(stageNumber)->second;

    //walk main block to find instanceOp with correct symbol
    for ( mlir::Operation &op : processorModuleOp->getRegion(0).front().getOperations()) {
      hw::InstanceOp instance = dyn_cast<hw::InstanceOp>(op);
      if (!instance) {continue;}
      auto tmp = instance.getInstanceName().str();
    }
    //find link from instanceOp to the registers providing input
  }

  std::string analyseStage(size_t stageNumber) {
    isDataIndependent(stageNumber, nullptr); // TODO: fix 2nd argument
    os << "analyzing stage " << stageNumber << "\n";
    return "placeholder analysis stage " + std::to_string(stageNumber) + stages.find(stageNumber)->second.getName().str() + "\n"; //TODO: replace
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
      if (moduleOp) {
        if(moduleOp.getName().str() == toplevelName) {
        	processorModuleOp = moduleOp;
        } else {
        	countModule(moduleOp);
        }
      }
    }
  }

  void addPipelineRegister(seq::FirRegOp reg, size_t fromStage, synth::SynthEnumConst enumValue) {
    pipelineRegisters[fromStage].push_back({reg, enumValue});
  }

  void countAllPipelineRegisters() {
    initializePipelineRegisters();

    getOperation().walk(
      [&](seq::FirRegOp reg) {
        mlir::Attribute attr = reg.getOperation()->getDiscardableAttr("synth.attributeEnum");
        if (attr) {
          synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
          if (synthAttr) {
            size_t fromStage = synthAttr.getFromStage();
            size_t toStage = synthAttr.getToStage();
            synth::SynthEnumConst enumValue = synthAttr.getEnumAttr().getValue();
            if (toStage != fromStage + 1) {return;} // TODO: throw error, invalid register configuration
            addPipelineRegister(reg, fromStage, enumValue);
          }
        }
      });
  }

  void registerAllModules(mlir::Operation &op) {
    symTable = std::optional(mlir::SymbolTable(&op));
  }

  bool testPipelineValid() {
    // test modules 1..nStages are present
    for (size_t i = 1; i <= nStages; i++) {
      hw::HWModuleOp stage = stages.find(i)->second;
      if (!stage) {
        os << "ERROR: stage " << i << " missing\n";
        return false;
      }
    }

    // test pipeline registers 1..(nStages-1) are present
    for (size_t i = 1; i < nStages; i++) {
      auto regs = pipelineRegisters[i];
      if (regs.size() == 0) {
        os << "ERROR: interstage pipelines " << i << " - " << i+1 << " missing\n";
        return false;
      }
    }
    return true;
  }

  void initializePipelineRegisters() {
    for (size_t i = 0; i <= nStages; i++){
      pipelineRegisters.push_back({});
    }
  }
  };
} // namespace  

void SynthLeakageContractPass::runOnOperation() {
  if (getOperation().getName().str() != processorModule) {return;} // only keep the pass that runs on the processor operation

  countAllModules(*getOperation()->getBlock(), processorModule);
  countAllPipelineRegisters();

  registerAllModules(*getOperation().getOperation()->getParentOp());

  if (!testPipelineValid()) {return;}
    
  // now we have identified all pipeline stages and pipeline registers, we can analyse them 1 at a time
  std::string analysis = "";
  for (size_t s = 1; s <= nStages; s++) {
    //hw::HWModuleOp stage = stages.find(s)->second;
    analysis += analyseStage(s);
    analysis += "\n";
  }
  os << "\n\nAnalysis:\n" << analysis; 
}

// Constructor with custom ostream
std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass(llvm::raw_ostream &os, std::string processorModule) {
  return std::make_unique<SynthLeakageContractPass>(processorModule, os);
}
// Basic constructor
std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass(std::string processorModule) {
  return std::make_unique<SynthLeakageContractPass>(processorModule, llvm::outs());
}