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

def IntrFetch: I32EnumAttrCase<"InstructionFetchRegister", 0, "instruction fetch register">;
def PC: I32EnumAttrCase<"ProgramCounter", 1, "program counter">;
def CommitSignal: I32EnumAttrCase<"CommitSignal", 2, "commit signal">;
def IIR: I32EnumAttrCase<"IIR", 3, "iir">;

// Constant synth enum values.
def SynthEnumAttrImpl: I32EnumAttr<"SynthEnumConst", "synth enum constant",
                                [IntrFetch, PC, CommitSignal, IIR]> {
  let genSpecializedAttr = 0;
}

def SynthEnumAttr : EnumAttr<SynthDialect, SynthEnumAttrImpl,
                       "synth_enum_constant",  [TypedAttrInterface]> {
 let extraClassDeclaration = [{
    mlir::Type getType();
 }];
}

}

// // It's common to define a base classes for attributes in the same dialect. This
// // removes the need to pass in the dialect for each attribute, and can also be used
// // to define a few fields ahead of time.
// class Synth_Attr<string name, string attrMnemonic, list<Trait> traits = []>
//     : AttrDef<SynthDialect, name, traits> {
//   let mnemonic = attrMnemonic;
// }

// // Here is a simple definition of an "integer" attribute, with a type and value parameter.
// def My_IntegerAttr : Synth_Attr<"Integer", "int"> {
//   let summary = "An Attribute containing a integer value";
//   let description = [{
//     An integer attribute is a literal attribute that represents an integral
//     value of the specified integer type.
//   }];
//   /// Here we've defined two parameters, one is a "self" type parameter, and the
//   /// other is the integer value of the attribute. The self type parameter is
//   /// specially handled by the assembly format.
//   let parameters = (ins AttributeSelfTypeParameter<"">:$type, "APInt":$value);

//   /// Here we've defined a custom builder for the type, that removes the need to pass
//   /// in an MLIRContext instance; as it can be infered from the `type`.
//   let builders = [
//     AttrBuilderWithInferredContext<(ins "Type":$type,
//                                         "const APInt &":$value), [{
//       return $_get(type.getContext(), type, value);
//     }]>
//   ];

//   /// Here we define the textual format of the attribute declaratively, which will
//   /// automatically generate parser and printer logic. This will allow for
//   /// instances of the attribute to be output as, for example:
//   ///
//   ///    #my.int<50> : !my.int<32> // a 32-bit integer of value 50.
//   ///
//   /// Note that the self type parameter is not included in the assembly format.
//   /// Its value is derived from the optional trailing type on all attributes.
//   let assemblyFormat = "`<` $value `>`";

//   /// Indicate that our attribute will add additional verification to the parameters.
//   let genVerifyDecl = 1;

//   /// Indicate to the ODS generator that we do not want the default builders,
//   /// as we have defined our own simpler ones.
//   let skipDefaultBuilders = 1;
// }

#endif // CIRCT_DIALECT_SYNTH_IR_SYNTHATTRIBUTES_TD