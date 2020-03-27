//===-- SpirvContext.h - Context holding SPIR-V codegen data ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_SPIRV_SPIRVCONTEXT_H
#define LLVM_CLANG_SPIRV_SPIRVCONTEXT_H

#include <array>

#include "dxc/DXIL/DxilShaderModel.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/SPIRV/SpirvInstruction.h"
#include "clang/SPIRV/SpirvType.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/Support/Allocator.h"

namespace clang {
namespace spirv {

struct RichDebugInfo {
  RichDebugInfo(SpirvDebugSource *src, SpirvDebugCompilationUnit *cu)
      : source(src), compilationUnit(cu) {
    scopeStack.push_back(cu);
  }
  RichDebugInfo() : source(nullptr), compilationUnit(nullptr), scopeStack() {}

  // The HLL source code
  SpirvDebugSource *source;

  // The compilation unit (topmost debug info node)
  SpirvDebugCompilationUnit *compilationUnit;

  // Stack of lexical scopes
  std::vector<SpirvDebugInstruction *> scopeStack;
};

// Provides DenseMapInfo for spv::StorageClass so that we can use
// spv::StorageClass as key to DenseMap.
//
// Mostly from DenseMapInfo<unsigned> in DenseMapInfo.h.
struct StorageClassDenseMapInfo {
  static inline spv::StorageClass getEmptyKey() {
    return spv::StorageClass::Max;
  }
  static inline spv::StorageClass getTombstoneKey() {
    return spv::StorageClass::Max;
  }
  static unsigned getHashValue(const spv::StorageClass &Val) {
    return static_cast<unsigned>(Val) * 37U;
  }
  static bool isEqual(const spv::StorageClass &LHS,
                      const spv::StorageClass &RHS) {
    return LHS == RHS;
  }
};

// Provides DenseMapInfo for ArrayType so we can create a DenseSet of array
// types.
struct ArrayTypeMapInfo {
  static inline ArrayType *getEmptyKey() { return nullptr; }
  static inline ArrayType *getTombstoneKey() { return nullptr; }
  static unsigned getHashValue(const ArrayType *Val) {
    return llvm::hash_combine(Val->getElementType(), Val->getElementCount(),
                              Val->getStride().hasValue());
  }
  static bool isEqual(const ArrayType *LHS, const ArrayType *RHS) {
    // Either both are null, or both should have the same underlying type.
    return (LHS == RHS) || (LHS && RHS && *LHS == *RHS);
  }
};

// Provides DenseMapInfo for RuntimeArrayType so we can create a DenseSet of
// runtime array types.
struct RuntimeArrayTypeMapInfo {
  static inline RuntimeArrayType *getEmptyKey() { return nullptr; }
  static inline RuntimeArrayType *getTombstoneKey() { return nullptr; }
  static unsigned getHashValue(const RuntimeArrayType *Val) {
    return llvm::hash_combine(Val->getElementType(),
                              Val->getStride().hasValue());
  }
  static bool isEqual(const RuntimeArrayType *LHS,
                      const RuntimeArrayType *RHS) {
    // Either both are null, or both should have the same underlying type.
    return (LHS == RHS) || (LHS && RHS && *LHS == *RHS);
  }
};

// Provides DenseMapInfo for ImageType so we can create a DenseSet of
// image types.
struct ImageTypeMapInfo {
  static inline ImageType *getEmptyKey() { return nullptr; }
  static inline ImageType *getTombstoneKey() { return nullptr; }
  static unsigned getHashValue(const ImageType *Val) {
    return llvm::hash_combine(Val->getSampledType(), Val->isArrayedImage(),
                              Val->isMSImage(),
                              static_cast<uint32_t>(Val->getDimension()),
                              static_cast<uint32_t>(Val->withSampler()),
                              static_cast<uint32_t>(Val->getImageFormat()));
  }
  static bool isEqual(const ImageType *LHS, const ImageType *RHS) {
    // Either both are null, or both should have the same underlying type.
    return (LHS == RHS) || (LHS && RHS && *LHS == *RHS);
  }
};

// Provides DenseMapInfo for FunctionType so we can create a DenseSet of
// function types.
struct FunctionTypeMapInfo {
  static inline FunctionType *getEmptyKey() { return nullptr; }
  static inline FunctionType *getTombstoneKey() { return nullptr; }
  static unsigned getHashValue(const FunctionType *Val) {
    // Hashing based on return type and number of function parameters.
    auto hashCode =
        llvm::hash_combine(Val->getReturnType(), Val->getParamTypes().size());
    for (const SpirvType *paramType : Val->getParamTypes())
      hashCode = llvm::hash_combine(hashCode, paramType);
    return hashCode;
  }
  static bool isEqual(const FunctionType *LHS, const FunctionType *RHS) {
    // Either both are null, or both should have the same underlying type.
    return (LHS == RHS) || (LHS && RHS && *LHS == *RHS);
  }
};

/// The class owning various SPIR-V entities allocated in memory during CodeGen.
///
/// All entities should be allocated from an object of this class using
/// placement new. This way other components of the CodeGen do not need to worry
/// about lifetime of those SPIR-V entities. They will be deleted when such a
/// context is deleted. Therefore, this context should outlive the usages of the
/// the SPIR-V entities allocated in memory.
class SpirvContext {
public:
  using ShaderModelKind = hlsl::ShaderModel::Kind;
  SpirvContext();
  ~SpirvContext() = default;

