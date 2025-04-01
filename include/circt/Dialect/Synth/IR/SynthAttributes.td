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
def PC: I32EnumAttrCase<"ProgramCounter", 1, "program_counter">;
def WaitSignal: I32EnumAttrCase<"WaitSignal", 2, "wait_signal">;
def InterstageReg: I32EnumAttrCase<"InterstageReg", 3, "interstage_register">;

// Constant synth enum values.
def SynthEnumAttrImpl: I32EnumAttr<"SynthEnumConst", "synth enum constant",
                                [IntrFetch, PC, WaitSignal, InterstageReg]> {
  let genSpecializedAttr = 0;
}

def SynthEnumAttr : EnumAttr<SynthDialect, SynthEnumAttrImpl,
                       "synth_enum_constant",  [TypedAttrInterface]> {
 let extraClassDeclaration = [{
    mlir::Type getType();
 }];
}

/// An attribute holding information about memory initialization.
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

}

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_TD