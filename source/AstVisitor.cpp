#include <iomanip>

#include "AstVisitor.h"
#include "Common.h"

using namespace namelint;

LOG_DECISION_DEFAULT(false);

MyASTVisitor::MyASTVisitor(const SourceManager *pSM, const ASTContext *pAstCxt,
                           const Config *pConfig) {
  this->m_DumpDecl.SetSourceManager(pSM);
  this->m_pSrcMgr = pSM;
  this->m_pAstCxt = (ASTContext *)pAstCxt;
  this->m_pConfig = pConfig->GetData();
  DcLib::Log::Out(INFO_ALL, "%s", __func__);
}

bool MyASTVisitor::VisitCXXRecordDecl(CXXRecordDecl *D) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);
  return true;
}

bool MyASTVisitor::VisitFunctionDecl(clang::FunctionDecl *pDecl) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);

  if (!this->m_pConfig->General.Options.bCheckFunctionName) {
    DcLib::Log::Out(INFO_ALL, "Skipped, becuase config file is disable. (bCheckFunctionName)");
    return true;
  }

  string FuncName;
  bool bResult = this->getFunctionInfo(pDecl, FuncName);
  if (!bResult) {
    return true;
  }

  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());
  pAppCxt->MemoBoard.Checked.nFunction++;

  // This FunctionDecl may just an external function.
  if (pDecl->isInvalidDecl()) {
    DcLib::Log::Out(INFO_ALL, "Found an invalid FunctionDecl. (%s)", FuncName.c_str());

    pAppCxt->MemoBoard.Assert.nInvalidDecl++;

    if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
      return true;
    }
  }

  if (bResult) {
    bool bIsPtr   = false;
    bool bIsArray = false;
    bResult = this->m_Detect.CheckFunction(this->m_pConfig->General.Rules.FunctionName, FuncName);

    if (!bResult) {
      pAppCxt->MemoBoard.Error.nFunction++;

      pAppCxt->MemoBoard.ErrorDetailList.push_back(
          this->createErrorDetail(pDecl, CheckType::CT_Function, bIsPtr, bIsArray, FuncName, ""));
    }
  }

  return true;
}

bool MyASTVisitor::VisitCXXMethodDecl(CXXMethodDecl *pDecl) {
  //
  return true;
}

bool MyASTVisitor::VisitRecordDecl(RecordDecl *pDecl) {

  if (!this->isMainFile(pDecl)) {
    return true;
  }

  DcLib::Log::Out(INFO_ALL, "%s", __func__);

  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());
  this->m_DumpDecl.PrintDecl(pDecl);

  bool bStatus   = false;
  string VarName = pDecl->getName();

  if (pDecl->isInvalidDecl()) {
    DcLib::Log::Out(INFO_ALL, "Found an invalid RecordDecl. (%s)", VarName.c_str());

    pAppCxt->MemoBoard.Assert.nInvalidDecl++;

    if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
      return true;
    }
  }

  switch (pDecl->getTagKind()) {
  case TTK_Struct: {
    if (VarName.empty()) {
      DcLib::Log::Out(INFO_ALL, "It is an anonymous structure.");
      bStatus = true;
    } else {
      pAppCxt->MemoBoard.Checked.nStruct++;

      RULETYPE RuleType = this->m_pConfig->General.Rules.StructTagName;

      bStatus = this->m_Detect.CheckStructVal(RuleType, "" /*no type*/, VarName, NOT_PTR);
      if (!bStatus) {
        pAppCxt->MemoBoard.Error.nStruct++;
        pAppCxt->MemoBoard.ErrorDetailList.push_back(this->createErrorDetail(
            pDecl, CheckType::CT_StructTag, NOT_PTR, NOT_ARRAY, "", VarName, ""));
      }
    }
    break;
  }
  case TTK_Union: {
    if (VarName.empty()) {
      DcLib::Log::Out(INFO_ALL, "It is an anonymous union.");
      bStatus = true;
    } else {
      pAppCxt->MemoBoard.Checked.nUnion++;

      RULETYPE RuleType = this->m_pConfig->General.Rules.StructTagName;

      bStatus = this->m_Detect.CheckStructVal(RuleType, "" /*no type*/, VarName, NOT_PTR);
      if (!bStatus) {
        pAppCxt->MemoBoard.Error.nUnion++;
        pAppCxt->MemoBoard.ErrorDetailList.push_back(this->createErrorDetail(
            pDecl, CheckType::CT_UnionTag, NOT_PTR, NOT_ARRAY, "", VarName, ""));
      }
    }
    break;
  }
  case TTK_Class: {

    pAppCxt->MemoBoard.Checked.nClass++;

    bool bStatus = this->m_Detect.CheckEnumVal(this->m_pConfig->General.Rules.ClassName, VarName);
    if (!bStatus) {
      pAppCxt->MemoBoard.Error.nClass++;
      pAppCxt->MemoBoard.ErrorDetailList.push_back(
          this->createErrorDetail(pDecl, CheckType::CT_Class, NOT_PTR, NOT_ARRAY, "", VarName, ""));
    }
    break;
  }
  case TTK_Interface:
    VarName = "TTK_Interface";
    break;
  case TTK_Enum:
    VarName = "TTK_Enum";
    break;
  default:
    VarName = "??";
  }

  return true;
}