  // Forbid copy construction and assignment
  SpirvContext(const SpirvContext &) = delete;
  SpirvContext &operator=(const SpirvContext &) = delete;

  // Forbid move construction and assignment
  SpirvContext(SpirvContext &&) = delete;
  SpirvContext &operator=(SpirvContext &&) = delete;

  /// Allocates memory of the given size and alignment.
  void *allocate(size_t size, unsigned align) const {
    return allocator.Allocate(size, align);
  }

  /// Deallocates the memory pointed by the given pointer.
  void deallocate(void *ptr) const {}

  // === DebugTypes ===

  // TODO: Replace uint32_t with an enum for encoding.
  SpirvDebugInstruction *getDebugTypeBasic(const SpirvType *spirvType,
                                           llvm::StringRef name,
                                           SpirvConstant *size,
                                           uint32_t encoding);

  SpirvDebugInstruction *
  getDebugTypeMember(llvm::StringRef name, const SpirvType *type,
                     SpirvDebugSource *source, uint32_t line, uint32_t column,
                     SpirvDebugInstruction *parent, uint32_t flags,
                     uint32_t offset = UINT32_MAX,
                     const APValue *value = nullptr);

  SpirvDebugInstruction *
  getDebugTypeComposite(const SpirvType *spirvType, llvm::StringRef name,
                        SpirvDebugSource *source, uint32_t line,
                        uint32_t column, SpirvDebugInstruction *parent,
                        llvm::StringRef linkageName, uint32_t size,
                        uint32_t flags, uint32_t tag);

  SpirvDebugInstruction *getDebugType(const SpirvType *spirvType);

  SpirvDebugInstruction *getDebugTypeArray(const SpirvType *spirvType,
                                           SpirvDebugInstruction *elemType,
                                           llvm::ArrayRef<uint32_t> elemCount);

  SpirvDebugInstruction *getDebugTypeVector(const SpirvType *spirvType,
                                            SpirvDebugInstruction *elemType,
                                            uint32_t elemCount);

  SpirvDebugInstruction *
  getDebugTypeFunction(const SpirvType *spirvType, uint32_t flags,
                       SpirvDebugType *ret,
                       llvm::ArrayRef<SpirvDebugType *> params);

  SpirvDebugInstruction *getDebugTypeTemplate(const SpirvType *spirvType,
                                              SpirvDebugInstruction *target);

  SpirvDebugInstruction *getDebugTypeTemplateParameter(
    const SpirvType *parentType,
      llvm::StringRef name, const SpirvType *type, SpirvInstruction *value,
      SpirvDebugSource *source, uint32_t line, uint32_t column);

  llvm::MapVector<const SpirvType *, SpirvDebugType *> &getDebugTypes() {
    return debugTypes;
  }

  llvm::SmallVector<SpirvDebugInstruction *, 16> &getTailDebugTypes() {
    return tailDebugTypes;
  }

  // === Types ===

  const VoidType *getVoidType() const { return voidType; }
  const BoolType *getBoolType() const { return boolType; }
  const IntegerType *getSIntType(uint32_t bitwidth);
  const IntegerType *getUIntType(uint32_t bitwidth);
  const FloatType *getFloatType(uint32_t bitwidth);

  const VectorType *getVectorType(const SpirvType *elemType, uint32_t count);
  // Note: In the case of non-floating-point matrices, this method returns an
  // array of vectors.
  const SpirvType *getMatrixType(const SpirvType *vecType, uint32_t vecCount);

  const ImageType *getImageType(const SpirvType *, spv::Dim,
                                ImageType::WithDepth, bool arrayed, bool ms,
                                ImageType::WithSampler sampled,
                                spv::ImageFormat);
  const SamplerType *getSamplerType() const { return samplerType; }
  const SampledImageType *getSampledImageType(const ImageType *image);
  const HybridSampledImageType *getSampledImageType(QualType image);

