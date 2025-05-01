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
#include "circt/Dialect/Synth/IR/DependencySemiLattice.h"
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
struct SynthLeakageContractPass : public circt::synth::impl::SynthGenerateLeakageContractBase<SynthLeakageContractPass> {
public:
  SynthLeakageContractPass(std::string processorModuleName, llvm::raw_ostream &os) : os(os) {
    processorModule = processorModuleName;
  }
  void runOnOperation() override;
private:
  raw_ostream &os;

  const std::string pipelineAttributeName = "synth.attributeEnum";
  const std::string dataDepAttributeName = "synth.dataDep";

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
  std::string currentInstruction = "ADD"; //TODO: replace by some way to loop for all instructions

public:

  std::optional<synth::DataDependencyEnum> getDataAttribute(mlir::Operation *op) {
    auto attr = op->getDiscardableAttr(dataDepAttributeName);
    if (!attr) {return std::nullopt;}
    synth::DataDependenciesAttr synthAttr = dyn_cast<DataDependenciesAttr>(attr);
    if (!synthAttr) {return std::nullopt;}
    return synthAttr.getDataDepEnum().getValue();
  }

  std::optional<bool> isDataDependent(mlir::Operation *op) {
    auto attr = getDataAttribute(op);
    if (attr == std::nullopt) {return std::nullopt;}
    DataDependencyEnum dataEnum = attr.value();
    switch(dataEnum) {
      case DataDependencyEnum::Data:
        return true;
      case DataDependencyEnum::Instruction:
        return false;
//      case DataDependencyEnum::ConstantSignal:
//        return false;
//      case DataDependencyEnum::ExternalSignal:
//        return false;
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
    auto attr = op->getDiscardableAttr(pipelineAttributeName);
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
    auto attr = stage->getDiscardableAttr(pipelineAttributeName);
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

        auto tmpAttr = module->getDiscardableAttr(pipelineAttributeName);
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
  
  bool transitionMatchesInstruction(mlir::Operation *op, std::string instruction) {
    // assumptions: either is marked with instruction(s), or is AND case, of which first operand is marked with instruction(s)
    return true; //TODO: implement
  }

  std::string analyseStage(size_t stageNumber) {
    os << "analyzing stage " << stageNumber << "\n";

    if (stageIsCombinational(stageNumber)) {
      os << "is combinational\n";
      return std::to_string(stageNumber) + " is combinational";
    }
    hw::HWModuleOp stage = stages.at(stageNumber);
    //TODO: first find FSM instance+module -> walk FSMOps en voeg toe aan FSMs map?
    fsm::HWInstanceOp fsmInstance;
    stage.walk(
      [&] (fsm::HWInstanceOp instance) {
        fsmInstance = instance;
      }
    );
    fsm::MachineOp fsm = fsmInstance.getMachineOp();
    if (!fsm) {
      os << "error, fsm in stage " << stageNumber << "is invalid or missing";
      return "error, fsm in stage " + std::to_string(stageNumber) + "is invalid or missing";
    }

    //TODO: match inputs, for each input: mark module side input data dependent by search if instance side is data dependent (kan general voor module : instanceOpInterface)
    //annotateInputs(fsmInstance, fsm); // TODO: dit moet dan ook nog eens gebeuren voor elke transition

    fsm::StateOp initialState = fsm.getInitialStateOp();
    std::vector<fsm::StateOp> checkedStates = {}; // TODO: gezien search, zou hashset in principe efficienter zijn voor grote hoeveelheid opties
    std::vector<fsm::StateOp> statesToCheck = {initialState};

    while (statesToCheck.size() > 0) {
      fsm::StateOp currentState = statesToCheck.back(); // TODO: last one because is most efficient?
      statesToCheck.pop_back(); // remove last state from list, we will be checking now.
      checkedStates.push_back(currentState); // mark this state is checked
      os << "currently analyzing state: " << currentState.getName() << "\n";

      mlir::Region &transitions = currentState.getTransitions();
      for (auto &transition : transitions.front().getOperations()) {
        fsm::TransitionOp transitionOp = dyn_cast<fsm::TransitionOp>(transition);
        if (!transitionOp) {os << "\terror: expected TransitionOp";} //TODO: error
        if (!transitionOp.hasGuard()) {
          os << "\ttransition to: " << transitionOp.getNextState() << " without guard\n";
          continue;
        }
        mlir::Region &guard = transitionOp.getGuard();
        auto returnOp = transitionOp.getGuardReturn();
        auto operands = returnOp.getOperation()->getOperands(); // always has 1 operand.
        auto decisionFunction = operands[0].getDefiningOp();
        if (!transitionMatchesInstruction(decisionFunction, currentInstruction)) {continue;}
        // TODO: check what decision depends on
        os << "\ttransition to: " << transitionOp.getNextState() << " depends on: " << "data" << "\n"; //TODO: add dependent
        auto nextState = transitionOp.getNextStateOp(); // = destination of this transition
        if (find(checkedStates.begin(), checkedStates.end(), nextState) == checkedStates.end()) { // if next state not already checked: add to check
          statesToCheck.push_back(nextState);
        }
      }
      // TODO: from the found options with their dependencies: launch analysis on those
      // TODO: keep track of all already checked stages: loops should only be analysed once.
    }
    os << "finished stage " + std::to_string(stageNumber) + " analysis\n";
    return "placeholder analysis stage " + std::to_string(stageNumber) + stages.find(stageNumber)->second.getName().str() + "\n"; //TODO: replace
  }

  void countModule(hw::HWModuleOp module) {
    auto tmp = module.getOperation()->getDiscardableAttr(pipelineAttributeName);
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
        mlir::Attribute attr = reg.getOperation()->getDiscardableAttr(pipelineAttributeName);
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
    user->setAttr(pipelineAttributeName, attr);
    //TODO: check if already marked, if so mark as least upper bound of those values
  }

  void markBlockInputUsers(size_t inputNumber, mlir::Block &block) { // TODO: add argument what to mark.
    mlir::Value::user_range users = block.getArgument(inputNumber).getUsers();
    for (mlir::Operation *user : users) {
      markOperation(user);
    }
  }

  void propagateRegisterAnnotations() {
    // TODO: First instance has no preceding pipeline register, instead read wires from instruction memory should be marked
    for (size_t stage = 2; stage <= nStages; stage++) {
      if (stageIsCombinational(stage)) {continue;}
      hw::InstanceOp instance = stageInstances.at(stage);
      hw::HWModuleOp module = stages.at(stage);
      auto instanceOperands = instance.getOperands();
      auto argnames = instance.getArgNames();
      auto modArgNames = ArrayAttr::get(instance->getContext(), module.getInputNames());
	    os << std::to_string(stage) << "\n";
      //for (Value operand : instance.getOperands()) {
      for (size_t i = 0; i < instanceOperands.size(); i++) { // TODO: start from 2 to ignore clock and reset signal?
        auto operand = instanceOperands[i]; // input of instanceOp
        auto operandName = dyn_cast<StringAttr>(argnames[i]).str();
        if (operandName == "clock" || operandName == "reset") {continue;} // skip analysis for clock and reset signal
        if (isDataDependentRecursive(operand.getDefiningOp())) {markBlockInputUsers(i, module.getBody().front());} // replace with lattice
      }
    }
  }

  bool instrSatisfiesCase(std::string instruction, mlir::Operation *instrCaseOp) {
    auto attr = instrCaseOp->getAttr("synth.instrCase");
    if (!attr) {os << "instruction case missing instruction attribute\n";} //TODO: error
    ArrayAttr instrCaseArrayAttr = dyn_cast<ArrayAttr>(attr);
    if (!instrCaseArrayAttr) {os << "instruction case missing instruction attribute\n";} // TODO: error
    auto instrCaseArray = instrCaseArrayAttr.getValue();
    for (auto atr : instrCaseArray) {
      if (dyn_cast<InstrAttr>(atr).getInstrName().compare(OpBuilder(instrCaseOp->getContext()).getStringAttr(instruction)) == 0) {//TODO: should check if cast is valid?
        return true;
      }
    }
    return false;
  }

  bool analyzeDecisionFunction(std::string current_instruction, mlir::Value decisionFunction) {
    //TODO: check is OR function (isa<comb.Or>)
    for (auto op : decisionFunction.getDefiningOp()->getOperands()) {
      //TODO: check is AND function, expect 2 Ops
      auto definingOp = op.getDefiningOp();
      if (definingOp->getNumOperands() != 2) {os << "decision function 2nd level AND does not have 2 operands";} // TODO: error
      auto instrCaseOp = definingOp->getOperand(0).getDefiningOp(); // 1st operand must always be the instruction case
      if (!instrSatisfiesCase(current_instruction, instrCaseOp)) {continue;}
      return isDataDependentRecursive(definingOp->getOperand(1).getDefiningOp()); // 2nd (and last) operand must be the other checks
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

  //if (!testPipelineValid()) {return;}
    
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