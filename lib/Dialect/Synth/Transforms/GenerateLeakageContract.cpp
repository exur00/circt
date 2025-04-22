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
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/Synth/SynthPasses.h"
#include "circt/Dialect/Synth/IR/SynthAttributes.h"
#include "circt/Dialect/HW/HWTypes.h"
#include "circt/Dialect/HW/HWInstanceImplementation.h"
#include "circt/Dialect/FSM/FSMOps.h"
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
  DenseMap<size_t, hw::InstanceOp> stageInstances;
  DenseMap<size_t, fsm::MachineOp> stateMachines;
  DenseMap<size_t, fsm::HWInstanceOp> stateMachineInstances;
  // for each stage t and t+1, pipeline[t] holds a vector containing all the registers 
  std::vector<std::vector<std::pair<seq::FirRegOp, synth::SynthEnumConst>>> pipelineRegisters;
  // for each instruction, instructions[i] holds a string 
  //DenseMap<std::string, std::string> instructions;
  hw::HWModuleOp processorModuleOp;
  size_t nStages = 0;

public:

  std::optional<synth::SynthEnumConst> getDataAttribute(mlir::Operation *op) {
    auto attr = op->getDiscardableAttr("synth.attributeEnum");
    if (!attr) {return std::nullopt;}
    synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
    if (!synthAttr) {return std::nullopt;}
    return synthAttr.getEnumAttr().getValue();
  }

  std::optional<bool> isDataDependent(mlir::Operation *op) {
    auto attr = getDataAttribute(op);
    if (attr == std::nullopt) {return std::nullopt;}
    SynthEnumConst dataEnum = attr.value();
    switch(dataEnum) {
      case SynthEnumConst::DataSignal:
        return true;
      case SynthEnumConst::InstrSignal:
        return false;
      case SynthEnumConst::ConstantSignal:
        return false;
      case SynthEnumConst::ExternalSignal:
        return false;
      default:
        os << "unknown signal at:\n";
        op->print(os);
    }
  }

  bool isDataDependentRecursive(mlir::Operation *op) {
    if (isa<hw::ConstantOp>(*op)) {return false;}
    auto dataDependentAnnotation = isDataDependent(op);
    if (dataDependentAnnotation != std::nullopt) {
      return dataDependentAnnotation.value();
    } else {
      // TODO: add check that if is an unmarked module input / port, it is independent
      bool inputsDataDependent = false;
      for (Value operand : op->getOperands()) {
        if (isDataDependentRecursive(operand.getDefiningOp())) {inputsDataDependent = true;}
      }
      return inputsDataDependent;
    }    
  }

  synth::StageAttr getStageAttr(mlir::Operation *op) {
    auto attr = op->getDiscardableAttr("synth.attributeEnum");
    if (!attr) {
        //TODO: error instead of returning
      }
    synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
    if(!synthAttr) {
      //TODO: error instead of returning
    }
    return synthAttr;
  }

  // void annotateInputs(igraph::InstanceOpInterface instance, igraph::ModuleOpInterface module) {
  //   auto argnames = instance.getArgNames();
  //   auto modArgNames = ArrayAttr::get(instance->getContext(), module.getInputNames());
  //   os << std::to_string(i) << "\n";
  //   for (Value operand : instance.getOperands()) {
  //     // TODO: handle clock and reset signals as separate case
  //     if (Operation *defOp = operand.getDefiningOp()) {
  //         os << "Operand defined by: ";
  //         defOp->print(os);
  //         os << "\n";
  //       } else {
  //         os << "Operand is a block argument or undefined:\n";
  //         operand.print(os);
  //         os << "\n";
  //       }
  //     }
  // }

  bool stageIsCombinational(size_t stageNum) {
    hw::HWModuleOp stage = stages.at(stageNum);
    auto attr = stage->getDiscardableAttr("synth.attributeEnum");
    if (!attr) {return false;}
    synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
    if (!synthAttr) {return false;}
    return (synthAttr.getEnumAttr().getValue() == SynthEnumConst::CombinationalStage);
  }

  void registerInstances() {
    mlir::SymbolTableCollection symTables;
    
    getOperation().getOperation()->getParentOp()->walk(
      [&](hw::InstanceOp instance) {
        auto tmp = instance.getInstanceName().str(); // TODO: remove for debugging only

        Operation *module;
        mlir::FlatSymbolRefAttr moduleRef = instance.getModuleNameAttr();
        mlir::SymbolTableCollection symTables;
        if (failed(hw::instance_like_impl::verifyReferencedModule(instance.getOperation(), symTables,
                                                            moduleRef, module))) {
          // TODO: error
          os << "test";
        }
        hw::HWModuleOp moduleOp = dyn_cast<hw::HWModuleOp>(*module);
        auto tmp3 = moduleOp.getModuleName().str(); // TODO: remove for debugging only

        auto tmpAttr = module->getDiscardableAttr("synth.attributeEnum");
          if (tmpAttr != nullptr) {
          synth::StageAttr attr = dyn_cast<synth::StageAttr>(tmpAttr);
            if (attr) {
                size_t fromStage = attr.getFromStage();
                stageInstances[fromStage] = instance;
                os << "registered stageInstance for module " << moduleOp.getModuleName().str() << " as stage number " << std::to_string(fromStage) << "\n";
                countModule(moduleOp);
            }
          }
      });
  }

  std::string analyseStage(size_t stageNumber) {
    os << "analyzing stage " << stageNumber << "\n";

    if (stageIsCombinational(stageNumber)) {
      os << "is combinational\n";
      return std::to_string(stageNumber) + " is combinational";
    }
    hw::HWModuleOp stage = stages.at(stageNumber);
    //TODO: first find FSM instance+module -> walk FSMOps en voeg toe aan FSMs map?
    fsm::InstanceOp fsmInstance;
    stage.walk(
      [&] (fsm::InstanceOp instance) {
        fsmInstance = instance;
      }
    );
    fsm::MachineOp fsm = fsmInstance.getMachineOp();
    if (!fsm) {
      os << "error, referenced fsm invalid";
      return "error, referenced fsm invalid";
    }

    //TODO: match inputs, for each input: mark module side input data dependent by search if instance side is data dependent (kan general voor module : instanceOpInterface)
    //annotateInputs(fsmInstance, fsm);
    //TODO: analyze FSM MachineOp

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
          //symTables.getSymbolTable(module);
          //os << "added module to symbolTable\n";
      		return;
    	}
    }
    os << "registered module " << module.getName().str() << " is not a stage" << "\n";
  }