bool MyASTVisitor::VisitVarDecl(VarDecl *pDecl) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);

  if (!this->m_pConfig->General.Options.bCheckVariableName) {
    DcLib::Log::Out(INFO_ALL, "Skipped, becuase config file is disable. (bCheckVariableName)");
    return true;
  }

  if (pDecl->isInvalidDecl()) {
    DcLib::Log::Out(INFO_ALL, "Found an invalid VarDecl. (%s)", pDecl->getName());

    APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());
    pAppCxt->MemoBoard.Assert.nInvalidDecl++;

    if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
      return true;
    }
  }

  if (isa<ParmVarDecl>(pDecl)) {
    return true;
  }

  bool bRet = checkRuleForVariable(pDecl);
  return true;
}

bool MyASTVisitor::VisitFieldDecl(FieldDecl *pDecl) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);

  if (!this->m_pConfig->General.Options.bCheckVariableName) {
    DcLib::Log::Out(INFO_ALL, "Skipped, becuase config file is disable. (bCheckVariableName)");
    return true;
  }

  if (pDecl->isInvalidDecl()) {
    DcLib::Log::Out(INFO_ALL, "Found an invalid FieldDecl. (%s)", pDecl->getName());

    APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());
    pAppCxt->MemoBoard.Assert.nInvalidDecl++;

    if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
      return true;
    }
  }

  bool bRet = false;
  auto aaa  = pDecl->getParent()->getTagKind();
  switch (pDecl->getParent()->getTagKind()) {
  case TTK_Struct: {
    bRet = checkRuleForStructValue(pDecl);
    break;
  case TTK_Class:
    bRet = checkRuleForVariable(pDecl);
    break;
  case TTK_Union:
    bRet = checkRuleForUnionValue(pDecl);
    break;
  default:
    bRet = checkRuleForStructValue(pDecl);
  }
  }

  return true;
}

bool MyASTVisitor::VisitReturnStmt(ReturnStmt *pRetStmt) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);

  const Expr *pExpr = pRetStmt->getRetValue();
  if (pExpr) {
    clang::QualType MyQualType = pExpr->getType();
    std::string MyTypeStr      = MyQualType.getAsString();
    return true;
  }
  return false;
}

bool MyASTVisitor::VisitParmVarDecl(ParmVarDecl *pDecl) {
  string VarType;
  string VarName;

  bool bIsPtr     = false;
  bool bIsArray   = false;
  bool bAnonymous = false;
  bool bResult    = false;

  DcLib::Log::Out(INFO_ALL, "%s", __func__);
  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());

  this->m_DumpDecl.PrintDecl(pDecl);

  if (pDecl->isInvalidDecl()) {
    DcLib::Log::Out(INFO_ALL, "Found an invalid ParmVarDecl. (%s)", pDecl->getName());

    pAppCxt->MemoBoard.Assert.nInvalidDecl++;

    if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
      return true;
    }
  }

  bool bStatus = this->getParmsInfo(pDecl, VarType, VarName, bIsPtr, bAnonymous);
  if (!bAnonymous) // if this variable doesn't have name, it doesn't need to be
                   // checked.
  {
    if (bStatus) {
      bResult = this->m_Detect.CheckVariable(
          this->m_pConfig->General.Rules.VariableName, VarType, VarName,
          this->m_pConfig->Hungarian.Options.PreferUpperCamelIfMissed, bIsPtr, bIsArray);

      pAppCxt->MemoBoard.Checked.nParameter++;
      if (!bResult) {
        pAppCxt->MemoBoard.Error.nParameter++;

        pAppCxt->MemoBoard.ErrorDetailList.push_back(this->createErrorDetail(
            pDecl, CheckType::CT_Parameter, bIsPtr, bIsArray, VarType, VarName, ""));
      }
    }
  }

  return true;
}

