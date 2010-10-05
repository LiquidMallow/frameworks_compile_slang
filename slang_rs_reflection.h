#ifndef _SLANG_COMPILER_RS_REFLECTION_H
#define _SLANG_COMPILER_RS_REFLECTION_H

#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <iostream>

#include "llvm/ADT/StringExtras.h"

#include "slang_rs_export_type.h"

namespace slang {

  class RSContext;
  class RSExportVar;
  class RSExportFunc;

class RSReflection {
 private:
  const RSContext *mRSContext;

  std::string mLastError;
  inline void setError(const std::string &Error) { mLastError = Error; }

  class Context {
   private:
    static const char *const ApacheLicenseNote;

    static const char *const Import[];

    bool mVerbose;

    std::string mInputRSFile;

    std::string mPackageName;
    std::string mResourceId;

    std::string mClassName;

    std::string mLicenseNote;

    std::string mIndent;

    int mPaddingFieldIndex;

    int mNextExportVarSlot;
    int mNextExportFuncSlot;

    // A mapping from a field in a record type to its index in the rsType
    // instance. Only used when generates TypeClass (ScriptField_*).
    typedef std::map<const RSExportRecordType::Field*, unsigned> FieldIndexMapTy;
    FieldIndexMapTy mFieldIndexMap;
    // Field index of current processing TypeClass.
    unsigned mFieldIndex;

    inline void clear() {
      mClassName = "";
      mIndent = "";
      mPaddingFieldIndex = 1;
      mNextExportVarSlot = 0;
      mNextExportFuncSlot = 0;
      return;
    }

   public:
    typedef enum {
      AM_Public,
      AM_Protected,
      AM_Private
    } AccessModifier;

    bool mUseStdout;
    mutable std::ofstream mOF;

    static const char *AccessModifierStr(AccessModifier AM);

    Context(const std::string &InputRSFile,
            const std::string &PackageName,
            const std::string &ResourceId,
            bool UseStdout)
        : mVerbose(true),
          mInputRSFile(InputRSFile),
          mPackageName(PackageName),
          mResourceId(ResourceId),
          mLicenseNote(ApacheLicenseNote),
          mUseStdout(UseStdout) {
      clear();
      resetFieldIndex();
      clearFieldIndexMap();
      return;
    }

    inline std::ostream &out() const {
      return ((mUseStdout) ? std::cout : mOF);
    }
    inline std::ostream &indent() const {
      out() << mIndent;
      return out();
    }

    inline void incIndentLevel() {
      mIndent.append(4, ' ');
      return;
    }

    inline void decIndentLevel() {
      assert(getIndentLevel() > 0 && "No indent");
      mIndent.erase(0, 4);
      return;
    }

    inline int getIndentLevel() { return (mIndent.length() >> 2); }

    inline int getNextExportVarSlot() { return mNextExportVarSlot++; }

    inline int getNextExportFuncSlot() { return mNextExportFuncSlot++; }

    // Will remove later due to field name information is not necessary for
    // C-reflect-to-Java
    inline std::string createPaddingField() {
      return "#padding_" + llvm::itostr(mPaddingFieldIndex++);
    }

    inline void setLicenseNote(const std::string &LicenseNote) {
      mLicenseNote = LicenseNote;
    }

    bool startClass(AccessModifier AM,
                    bool IsStatic,
                    const std::string &ClassName,
                    const char *SuperClassName,
                    std::string &ErrorMsg);
    void endClass();

    void startFunction(AccessModifier AM,
                       bool IsStatic,
                       const char *ReturnType,
                       const std::string &FunctionName,
                       int Argc, ...);

    typedef std::vector<std::pair<std::string, std::string> > ArgTy;
    void startFunction(AccessModifier AM,
                       bool IsStatic,
                       const char *ReturnType,
                       const std::string &FunctionName,
                       const ArgTy &Args);
    void endFunction();

    void startBlock(bool ShouldIndent = false);
    void endBlock();

    inline const std::string &getPackageName() const { return mPackageName; }
    inline const std::string &getClassName() const { return mClassName; }
    inline const std::string &getResourceId() const { return mResourceId; }

    void startTypeClass(const std::string &ClassName);
    void endTypeClass();

    inline void incFieldIndex() { mFieldIndex++; }

