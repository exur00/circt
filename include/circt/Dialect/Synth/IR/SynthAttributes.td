//===- SynthAttributes.td - Attributes for Synth dialect ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines Synth dialect specific attributes.
//
//===----------------------------------------------------------------------===//

// Include the definition of the necessary tablegen constructs for defining
// our attributes.
include "mlir/IR/AttrTypeBase.td"

// Here is a simple definition of an "integer" attribute, with a type and value parameter.
def SynthAttributeAttr : AttrDef<SynthDialect, "SynthAttribute"> {
  let summary = "An Attribute containing a integer value";
  let mnemonic = "attribute";
  let description = [{
    An integer attribute is a literal attribute that represents an integral
    value of the specified integer type.
  }];
  /// Here we've defined two parameters, one is a "self" type parameter, and the
  /// other is the integer value of the attribute. The self type parameter is
  /// specially handled by the assembly format.
  let parameters = (ins "::mlir::StringAttr":$name);

  /// Here we've defined a custom builder for the type, that removes the need to pass
  /// in an MLIRContext instance; as it can be infered from the `type`.
  let builders = [
    AttrBuilder<(ins "::llvm::StringRef":$name), [{
        return get($_ctxt, ::mlir::StringAttr::get($_ctxt, name));
    }]>,
    AttrBuilder<(ins "::mlir::StringAttr":$name), [{
        return get($_ctxt, name);
    }]>
  ];

  /// Here we define the textual format of the attribute declaratively, which will
  /// automatically generate parser and printer logic. This will allow for
  /// instances of the attribute to be output as, for example:
  ///
  ///    #my.int<50> : !my.int<32> // a 32-bit integer of value 50.
  ///
  /// Note that the self type parameter is not included in the assembly format.
  /// Its value is derived from the optional trailing type on all attributes.
  //let assemblyFormat = "`<` $name `>`";
  let assemblyFormat = "$name";
  //let hasCustomAssemblyFormat = true;

  /// Indicate that our attribute will add additional verification to the parameters.
  //let genVerifyDecl = 1;

  /// Indicate to the ODS generator that we do not want the default builders,
  /// as we have defined our own simpler ones.
  let skipDefaultBuilders = true;
}