bool MyASTVisitor::VisitTypedefDecl(TypedefDecl *pDecl) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);
  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());

  this->m_DumpDecl.PrintDecl(pDecl);

  return true;
}

bool MyASTVisitor::VisitEnumConstantDecl(EnumConstantDecl *pDecl) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);
  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());

  this->m_DumpDecl.PrintDecl(pDecl);

  pAppCxt->MemoBoard.Checked.nEnum++;

  string EnumValName = pDecl->getName();

  if (pDecl->isInvalidDecl()) {
    DcLib::Log::Out(INFO_ALL, "Found an invalid EnumConstantDecl. (%s)", EnumValName.c_str());

    pAppCxt->MemoBoard.Assert.nInvalidDecl++;

    if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
      return true;
    }
  }

  bool bStatus = checkRuleForEnumValue(pDecl);
  if (!bStatus) {
    string EnumTagName = "???1";
    if (pAppCxt->MemoBoard.pLastEnumDecl) {
      EnumTagName = pAppCxt->MemoBoard.pLastEnumDecl->getName();
    }
    pAppCxt->MemoBoard.Error.nEnum++;
    pAppCxt->MemoBoard.ErrorDetailList.push_back(this->createErrorDetail(
        pDecl, CheckType::CT_EnumVal, NOT_PTR, NOT_ARRAY, EnumTagName, EnumValName, "???2"));
  }

  return true;
}

bool MyASTVisitor::VisitEnumDecl(EnumDecl *pDecl) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);
  APP_CONTEXT *pAppCxt = ((APP_CONTEXT *)GetAppCxt());

  this->m_DumpDecl.PrintDecl(pDecl);

  pAppCxt->MemoBoard.pLastEnumDecl = pDecl;
  pAppCxt->MemoBoard.Checked.nEnum++;

  string EnumTagName = pDecl->getName();
  if (!EnumTagName.empty()) {

    if (pDecl->isInvalidDecl()) {
      DcLib::Log::Out(INFO_ALL, "Found an invalid EnumDecl. (%s)", EnumTagName.c_str());

      pAppCxt->MemoBoard.Assert.nInvalidDecl++;

      if (true == this->m_pConfig->General.Options.bBypassInvalidDecl) {
        return true;
      }
    }

    bool bStatus =
        this->m_Detect.CheckEnumVal(this->m_pConfig->General.Rules.EnumTagName, EnumTagName);
    if (!bStatus) {
      pAppCxt->MemoBoard.Error.nEnum++;
      pAppCxt->MemoBoard.ErrorDetailList.push_back(this->createErrorDetail(
          pDecl, CheckType::CT_EnumTag, NOT_PTR, NOT_ARRAY, "", EnumTagName, ""));
    }
  }

  return true;
}

bool MyASTVisitor::VisitTagTypeLoc(TagTypeLoc TL) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);

  // TagDecl* pDecl = TL.getDecl();

  // SourceLocation MyBeginLoc = pDecl->getBeginLoc();
  // SourceLocation MyCurrLoc = pDecl->getLocation();
  // SourceLocation MyEndLoc = pDecl->getEndLoc();

  // const char *szBegin = this->m_pSrcMgr->getCharacterData(MyBeginLoc);
  // const char *szCurr = this->m_pSrcMgr->getCharacterData(MyCurrLoc);
  // const char *szEnd = this->m_pSrcMgr->getCharacterData(MyEndLoc);
  return true;
}

bool MyASTVisitor::VisitArrayTypeLoc(ArrayTypeLoc TL) {
  DcLib::Log::Out(INFO_ALL, "%s", __func__);
  // SourceLocation MyBeginLoc = TL.getBeginLoc();
  // SourceLocation MyEndLoc = TL.getEndLoc();
  //
  // const char *szBegin = this->m_pSrcMgr->getCharacterData(MyBeginLoc);
  // const char *szEnd = this->m_pSrcMgr->getCharacterData(MyEndLoc);
  return true;
}

bool MyASTVisitor::VisitFunctionTypeLoc(FunctionTypeLoc TL, bool SkipResultType) {
  SourceLocation MyBeginLoc = TL.getBeginLoc();
  SourceLocation MyEndLoc   = TL.getEndLoc();

  const char *szBegin = this->m_pSrcMgr->getCharacterData(MyBeginLoc);
  const char *szEnd   = this->m_pSrcMgr->getCharacterData(MyEndLoc);
  return true;
}