  const ArrayType *getArrayType(const SpirvType *elemType, uint32_t elemCount,
                                llvm::Optional<uint32_t> arrayStride);
  const RuntimeArrayType *
  getRuntimeArrayType(const SpirvType *elemType,
                      llvm::Optional<uint32_t> arrayStride);

  const StructType *getStructType(
      llvm::ArrayRef<StructType::FieldInfo> fields, llvm::StringRef name,
      bool isReadOnly = false,
      StructInterfaceType interfaceType = StructInterfaceType::InternalStorage);

  void saveFunctionInfo(const CXXMethodDecl *decl, SpirvDebugFunction *fn) {
    methodDeclToDebugFunction[decl] = fn;
  }
  SpirvDebugFunction *findFunctionInfo(const CXXMethodDecl *decl) {
    auto it = methodDeclToDebugFunction.find(decl);
    if (it != methodDeclToDebugFunction.end())
      return it->second;
    return nullptr;
  }

  const SpirvPointerType *getPointerType(const SpirvType *pointee,
                                         spv::StorageClass);

  FunctionType *getFunctionType(const SpirvType *ret,
                                llvm::ArrayRef<const SpirvType *> param,
                                bool isMember = false);

  const StructType *getByteAddressBufferType(bool isWritable);
  const StructType *getACSBufferCounterType();

  const AccelerationStructureTypeNV *getAccelerationStructureTypeNV() const {
    return accelerationStructureTypeNV;
  }

  /// --- Hybrid type getter functions ---
  ///
  /// Concrete SpirvType objects represent a SPIR-V type completely. Hybrid
  /// SpirvTypes, however, represent a QualType that can later be lowered to a
  /// concrete SpirvType.
  ///
  /// For example, the caller may want to get a SpirvType for a pointer in which
  /// the pointee is a QualType. This would be a HybridPointerType, which can
  /// later be lowered to a SpirvPointerType by lowereing the pointee from
  /// QualType to SpirvType).
  const HybridStructType *getHybridStructType(
      llvm::ArrayRef<HybridStructType::FieldInfo> fields, llvm::StringRef name,
      bool isReadOnly = false,
      StructInterfaceType interfaceType = StructInterfaceType::InternalStorage);

  const HybridPointerType *getPointerType(QualType pointee, spv::StorageClass);

  /// Functions to get/set current entry point ShaderModelKind.
  ShaderModelKind getCurrentShaderModelKind() { return curShaderModelKind; }
  void setCurrentShaderModelKind(ShaderModelKind smk) {
    curShaderModelKind = smk;
  }
  /// Functions to get/set hlsl profile version.
  uint32_t getMajorVersion() const { return majorVersion; }
  void setMajorVersion(uint32_t major) { majorVersion = major; }
  uint32_t getMinorVersion() const { return minorVersion; }
  void setMinorVersion(uint32_t minor) { minorVersion = minor; }

  /// Functions to query current entry point ShaderModelKind.
  bool isPS() const { return curShaderModelKind == ShaderModelKind::Pixel; }
  bool isVS() const { return curShaderModelKind == ShaderModelKind::Vertex; }
  bool isGS() const { return curShaderModelKind == ShaderModelKind::Geometry; }
  bool isHS() const { return curShaderModelKind == ShaderModelKind::Hull; }
  bool isDS() const { return curShaderModelKind == ShaderModelKind::Domain; }
  bool isCS() const { return curShaderModelKind == ShaderModelKind::Compute; }
  bool isLib() const { return curShaderModelKind == ShaderModelKind::Library; }
  bool isRay() const {
    return curShaderModelKind >= ShaderModelKind::RayGeneration &&
           curShaderModelKind <= ShaderModelKind::Callable;
  }
  bool isMS() const { return curShaderModelKind == ShaderModelKind::Mesh; }
  bool isAS() const {
    return curShaderModelKind == ShaderModelKind::Amplification;
  }

  /// Function to get all RichDebugInfo (i.e., the current status of
  /// compilation units).
  llvm::MapVector<llvm::StringRef, RichDebugInfo> &getDebugInfo() {
    return debugInfo;
  }

  /// Function to let the lexical scope stack grow when it enters a
  /// new lexical scope.
  void pushDebugLexicalScope(RichDebugInfo *info, SpirvDebugInstruction *scope);

  /// Function to pop the last element from the lexical scope stack.
  void popDebugLexicalScope(RichDebugInfo *info) {
    info->scopeStack.pop_back();
    currentLexicalScope = info->scopeStack.back();
  }