//  void countAllModules(mlir::Block &block, std::string toplevelName) {
//    for ( mlir::Operation &op : block.getOperations()) {
//      hw::HWModuleOp moduleOp = dyn_cast<hw::HWModuleOp>(op);
//      if (moduleOp) {
//        if(moduleOp.getName().str() == toplevelName) {
//        	processorModuleOp = moduleOp;
//        } else {
//        	countModule(moduleOp);
//        }
//      }
//    }
//  }

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

  // void registerAllModules(mlir::Operation *op) {
  //   symTables.getSymbolTable(op);
  // }

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

  void markOperation(mlir::Operation *user) {// TODO: add argument what to mark it
    SynthEnumConstAttr enumAttr = SynthEnumConstAttr::get(user->getContext(), SynthEnumConst::DataSignal); // TODO assign value based to mark based on argument
    auto attr = synth::StageAttr::get(user->getContext(), enumAttr, 0, 0);
    user->setAttr("synth.attributeEnum", attr);
  }

  void markBlockInputUsers(size_t inputNumber, mlir::Block &block) { // TODO: add argument what to mark.
    mlir::Value::user_range users = block.getArgument(inputNumber).getUsers();
    for (mlir::Operation *user : users) {
      markOperation(user);
    }
  }

  void propagateAnnotations() {
    // TODO: First instance has no preceding pipeline register, instead read wires from instruction memory should be marked
    for (size_t i = 2; i <= nStages; i++) {
      hw::InstanceOp instance = stageInstances.at(i);
      hw::HWModuleOp module = stages.at(i);
      auto instanceOperands = instance.getOperands();
      auto argnames = instance.getArgNames();
      auto modArgNames = ArrayAttr::get(instance->getContext(), module.getInputNames());
	    os << std::to_string(i) << "\n";
      //for (Value operand : instance.getOperands()) {
      for (size_t i = 0; i < instanceOperands.size(); i++) { // TODO: start from 2 to ignore clock and reset signal?
        auto operand = instanceOperands[i]; // input of instanceOp
        if (isDataDependentRecursive(operand.getDefiningOp())) {markBlockInputUsers(i, module.getBody().front());}

        //auto port = module.getPort(module.getPortIdForInputId(i)); // TODO: kijk waar je uitkomt met het terugkeren naar operanden vanuit de module.
        // auto& block = module.getBody().front(); // TODO: getBody should return a block (because HWModuleOp has the SingleBlock trait) but returns a region
        // mlir::Value blockInput = block.getArgument(i); // matching input inside moduleOp
        // auto users = blockInput.getUsers();
        // propagateAnnotations(operand, users);
        // for (auto user : users) {
        //   user->dump();
        // }


        // //TODO: remove, just for a test
        // module.walk(
        //   [&](comb::ICmpOp op) {
        //     mlir::Attribute attr = op.getOperation()->getDiscardableAttr("synth.attributeEnum");
        //     auto tmp = op.getOperation();
        //     if (attr) {
        //       synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
        //       if (synthAttr) {
        //         size_t fromStage = synthAttr.getFromStage();
        //         size_t toStage = synthAttr.getToStage();
        //         synth::SynthEnumConst enumValue = synthAttr.getEnumAttr().getValue();
        //       }
        //     }
        //   }
        // );

        // TODO: handle clock and reset signals as separate case
        if (Operation *defOp = operand.getDefiningOp()) {
            os << "Operand defined by: ";
            defOp->print(os);
            os << "\n";
          } else {
            os << "Operand is a block argument or undefined:\n";
            operand.print(os);
            os << "\n";
          }
        }
    }
  }

  bool instrSatisfiesCase(std::string instruction, mlir::Operation *instrCase) {
    auto attr = instrCase->getAttr("synth.attributeEnum");
    if (!attr) {os << "instruction case missing instruction attribute\n";} //TODO: error
    auto stageAttr = dyn_cast<StageAttr>(attr);
    if (!stageAttr) {os << "instruction case missing instruction attribute\n";} // TODO: error
    //TODO: replace this attribute to capture multiple possible instructions
    // TODO: make this check proper
    
    // for (auto instrAttr : instrAttrArray) {
    //   if (instrAttr.instruction().getValue() == instruction) { //TODO: bij grouping: vervang dit door .contains ofzo
    //     return true;
    //   }
    // }
    return false;
  }

  bool analyzeDecisionFunction(std::string current_instruction, mlir::Value decisionFunction) {
    //TODO: check is OR function
    for (auto op : decisionFunction.getDefiningOp()->getOperands()) {
      //TODO: check is AND function, expect 2 Ops
      auto definingOp = op.getDefiningOp();
      if (definingOp->getNumOperands() != 2) {os << "decision function 2nd level AND does not have 2 operands";} // TODO: error
      auto instrCase = definingOp->getOperand(0).getDefiningOp();
      if (!instrSatisfiesCase(current_instruction, instrCase)) {continue;}
      return isDataDependentRecursive(definingOp->getOperand(1).getDefiningOp());
    }
  }

  };
} // namespace  

void SynthLeakageContractPass::runOnOperation() {
  if (getOperation().getName().str() != processorModule) {return;} // only keep the pass that runs on the processor operation

  //countAllModules(*getOperation()->getBlock(), processorModule);
  registerInstances();
  countAllPipelineRegisters();

  propagateRegisterAnnotations();

  if (!testPipelineValid()) {return;}
    
  // now we have identified all pipeline stages and pipeline registers, we can analyse them 1 at a time
  std::string analysis = "";
  for (size_t s = 1; s <= nStages; s++) {
    //hw::HWModuleOp stage = stages.find(s)->second;
    analysis += analyseStage(s);
    analysis += "\n";
  }
  os << "\n\nAnalysis results:\n" << analysis;
}

// Constructor with custom ostream
std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass(llvm::raw_ostream &os, std::string processorModule) {
  return std::make_unique<SynthLeakageContractPass>(processorModule, os);
}
// Basic constructor
std::unique_ptr<mlir::Pass> circt::synth::createGenerateLeakageContractPass(std::string processorModule) {
  return std::make_unique<SynthLeakageContractPass>(processorModule, llvm::outs());
}