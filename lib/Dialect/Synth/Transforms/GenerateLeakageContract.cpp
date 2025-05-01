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

  const std::string pipelineAttributeName = "synth.attributeEnum"; //TODO: horen deze hier of bij hun definitie?
  const std::string dataDepAttributeName = "synth.dataDep";
  const std::string instrCaseName = "synth.instrCase";

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
  std::string currentInstruction = "MUL"; //TODO: replace by some way to loop for all instructions
  fsm::HWInstanceOp currentFSM;

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
      // TODO: add check that if is an unmarked module input / port, it is independent?
      bool inputsDataDependent = false;
      for (Value operand : op->getOperands()) {
        if (isDataDependentRecursive(operand.getDefiningOp())) {inputsDataDependent = true;}
      }
      return inputsDataDependent;
    }    
  }

  mlir::Operation *traceBlockArgumentFSM(mlir::Value val) {
    std::string str; //TODO: this is very dirty, but seemingly the only way? if operand is a block value of the fsm, this will print "<block argument> of type '[TYPE]' at index: x"
    // where x is the index of the matching input to the fsm::HWInstanceOp
    llvm::raw_string_ostream stream = llvm::raw_string_ostream(str);
    val.print(stream);
    auto inputNumString = str.substr(str.find_last_not_of("0123456789"));
    auto inputNum = std::stoi(inputNumString);
    return currentFSM.getOperation()->getOperand(inputNum).getDefiningOp();
  }

  Dependencies dataDependenciesRecursive(mlir::Operation *op) { 
    Dependencies deps = Dependencies(); // the constant dependencies
    if (isa<hw::ConstantOp>(*op)) {return deps;}
    auto attributeDeps = dependenciesUtils::fromOp(op); // if it is already marked, return that
    if (attributeDeps != std::nullopt) {
      return attributeDeps.value();
    }
    // TODO: add check that if is an unmarked module input / port, it is independent // really? is that safe?
    for (Value operand : op->getOperands()) {
      auto defOp = operand.getDefiningOp();
      if (!defOp) {
        defOp = traceBlockArgumentFSM(operand);
      }
      Dependencies operandDeps = dataDependenciesRecursive(defOp);
      deps = Dependencies::leastUpperBound(deps, operandDeps);
    }
    markOperation(op, deps);
    return deps;
  }

  synth::StageAttr getStageAttr(mlir::Operation *op) {
    auto attr = op->getDiscardableAttr(pipelineAttributeName);
    assert(attr);
    synth::StageAttr synthAttr = dyn_cast<StageAttr>(attr);
    assert(synthAttr);
    return synthAttr;
  }

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
        Operation *module;
        mlir::FlatSymbolRefAttr moduleRef = instance.getModuleNameAttr();
        mlir::SymbolTableCollection symTables;
        if (failed(hw::instance_like_impl::verifyReferencedModule(instance.getOperation(), symTables,
                                                            moduleRef, module))) {
          // TODO: error
          os << "error";
        }
        hw::HWModuleOp moduleOp = dyn_cast<hw::HWModuleOp>(*module);
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

  std::string analyseStage(size_t stageNumber) {
    os << "analyzing stage " << stageNumber << "\n";

    if (stageIsCombinational(stageNumber)) {
      os << "is combinational\n";
      return std::to_string(stageNumber) + " is combinational";
    }
    hw::HWModuleOp stage = stages.at(stageNumber);
    stage.walk(
      [&] (fsm::HWInstanceOp instance) {
        currentFSM = instance;
      }
    );
    fsm::MachineOp fsm = currentFSM.getMachineOp();
    if (!fsm) {
      os << "error, fsm in stage " << stageNumber << "is invalid or missing";
      return "error, fsm in stage " + std::to_string(stageNumber) + "is invalid or missing";
    }

    fsm::StateOp initialState = fsm.getInitialStateOp();
    std::vector<fsm::StateOp> checkedStates = {}; // TODO: this is searched, so with large numbers of operations a hashset could be faster
    std::vector<fsm::StateOp> statesToCheck = {initialState};

    while (statesToCheck.size() > 0) {
      fsm::StateOp currentState = statesToCheck.back();
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
        auto transitionDependencies = analyseDecisionFunction(currentInstruction, operands[0]);
        if (transitionDependencies == std::nullopt) {continue;}
        os << "\ttransition to: " << transitionOp.getNextState() << " depends on: " << transitionDependencies.value().toString() << "\n";
        auto nextState = transitionOp.getNextStateOp(); // = destination of this transition
        if (find(checkedStates.begin(), checkedStates.end(), nextState) == checkedStates.end()) { // if next state not already checked: add to check
          statesToCheck.push_back(nextState);
        }
      }
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
      		return;
    	}
    }
    os << "registered module " << module.getName().str() << " is not a stage" << "\n";
  }

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

  void markOperation(mlir::Operation *user, Dependencies deps) {
    auto currentDepsOpt = dependenciesUtils::fromOp(user);
    Dependencies currentDeps;
    if (currentDepsOpt == std::nullopt) {
      currentDeps = Dependencies();
    } else {
      currentDeps = currentDepsOpt.value();
    }
    Dependencies newDeps = Dependencies::leastUpperBound(currentDeps, deps);
    auto attr = dependenciesUtils::asAttribute(user->getContext(), newDeps);
    user->setDiscardableAttr(dataDepAttributeName, attr);
  }

  void markBlockInputUsers(size_t inputNumber, mlir::Block &block, Dependencies deps) {
    mlir::Value::user_range users = block.getArgument(inputNumber).getUsers();
    for (mlir::Operation *user : users) {
      markOperation(user, deps);
    }
  }

  void propagateRegisterAnnotations() {
    // First instance has no preceding pipeline register, instead read wires from instruction memory should be marked
    for (size_t stage = 2; stage <= nStages; stage++) {
      if (stageIsCombinational(stage)) {continue;}
      hw::InstanceOp instance = stageInstances.at(stage);
      hw::HWModuleOp module = stages.at(stage);
      auto instanceOperands = instance.getOperands();
      auto argnames = instance.getArgNames();
      auto modArgNames = ArrayAttr::get(instance->getContext(), module.getInputNames());
	  os << "propagating register annotations for stage " << std::to_string(stage) << "\n";
      for (size_t i = 0; i < instanceOperands.size(); i++) {
        auto operand = instanceOperands[i]; // input of instanceOp
        auto operandName = dyn_cast<StringAttr>(argnames[i]).str();
        if (operandName == "clock" || operandName == "reset") {continue;} // skip analysis for clock and reset signal
        if (isDataDependentRecursive(operand.getDefiningOp())) {markBlockInputUsers(i, module.getBody().front(), Dependencies(synth::DataDependencyEnum::Data));} // replace with properly done latice
        //Dependencies deps = dataDependenciesRecursive(operand.getDefiningOp()); // todo: this causes errors later on, why?
        //markBlockInputUsers(i, module.getBody().front(), deps);
      }
    }
  }

  bool instrSatisfiesCase(std::string instruction, mlir::Operation *instrCaseOp) {
    auto attr = instrCaseOp->getAttr(instrCaseName);
    assert (attr && "instruction case missing instruction attribute");
    ArrayAttr instrCaseArrayAttr = dyn_cast<ArrayAttr>(attr);
    assert (instrCaseArrayAttr && "instruction case missing instruction attribute");
    auto instrCaseArray = instrCaseArrayAttr.getValue();
    for (auto atr : instrCaseArray) {
      if (dyn_cast<InstrAttr>(atr).getInstrName().compare(OpBuilder(instrCaseOp->getContext()).getStringAttr(instruction)) == 0) {//TODO: should check if cast is valid?
        return true;
      }
    }
    return false;
  }

  // returns optional<dependencies> with nullopt if does not match, and Dependencies object if it does
  std::optional<Dependencies> analyseDecisionFunction(std::string current_instruction, mlir::Value decisionFunction) {
    assert(isa<comb::OrOp>(decisionFunction.getDefiningOp()));
    for (auto op : decisionFunction.getDefiningOp()->getOperands()) {
      assert(isa<comb::AndOp>(op.getDefiningOp()));
      auto definingOp = op.getDefiningOp();
      if (definingOp->getNumOperands() != 2) {os << "decision function 2nd level AND does not have 2 operands";} // TODO: error
      auto instrCaseOp = definingOp->getOperand(0).getDefiningOp(); // 1st operand must always be the instruction case
      if (!instrSatisfiesCase(current_instruction, instrCaseOp)) {continue;}
      Value otherRequirementsValue = definingOp->getOperand(1);
      auto otherRequirements = otherRequirementsValue.getDefiningOp();
      if (!otherRequirements) {otherRequirements = traceBlockArgumentFSM(otherRequirementsValue);}
      return dataDependenciesRecursive(otherRequirements);// 2nd (and last) operand must be the other checks
    }
    return std::nullopt;
  }

  };
} // namespace  

void SynthLeakageContractPass::runOnOperation() {
  if (getOperation().getName().str() != processorModule) {return;} // only keep the pass that runs on the processor operation

  registerInstances();
  countAllPipelineRegisters();

  propagateRegisterAnnotations();

//  if (!testPipelineValid()) {return;}
    
  // now we have identified all pipeline stages and pipeline registers, we can analyse them 1 at a time
  std::string analysis = "";
  for (size_t s = 1; s <= nStages; s++) {
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