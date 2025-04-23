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



// Constant synth enum values.
def SynthEnumAttrImpl: I32EnumAttr<"SynthEnumConst", "synth enum constant",
                                [IntrFetch, ControlFSM, WaitSignal, InterstageReg, DataSignal, InstrSignal, CombinationalStage, ConstantSignal, ExternalSignal]> {
  let genSpecializedAttr = 0;
}

def SynthEnumAttr : EnumAttr<SynthDialect, SynthEnumAttrImpl,
                       "synth_enum_constant",  [TypedAttrInterface]> {
 let extraClassDeclaration = [{
    mlir::Type getType();
 }];
}

/// An attribute holding information about memory initialization. // TODO: change name
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

/// An attribute describing an instruction
def InstrAttribute : AttrDef<SynthDialect, "instr"> {
  let mnemonic = "instrAttr";
  let summary = "attribute describing an instruction";
  let description = [{TODO}];
  let parameters = (ins
    "StringRef":$instrName
  );
  let assemblyFormat = "`<` $instrName`>`";
}

}

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_TD