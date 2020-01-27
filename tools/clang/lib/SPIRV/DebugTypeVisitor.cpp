//===--- LowerTypeVisitor.cpp - AST type to SPIR-V type impl -----*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include "DebugTypeVisitor.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/SPIRV/SpirvBuilder.h"
#include "clang/SPIRV/SpirvModule.h"
#include "llvm/ADT/SmallSet.h"

namespace clang {
namespace spirv {

SpirvDebugInstruction *
DebugTypeVisitor::lowerToDebugTypeEnum(const StructType *type) {
  return nullptr;
}

SpirvDebugInstruction *
DebugTypeVisitor::lowerToDebugTypeComposite(const StructType *type) {
  const auto &sm = astContext.getSourceManager();

  StringRef file = "";
  uint32_t line = 0;
  uint32_t column = 0;
  StringRef linkageName = type->getName();
  uint32_t tag = 1;
  bool isPrivate = false;

  const auto *decl = type->getDecl();
  const SourceLocation &loc = decl->getLocStart();
  file = sm.getPresumedLoc(loc).getFilename();
  line = sm.getPresumedLineNumber(loc);
  column = sm.getPresumedColumnNumber(loc);

  // TODO: Update linkageName using astContext.createMangleContext().
  //
  // Currently, the following code fails because it is not a
  // FunctionDecl nor VarDecl. I guess we should mangle a RecordDecl
  // as well.
  //
  // std::string s;
  // llvm::raw_string_ostream stream(s);
  // mangleCtx->mangleName(decl, stream);

  if (decl->isStruct())
    tag = 1;
  else if (decl->isClass())
    tag = 0;
  else if (decl->isUnion())
    tag = 2;
  else
    assert(!"DebugTypeComposite must be a struct, class, or union.");

  isPrivate = decl->isModulePrivate();

  // TODO: Update parent, size, and flags information correctly.
  auto &debugInfo = spvContext.getDebugInfo()[file];
  auto *dbgTyComposite =
      dyn_cast<SpirvDebugTypeComposite>(spvContext.getDebugTypeComposite(
          type, type->getName(), debugInfo.source, line, column,
          /* parent */ debugInfo.compilationUnit, linkageName,
          /* size */ 0,
          /* flags */ isPrivate ? 2u : 3u, tag));

  // If we already visited this composite type and its members,
  // we should skip it.
  auto &members = dbgTyComposite->getMembers();
  if (!members.empty())
    return dbgTyComposite;

  uint32_t sizeInBits = 0;
  uint32_t offsetInBits = 0;
  llvm::SmallSet<const FieldDecl *, 4> visited;

  auto fieldIt = type->getFields().begin();
  for (auto &memberDecl : decl->decls()) {
    if (const auto *cxxMethodDecl = dyn_cast<CXXMethodDecl>(memberDecl)) {
      auto *fn = spvContext.findFunctionInfo(cxxMethodDecl);
      assert(fn && "DebugFunction for method does not exist");
      members.push_back(fn);
      continue;
    }

    if (isa<CXXRecordDecl>(memberDecl))
      continue;

    auto &field = *fieldIt++;
    assert(isa<FieldDecl>(memberDecl) &&
           "Decl of member must be CXXMethodDecl, CXXRecordDecl, or FieldDecl");
    assert(memberDecl == field.decl &&
           "Field in SpirvType does not match to member decl");

    assert(field.decl && "Field must contain its declaration");
    auto cnt = visited.count(field.decl);
    if (cnt)
      continue;
    visited.insert(field.decl);

    SpirvDebugType *fieldTy =
        dyn_cast<SpirvDebugType>(lowerToDebugType(field.type));
    assert(fieldTy && "Field type must be SpirvDebugType");

    const SourceLocation &fieldLoc = field.decl->getLocStart();
    const StringRef fieldFile = sm.getPresumedLoc(fieldLoc).getFilename();
    const uint32_t fieldLine = sm.getPresumedLineNumber(fieldLoc);
    const uint32_t fieldColumn = sm.getPresumedColumnNumber(fieldLoc);

    uint32_t fieldSizeInBits = fieldTy->getSizeInBits();
    uint32_t fieldOffset =
        field.offset.hasValue() ? *field.offset : offsetInBits;

    llvm::Optional<SpirvInstruction *> value = llvm::None;
    if (const auto *varDecl = dyn_cast<VarDecl>(field.decl)) {
      if (const auto *val = varDecl->evaluateValue()) {
        if (val->isInt()) {
          *value = spvBuilder.getConstantInt(astContext.IntTy, val->getInt());
        } else if (val->isFloat()) {
          *value =
              spvBuilder.getConstantFloat(astContext.FloatTy, val->getFloat());
        }
        // TODO: Handle other constant types.
      }
    }

    auto *debugInstr =
        dyn_cast<SpirvDebugInstruction>(spvContext.getDebugTypeMember(
            field.name, fieldTy, spvContext.getDebugInfo()[fieldFile].source,
            fieldLine, fieldColumn, dbgTyComposite, fieldOffset,
            fieldSizeInBits, field.decl->isModulePrivate() ? 2u : 3u, value));
    assert(debugInstr && "We expect SpirvDebugInstruction for DebugTypeMember");
    debugInstr->setAstResultType(astContext.VoidTy);
    debugInstr->setResultType(spvContext.getVoidType());
    debugInstr->setInstructionSet(spvBuilder.getOpenCLDebugInfoExtInstSet());
    members.push_back(debugInstr);

    offsetInBits = fieldOffset + fieldSizeInBits;
    if (sizeInBits < offsetInBits)
      sizeInBits = offsetInBits;
  }
  dbgTyComposite->setSizeInBits(sizeInBits);
  return dbgTyComposite;
}

SpirvDebugInstruction *
DebugTypeVisitor::lowerToDebugType(const SpirvType *spirvType) {
  SpirvDebugInstruction *debugType = nullptr;

  switch (spirvType->getKind()) {
  case SpirvType::TK_Bool: {
    llvm::StringRef name = "bool";
    // TODO: Should we use 1 bit for booleans or 32 bits?
    uint32_t size = 32;
    // TODO: Use enums rather than uint32_t.
    uint32_t encoding = 2u;
    SpirvConstant *sizeInstruction = spvBuilder.getConstantInt(
        astContext.UnsignedIntTy, llvm::APInt(32, size));
    sizeInstruction->setResultType(spvContext.getUIntType(32));
    debugType = spvContext.getDebugTypeBasic(spirvType, name, sizeInstruction,
                                             encoding);
    break;
  }
  case SpirvType::TK_Integer: {
    auto *intType = dyn_cast<IntegerType>(spirvType);
    const uint32_t size = intType->getBitwidth();
    const bool isSigned = intType->isSignedInt();
    SpirvConstant *sizeInstruction = spvBuilder.getConstantInt(
        astContext.UnsignedIntTy, llvm::APInt(32, size));
    sizeInstruction->setResultType(spvContext.getUIntType(32));
    // TODO: Use enums rather than uint32_t.
    uint32_t encoding = isSigned ? 4u : 6u;
    std::string debugName = "";
    if (size == 32) {
      debugName = isSigned ? "int" : "uint";
    } else {
      std::ostringstream stream;
      stream << (isSigned ? "int" : "uint") << size << "_t";
      debugName = stream.str();
    }
    debugType = spvContext.getDebugTypeBasic(spirvType, debugName,
                                             sizeInstruction, encoding);
    break;
  }
  case SpirvType::TK_Float: {
    auto *floatType = dyn_cast<FloatType>(spirvType);
    const uint32_t size = floatType->getBitwidth();
    SpirvConstant *sizeInstruction = spvBuilder.getConstantInt(
        astContext.UnsignedIntTy, llvm::APInt(32, size));
    sizeInstruction->setResultType(spvContext.getUIntType(32));
    // TODO: Use enums rather than uint32_t.
    uint32_t encoding = 3u;
    std::string debugName = "";
    if (size == 32) {
      debugName = "float";
    } else {
      std::ostringstream stream;
      stream << "float" << size << "_t";
      debugName = stream.str();
    }
    debugType = spvContext.getDebugTypeBasic(spirvType, debugName,
                                             sizeInstruction, encoding);
    break;
  }
  case SpirvType::TK_Struct: {
    const auto *structType = dyn_cast<StructType>(spirvType);
    const auto *decl = structType->getDecl();
    if (decl) {
      if (decl->isEnum())
        debugType = lowerToDebugTypeEnum(structType);
      else
        debugType = lowerToDebugTypeComposite(structType);
    }
    break;
  }
  // TODO: Add DebugTypeComposite for class and union.
  // TODO: Add DebugTypeEnum.
  case SpirvType::TK_Array: {
    auto *arrType = dyn_cast<ArrayType>(spirvType);
    SpirvDebugInstruction *elemDebugType =
        lowerToDebugType(arrType->getElementType());
    debugType = spvContext.getDebugTypeArray(spirvType, elemDebugType,
                                             {arrType->getElementCount()});
    break;
  }
  case SpirvType::TK_Vector: {
    auto *vecType = dyn_cast<VectorType>(spirvType);
    SpirvDebugInstruction *elemDebugType =
        lowerToDebugType(vecType->getElementType());
    debugType = spvContext.getDebugTypeVector(spirvType, elemDebugType,
                                             vecType->getElementCount());
    break;
  }
  case SpirvType::TK_Pointer: {
    debugType = lowerToDebugType(
        dyn_cast<SpirvPointerType>(spirvType)->getPointeeType());
    break;
  }
  case SpirvType::TK_Function: {
    auto *fnType = dyn_cast<FunctionType>(spirvType);
    // Special case: There is no DebugType for void. So if the function return
    // type is void, we set it to nullptr.
    SpirvDebugInstruction *returnType =
        isa<VoidType>(fnType->getReturnType())
            ? nullptr
            : lowerToDebugType(fnType->getReturnType());
    llvm::SmallVector<SpirvDebugInstruction *, 4> params;
    for (const auto *paramType : fnType->getParamTypes())
      params.push_back(lowerToDebugType(paramType));
    // TODO: Add mechanism to properly calculate the flags.
    // The info needed probably resides in clang::FunctionDecl.
    // This info can either be stored in the SpirvFunction class. Or,
    // alternatively the info can be stored in the SpirvContext.
    const uint32_t flags = 3u;
    debugType =
        spvContext.getDebugTypeFunction(spirvType, flags, returnType, params);
    break;
  }
  }

  // TODO: When we emit all debug type completely, we should remove "Unknown"
  // type.
  if (!debugType) {
    debugType =
        spvContext.getDebugTypeBasic(nullptr, "Unknown", 0, 0 /*Unspecified*/);
  }

  debugType->setAstResultType(astContext.VoidTy);
  debugType->setResultType(context.getVoidType());
  debugType->setInstructionSet(spvBuilder.getOpenCLDebugInfoExtInstSet());
  return debugType;
}

bool DebugTypeVisitor::visitInstruction(SpirvInstruction *instr) {
  if (auto *debugInstr = dyn_cast<SpirvDebugInstruction>(instr)) {
    // Set the result type of debug instructions to OpTypeVoid.
    // According to the OpenCL.DebugInfo.100 spec, all debug instructions are
    // OpExtInst with result type of void.
    debugInstr->setAstResultType(astContext.VoidTy);
    debugInstr->setResultType(spvContext.getVoidType());
    debugInstr->setInstructionSet(spvBuilder.getOpenCLDebugInfoExtInstSet());

    // The following instructions are the only debug instructions that contain a
    // debug type:
    // DebugGlobalVariable
    // DebugLocalVariable
    // DebugFunction
    // DebugFunctionDeclaration
    // TODO: We currently don't have a SpirvDebugFunctionDeclaration class. Add
    // one if needed.
    if (isa<SpirvDebugGlobalVariable>(debugInstr) ||
        isa<SpirvDebugLocalVariable>(debugInstr)) {
      const SpirvType *spirvType = debugInstr->getDebugSpirvType();
      if (spirvType) {
        SpirvDebugInstruction *debugType = lowerToDebugType(spirvType);
        if (auto *var = dyn_cast<SpirvDebugGlobalVariable>(debugInstr)) {
          var->setDebugType(debugType);
        } else if (auto *var = dyn_cast<SpirvDebugLocalVariable>(debugInstr)) {
          var->setDebugType(debugType);
        } else {
          llvm_unreachable("Debug instruction must be DebugGlobalVariable or "
                           "DebugLocalVariable");
        }
      }
    }
    if (auto *debugFunction = dyn_cast<SpirvDebugFunction>(debugInstr)) {
      const SpirvType *spirvType =
          debugFunction->getSpirvFunction()->getFunctionType();
      if (spirvType) {
        SpirvDebugInstruction *debugType = lowerToDebugType(spirvType);
        debugFunction->setDebugType(debugType);
      }
    }
  }

  return true;
}

bool DebugTypeVisitor::visit(SpirvModule *module, Phase phase) {
  if (phase == Phase::Done) {
    // When the processing for all debug types is done, we need to take all the
    // debug types in the context and add their SPIR-V instructions to the
    // SPIR-V module.
    // Note that we don't add debug types to the module when we create them, as
    // there could be duplicates.
    for (const auto typePair : spvContext.getDebugTypes()) {
      module->addDebugInfo(typePair.second);
    }
    for (auto *type : spvContext.getMemberTypes()) {
      module->addDebugInfo(type);
    }
  }

  return true;
}

} // namespace spirv
} // namespace clang
