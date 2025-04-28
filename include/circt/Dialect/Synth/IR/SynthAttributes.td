//===- SynthAttributes.td - Synth attributes -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Attribute definitions for the Synth dialect.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_TD
#define CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_TD

include "circt/Dialect/Synth/IR/SynthDialect.td"
include "mlir/IR/EnumAttr.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"

let cppNamespace = "circt::synth" in {

def IntrFetch: I32EnumAttrCase<"InstructionFetchRegister", 0, "instruction_fetch_register">;
def ControlFSM: I32EnumAttrCase<"ControlFSM", 1, "control_fsm">;
def WaitSignal: I32EnumAttrCase<"WaitSignal", 2, "wait_signal">;
def InterstageReg: I32EnumAttrCase<"InterstageReg", 3, "interstage_register">;
def DataSignal: I32EnumAttrCase<"DataSignal", 4, "data_signal">;
def InstrSignal: I32EnumAttrCase<"InstrSignal", 5, "instruction_signal">;
def CombinationalStage: I32EnumAttrCase<"CombinationalStage", 6, "combinational_stage">;
def ConstantSignal: I32EnumAttrCase<"ConstantSignal", 7, "constant_signal">;
def ExternalSignal: I32EnumAttrCase<"ExternalSignal", 8, "external_signal">;
def Stage : I32EnumAttrCase<"Stage", 9, "stage">;



// Constant synth enum values.
def SynthEnumAttrImpl: I32EnumAttr<"SynthEnumConst", "synth enum constant",
                                [IntrFetch, ControlFSM, WaitSignal, InterstageReg, DataSignal, InstrSignal, CombinationalStage, ConstantSignal, ExternalSignal, Stage]> {
  let genSpecializedAttr = 0;
}

def SynthEnumAttr : EnumAttr<SynthDialect, SynthEnumAttrImpl,
                       "synth_enum_constant",  [TypedAttrInterface]> {
 let extraClassDeclaration = [{
    mlir::Type getType();
 }];
}

/// An attribute holding information about pipeline structure. // TODO: change name
def StageAttribute : AttrDef<SynthDialect, "Stage"> {
  let mnemonic = "stageAttr";
  let summary = "TODO";
  let description = [{
    TODO
  }];
  let parameters = (ins
    "synth::SynthEnumConstAttr":$enumAttr,
    "int":$fromStage,
    "int":$toStage
  );
  let assemblyFormat = "`<` $enumAttr `,` $fromStage `,` $toStage `>`";
}

def Data: I32EnumAttrCase<"Data", 1, "data_signal">;
def Instruction: I32EnumAttrCase<"Instruction", 2, "instruction_signal">;
//TODO: add differentiation between reg addresses and contents, same with memory
// Constant synth enum values.
def DataDependencyEnumAttrImpl: I32EnumAttr<"DataDependencyEnum", "data dependency enum",
                                [Data, Instruction]> {
  let genSpecializedAttr = 0;
}

def DataDependencyEnumAttr : EnumAttr<SynthDialect, DataDependencyEnumAttrImpl,
                       "data_dependency_enum",  [TypedAttrInterface]> {
 let extraClassDeclaration = [{
    mlir::Type getType();
 }];
}

/// used to annotate on what sources a signal depends. empty means no the signal is constant
def DataDependenciesAttr : AttrDef<SynthDialect, "DataDependencies"> {
  let mnemonic = "dataDepAttr";
  let summary = "TODO";
  let description = [{TODO}];
  let parameters = (ins
       "synth::DataDependencyEnumAttr":$dataDepEnum
  );
  let assemblyFormat = "`<` $dataDepEnum`>`";
}

/// used to annotate states in the control FSM of a pipeline stage
// def FSMStateAttr : AttrDef<SynthDialect, "FSMState"> {
//   let mnemonic = "FSMStateAttr";
//   let summary = "TODO";
//   let description = [{TODO}];
//   let parameters = (ins
//   );
// }

/// An attribute describing an instruction
def InstrAttribute : AttrDef<SynthDialect, "instr"> {
  let mnemonic = "instrAttr";
  let summary = "attribute describing an instruction";
  let description = [{TODO}];
  let parameters = (ins
    "mlir::StringAttr":$instrName
  );
  let assemblyFormat = "`<` $instrName`>`";
}

}

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_TD