    inline void resetFieldIndex() { mFieldIndex = 0; }

    inline void addFieldIndexMapping(const RSExportRecordType::Field *F) {
      assert((mFieldIndexMap.find(F) == mFieldIndexMap.end()) &&
             "Nested structure never occurs in C language.");
      mFieldIndexMap.insert(std::make_pair(F, mFieldIndex));
    }

    inline unsigned getFieldIndex(const RSExportRecordType::Field *F) const {
      FieldIndexMapTy::const_iterator I = mFieldIndexMap.find(F);
      assert((I != mFieldIndexMap.end()) &&
             "Requesting field is out of scope.");
      return I->second;
    }

    inline void clearFieldIndexMap() { mFieldIndexMap.clear(); }
  };

  bool openScriptFile(Context &C,
                      const std::string &ClassName,
                      std::string &ErrorMsg);
  bool genScriptClass(Context &C,
                      const std::string &ClassName,
                      std::string &ErrorMsg);
  void genScriptClassConstructor(Context &C);

  void genInitBoolExportVariable(Context &C,
                                 const std::string &VarName,
                                 const clang::APValue &Val);
  void genInitPrimitiveExportVariable(Context &C,
                                      const std::string &VarName,
                                      const clang::APValue &Val);
  void genInitExportVariable(Context &C,
                             const RSExportType *ET,
                             const std::string &VarName,
                             const clang::APValue &Val);
  void genExportVariable(Context &C, const RSExportVar *EV);
  void genPrimitiveTypeExportVariable(Context &C, const RSExportVar *EV);
  void genPointerTypeExportVariable(Context &C, const RSExportVar *EV);
  void genVectorTypeExportVariable(Context &C, const RSExportVar *EV);
  void genMatrixTypeExportVariable(Context &C, const RSExportVar *EV);
  void genRecordTypeExportVariable(Context &C, const RSExportVar *EV);
  void genGetExportVariable(Context &C,
                            const std::string &TypeName,
                            const std::string &VarName);

  void genExportFunction(Context &C,
                         const RSExportFunc *EF);

  bool genTypeClass(Context &C,
                    const RSExportRecordType *ERT,
                    std::string &ErrorMsg);
  bool genTypeItemClass(Context &C,
                        const RSExportRecordType *ERT,
                        std::string &ErrorMsg);
  void genTypeClassConstructor(Context &C, const RSExportRecordType *ERT);
  void genTypeClassCopyToArray(Context &C, const RSExportRecordType *ERT);
  void genTypeClassItemSetter(Context &C, const RSExportRecordType *ERT);
  void genTypeClassItemGetter(Context &C, const RSExportRecordType *ERT);
  void genTypeClassComponentSetter(Context &C, const RSExportRecordType *ERT);
  void genTypeClassComponentGetter(Context &C, const RSExportRecordType *ERT);
  void genTypeClassCopyAll(Context &C, const RSExportRecordType *ERT);

  void genBuildElement(Context &C,
                       const RSExportRecordType *ERT,
                       const char *RenderScriptVar);
  void genAddElementToElementBuilder(Context &C,
                                     const RSExportType *ERT,
                                     const std::string &VarName,
                                     const char *ElementBuilderName,
                                     const char *RenderScriptVar);
  void genAddPaddingToElementBuiler(Context &C,
                                    int PaddingSize,
                                    const char *ElementBuilderName,
                                    const char *RenderScriptVar);

  bool genCreateFieldPacker(Context &C,
                            const RSExportType *T,
                            const char *FieldPackerName);
  void genPackVarOfType(Context &C,
                        const RSExportType *T,
                        const char *VarName,
                        const char *FieldPackerName);
  void genNewItemBufferIfNull(Context &C, const char *Index);
  void genNewItemBufferPackerIfNull(Context &C);

 public:
  explicit RSReflection(const RSContext *Context)
      : mRSContext(Context),
        mLastError("") {
    return;
  }

  bool reflect(const char *OutputPackageName,
               const std::string &InputFileName,
               const std::string &OutputBCFileName);

  inline const char *getLastError() const {
    if (mLastError.empty())
      return NULL;
    else
      return mLastError.c_str();
  }
};  // class RSReflection

}   // namespace slang

#endif  // _SLANG_COMPILER_RS_REFLECTION_H