  /// Function to get the last lexical scope that the SpirvEmitter
  /// class instance entered.
  SpirvDebugInstruction *getCurrentLexicalScope() {
    return currentLexicalScope;
  }

private:
  /// \brief The allocator used to create SPIR-V entity objects.
  ///
  /// SPIR-V entity objects are never destructed; rather, all memory associated
  /// with the SPIR-V entity objects will be released when the SpirvContext
  /// itself is destroyed.
  ///
  /// This field must appear the first since it will be used to allocate object
  /// for the other fields.
  mutable llvm::BumpPtrAllocator allocator;

  // Unique types

  const VoidType *voidType;
  const BoolType *boolType;

  // The type at index i is for bitwidth 2^i. So max bitwidth supported
  // is 2^6 = 64. Index 0/1/2/3 is not used right now.
  std::array<const IntegerType *, 7> sintTypes;
  std::array<const IntegerType *, 7> uintTypes;
  std::array<const FloatType *, 7> floatTypes;

  using VectorTypeArray = std::array<const VectorType *, 5>;
  using MatrixTypeVector = std::vector<const MatrixType *>;
  using SCToPtrTyMap =
      llvm::DenseMap<spv::StorageClass, const SpirvPointerType *,
                     StorageClassDenseMapInfo>;

  // Vector/matrix types for each possible element count.
  // Type at index is for vector of index components. Index 0/1 is unused.

  llvm::DenseMap<const ScalarType *, VectorTypeArray> vecTypes;
  llvm::DenseMap<const VectorType *, MatrixTypeVector> matTypes;
  llvm::DenseSet<const ImageType *, ImageTypeMapInfo> imageTypes;
  const SamplerType *samplerType;
  llvm::DenseMap<const ImageType *, const SampledImageType *> sampledImageTypes;
  llvm::DenseSet<const ArrayType *, ArrayTypeMapInfo> arrayTypes;
  llvm::DenseSet<const RuntimeArrayType *, RuntimeArrayTypeMapInfo>
      runtimeArrayTypes;
  llvm::SmallVector<const StructType *, 8> structTypes;
  llvm::DenseMap<const SpirvType *, SCToPtrTyMap> pointerTypes;
  llvm::DenseSet<FunctionType *, FunctionTypeMapInfo> functionTypes;
  const AccelerationStructureTypeNV *accelerationStructureTypeNV;

  // Current ShaderModelKind for entry point.
  ShaderModelKind curShaderModelKind;
  // Major/Minor hlsl profile version.
  uint32_t majorVersion;
  uint32_t minorVersion;

  /// File name to rich debug info map. When the main source file
  /// includes header files, we create an element of debugInfo for
  /// each file. RichDebugInfo includes DebugSource,
  /// DebugCompilationUnit and scopeStack which keeps lexical scopes
  /// recursively.
  llvm::MapVector<llvm::StringRef, RichDebugInfo> debugInfo;
  SpirvDebugInstruction *currentLexicalScope;

  // Mapping from SPIR-V type to debug type instruction.
  // The purpose is not to generate several DebugType* instructions for the same
  // type if the type is used for several variables.
  llvm::MapVector<const SpirvType *, SpirvDebugType *> debugTypes;

  // Keep DebugTypeMember, DebugTypeInheritance, DebugTypeTemplate,
  // and DebugTypeTemplateParameter.
  // Since they do not have corresponding SpirvType, we cannot keep them
  // in debugTypes. No component references them other than themselves,
  // there by being able to safely emit them at the end of other debug
  // extension instructions.
  //
  // TODO: remove tailDebugTypes. Instead, we can keep
  //       - DebugTypeMember and DebugTypeInheritance in DebugTypeComposite.
  //       - keep DebugTypeTemplate in DebugTypeComposite and DebugFunction.
  //       - keep DebugTypeTemplateParameter in DebugTypeTemplate.
  llvm::SmallVector<SpirvDebugInstruction *, 16> tailDebugTypes;

  // Mapping from CXXMethodDecl (member method of struct or class) to its
  // function info.
  llvm::DenseMap<const CXXMethodDecl *, SpirvDebugFunction *>
      methodDeclToDebugFunction;
};

} // end namespace spirv
} // end namespace clang

// operator new and delete aren't allowed inside namespaces.

/// Placement new for using the SpirvContext's allocator.
inline void *operator new(size_t bytes, const clang::spirv::SpirvContext &c,
                          size_t align = 8) {
  return c.allocate(bytes, align);
}

inline void *operator new(size_t bytes, const clang::spirv::SpirvContext *c,
                          size_t align = 8) {
  return c->allocate(bytes, align);
}

/// Placement delete companion to the new above.
inline void operator delete(void *ptr, const clang::spirv::SpirvContext &c,
                            size_t) {
  c.deallocate(ptr);
}

inline void operator delete(void *ptr, const clang::spirv::SpirvContext *c,
                            size_t) {
  c->deallocate(ptr);
}

#endif
