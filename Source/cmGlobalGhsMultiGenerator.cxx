/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2015 Geoffrey Viola <geoffrey.viola@asirobots.com>

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "cmGlobalGhsMultiGenerator.h"
#include "cmLocalGhsMultiGenerator.h"
#include "cmMakefile.h"
#include "cmVersion.h"
#include "cmGeneratedFileStream.h"
#include "cmGhsMultiTargetGenerator.h"
#include <cmsys/SystemTools.hxx>
#include <cmAlgorithms.h>

const char *cmGlobalGhsMultiGenerator::FILE_EXTENSION = ".gpj";

cmGlobalGhsMultiGenerator::cmGlobalGhsMultiGenerator() : OSDirRelative(false) {
  this->GhsBuildCommandInitialized = false;
}

cmGlobalGhsMultiGenerator::~cmGlobalGhsMultiGenerator() {
  cmDeleteAll(TargetFolderBuildStreams);
}

cmLocalGenerator *cmGlobalGhsMultiGenerator::CreateLocalGenerator() {
  cmLocalGenerator *lg = new cmLocalGhsMultiGenerator;
  lg->SetGlobalGenerator(this);
  this->SetCurrentLocalGenerator(lg);
  return lg;
}

void cmGlobalGhsMultiGenerator::GetDocumentation(cmDocumentationEntry &entry) {
  entry.Name = GetActualName();
  entry.Brief = "Generates Green Hills MULTI files (experimental).";
}

void cmGlobalGhsMultiGenerator::EnableLanguage(
    std::vector<std::string> const &l, cmMakefile *mf, bool optional) {
  mf->AddDefinition("CMAKE_SYSTEM_NAME", "GHS-MULTI");
  mf->AddDefinition("CMAKE_SYSTEM_PROCESSOR", "ARM");

  const std::string ghsCompRoot(GetCompRoot());
  mf->AddDefinition("GHS_COMP_ROOT", ghsCompRoot.c_str());
  std::string ghsCompRootStart =
      0 == ghsCompRootStart.size() ? "" : ghsCompRoot + "/";
  mf->AddDefinition("CMAKE_C_COMPILER",
                    std::string(ghsCompRootStart + "ccarm.exe").c_str());
  mf->AddDefinition("CMAKE_C_COMPILER_ID_RUN", "TRUE");
  mf->AddDefinition("CMAKE_C_COMPILER_ID", "GHSC");
  mf->AddDefinition("CMAKE_C_COMPILER_FORCED", "TRUE");

  mf->AddDefinition("CMAKE_CXX_COMPILER",
                    std::string(ghsCompRootStart + "cxarm.exe").c_str());
  mf->AddDefinition("CMAKE_CXX_COMPILER_ID_RUN", "TRUE");
  mf->AddDefinition("CMAKE_CXX_COMPILER_ID", "GHSCXX");
  mf->AddDefinition("CMAKE_CXX_COMPILER_FORCED", "TRUE");

  if (ghsCompRoot.length() > 0) {
    static const char *compPreFix = "comp_";
    std::string compFilename =
        cmsys::SystemTools::FindLastString(ghsCompRoot.c_str(), compPreFix);
    cmsys::SystemTools::ReplaceString(compFilename, compPreFix, "");
    mf->AddDefinition("CMAKE_SYSTEM_VERSION", compFilename.c_str());
  }

  mf->AddDefinition("GHSMULTI", "1"); // identifier for user CMake files
  this->cmGlobalGenerator::EnableLanguage(l, mf, optional);
}

void cmGlobalGhsMultiGenerator::FindMakeProgram(cmMakefile *mf) {
  // The GHS generator knows how to lookup its build tool
  // directly instead of needing a helper module to do it, so we
  // do not actually need to put CMAKE_MAKE_PROGRAM into the cache.
  if (cmSystemTools::IsOff(mf->GetDefinition("CMAKE_MAKE_PROGRAM"))) {
    mf->AddDefinition("CMAKE_MAKE_PROGRAM",
                      this->GetGhsBuildCommand().c_str());
  }
}

std::string const &cmGlobalGhsMultiGenerator::GetGhsBuildCommand() {
  if (!this->GhsBuildCommandInitialized) {
    this->GhsBuildCommandInitialized = true;
    this->GhsBuildCommand = this->FindGhsBuildCommand();
  }
  return this->GhsBuildCommand;
}

std::string cmGlobalGhsMultiGenerator::FindGhsBuildCommand() {
  std::string makeProgram = cmSystemTools::FindProgram("gbuild");
  if (makeProgram.empty()) {
    makeProgram = "gbuild";
  }
  return makeProgram;
}

std::string cmGlobalGhsMultiGenerator::GetCompRoot() {
  std::string output;

  const std::vector<std::string>
    potentialDirsHardPaths(GetCompRootHardPaths());
  const std::vector<std::string> potentialDirsRegistry(GetCompRootRegistry());

  std::vector<std::string> potentialDirsComplete;
  potentialDirsComplete.insert(potentialDirsComplete.end(),
                               potentialDirsHardPaths.begin(),
                               potentialDirsHardPaths.end());
  potentialDirsComplete.insert(potentialDirsComplete.end(),
                               potentialDirsRegistry.begin(),
                               potentialDirsRegistry.end());

  // Use latest version
  std::string outputDirName;
  for (std::vector<std::string>::const_iterator potentialDirsCompleteIt =
           potentialDirsComplete.begin();
       potentialDirsCompleteIt != potentialDirsComplete.end();
       ++potentialDirsCompleteIt) {
    const std::string dirName(
        cmsys::SystemTools::GetFilenameName(*potentialDirsCompleteIt));
    if (dirName.compare(outputDirName) > 0) {
      output = *potentialDirsCompleteIt;
      outputDirName = dirName;
    }
  }

  return output;
}

std::vector<std::string> cmGlobalGhsMultiGenerator::GetCompRootHardPaths() {
  std::vector<std::string> output;
  cmSystemTools::Glob("C:/ghs", "comp_[^;]+", output);
  for (std::vector<std::string>::iterator outputIt = output.begin();
       outputIt != output.end(); ++outputIt) {
    *outputIt = "C:/ghs/" + *outputIt;
  }
  return output;
}

std::vector<std::string> cmGlobalGhsMultiGenerator::GetCompRootRegistry() {
  std::vector<std::string> output(2);
  cmsys::SystemTools::ReadRegistryValue(
      "HKEY_LOCAL_"
      "MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\"
      "Windows\\CurrentVersion\\Uninstall\\"
      "GreenHillsSoftwared771f1b4;InstallLocation",
      output[0]);
  cmsys::SystemTools::ReadRegistryValue(
      "HKEY_LOCAL_"
      "MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\"
      "Windows\\CurrentVersion\\Uninstall\\"
      "GreenHillsSoftware9881cef6;InstallLocation",
      output[1]);
  return output;
}

void cmGlobalGhsMultiGenerator::OpenBuildFileStream(
    std::string const &filepath, cmGeneratedFileStream **filestream) {
  // Get a stream where to generate things.
  if (NULL == *filestream) {
    *filestream = new cmGeneratedFileStream(filepath.c_str());
    if (NULL != *filestream) {
      OpenBuildFileStream(*filestream);
    }
  }
}

void cmGlobalGhsMultiGenerator::OpenBuildFileStream(
    cmGeneratedFileStream *filestream) {
  *filestream << "#!gbuild" << std::endl;
}

void cmGlobalGhsMultiGenerator::OpenBuildFileStream() {
  // Compute GHS MULTI's build file path.
  std::string buildFilePath =
      this->GetCMakeInstance()->GetHomeOutputDirectory();
  buildFilePath += "/";
  buildFilePath += "default";
  buildFilePath += FILE_EXTENSION;

  Open(std::string(""), buildFilePath, &TargetFolderBuildStreams);
  OpenBuildFileStream(GetBuildFileStream());

  char const *osDir =
      this->GetCMakeInstance()->GetCacheDefinition("GHS_OS_DIR");
  if (NULL == osDir) {
    osDir = "";
    cmSystemTools::Error("GHS_OS_DIR cache variable must be set");
  } else {
    this->GetCMakeInstance()->MarkCliAsUsed("GHS_OS_DIR");
  }
  std::string fOSDir(trimQuotes(osDir));
  cmSystemTools::ReplaceString(fOSDir, "\\", "/");
  if (fOSDir.length() > 0 && ('c' == fOSDir[0] || 'C' == fOSDir[0])) {
    OSDirRelative = false;
  } else {
    OSDirRelative = true;
  }

  char const *bspName =
      this->GetCMakeInstance()->GetCacheDefinition("GHS_BSP_NAME");
  if (NULL == bspName) {
    bspName = "";
    cmSystemTools::Error("GHS_BSP_NAME cache variable must be set");
  } else {
    this->GetCMakeInstance()->MarkCliAsUsed("GHS_BSP_NAME");
  }
  std::string fBspName(trimQuotes(bspName));
  cmSystemTools::ReplaceString(fBspName, "\\", "/");
  WriteMacros();
  WriteHighLevelDirectives();

  GhsMultiGpj::WriteGpjTag(GhsMultiGpj::PROJECT, GetBuildFileStream());
  WriteDisclaimer(GetBuildFileStream());
  *GetBuildFileStream() << "# Top Level Project File" << std::endl;
  if (fBspName.length() > 0) {
    *GetBuildFileStream() << "    -bsp " << fBspName << std::endl;
  }
  WriteCompilerOptions(fOSDir);
}

void cmGlobalGhsMultiGenerator::CloseBuildFileStream(
    cmGeneratedFileStream **filestream) {
  if (filestream) {
    delete *filestream;
    *filestream = NULL;
  } else {
    cmSystemTools::Error("Build file stream was not open.");
  }
}

void cmGlobalGhsMultiGenerator::Generate() {
  this->cmGlobalGenerator::Generate();

  if (this->LocalGenerators.size() > 0) {
    this->OpenBuildFileStream();

    // Build all the folder build files
    for (unsigned int i = 0; i < this->LocalGenerators.size(); ++i) {
      cmLocalGhsMultiGenerator *lg =
          static_cast<cmLocalGhsMultiGenerator *>(this->LocalGenerators[i]);
      cmGeneratorTargetsType tgts = lg->GetMakefile()->GetGeneratorTargets();
      UpdateBuildFiles(tgts);
    }
  }

  cmDeleteAll(TargetFolderBuildStreams);
  this->TargetFolderBuildStreams.clear();
}

void cmGlobalGhsMultiGenerator::GenerateBuildCommand(
    std::vector<std::string> &makeCommand, const std::string &makeProgram,
    const std::string & /*projectName*/, const std::string & /*projectDir*/,
    const std::string &targetName, const std::string & /*config*/,
    bool /*fast*/, bool /*verbose*/,
    std::vector<std::string> const &makeOptions) {
  makeCommand.push_back(this->SelectMakeProgram(makeProgram));

  makeCommand.insert(makeCommand.end(),
                     makeOptions.begin(), makeOptions.end());
  if (!targetName.empty()) {
    if (targetName == "clean") {
      makeCommand.push_back("-clean");
    } else {
      makeCommand.push_back(targetName);
    }
  }
}

void cmGlobalGhsMultiGenerator::WriteMacros() {
  char const *ghsGpjMacros =
      this->GetCMakeInstance()->GetCacheDefinition("GHS_GPJ_MACROS");
  if (NULL != ghsGpjMacros) {
    std::vector<std::string> expandedList;
    cmSystemTools::ExpandListArgument(std::string(ghsGpjMacros), expandedList);
    for (std::vector<std::string>::const_iterator expandedListI =
             expandedList.begin();
         expandedListI != expandedList.end(); ++expandedListI) {
      *GetBuildFileStream() << "macro " << *expandedListI << std::endl;
    }
  }
}

void cmGlobalGhsMultiGenerator::WriteHighLevelDirectives() {
  *GetBuildFileStream() << "primaryTarget=arm_integrity.tgt" << std::endl;
  char const *const customization =
      this->GetCMakeInstance()->GetCacheDefinition("GHS_CUSTOMIZATION");
  if (NULL != customization && strlen(customization) > 0) {
    *GetBuildFileStream() << "customization=" << trimQuotes(customization)
                          << std::endl;
    this->GetCMakeInstance()->MarkCliAsUsed("GHS_CUSTOMIZATION");
  }
}

void cmGlobalGhsMultiGenerator::WriteCompilerOptions(
    std::string const &fOSDir) {
  *GetBuildFileStream() << "    -os_dir=\"" << fOSDir << "\"" << std::endl;
  *GetBuildFileStream() << "    --link_once_templates" << std::endl;
}

void cmGlobalGhsMultiGenerator::WriteDisclaimer(std::ostream *os) {
  (*os) << "#" << std::endl
        << "# CMAKE generated file: DO NOT EDIT!" << std::endl
        << "# Generated by \"" << GetActualName() << "\""
        << " Generator, CMake Version " << cmVersion::GetMajorVersion() << "."
        << cmVersion::GetMinorVersion() << std::endl
        << "#" << std::endl;
}

void cmGlobalGhsMultiGenerator::AddFilesUpToPath(
    cmGeneratedFileStream *mainBuildFile,
    std::map<std::string, cmGeneratedFileStream*> *
        targetFolderBuildStreams,
    char const *homeOutputDirectory, std::string const &path,
    GhsMultiGpj::Types projType, std::string const &relPath) {
  std::string workingPath(path);
  cmSystemTools::ConvertToUnixSlashes(workingPath);
  std::vector<cmsys::String> splitPath =
      cmSystemTools::SplitString(workingPath);
  std::string workingRelPath(relPath);
  if (relPath.size() > 0 && '/' != relPath.back()) {
    workingRelPath += "/";
  }
  std::string pathUpTo;
  for (std::vector<cmsys::String>::const_iterator splitPathI =
           splitPath.begin();
       splitPath.end() != splitPathI; ++splitPathI) {
    pathUpTo += *splitPathI;
    if (targetFolderBuildStreams->end() ==
        targetFolderBuildStreams->find(pathUpTo)) {
      AddFilesUpToPathNewBuildFile(
          mainBuildFile, targetFolderBuildStreams, homeOutputDirectory,
          pathUpTo, splitPath.begin() == splitPathI, workingRelPath, projType);
    }
    AddFilesUpToPathAppendNextFile(targetFolderBuildStreams, pathUpTo,
                                   splitPathI, splitPath.end(), projType);
    pathUpTo += "/";
  }
}

void cmGlobalGhsMultiGenerator::Open(
    std::string const &mapKeyName, std::string const &fileName,
    std::map<std::string, cmGeneratedFileStream*> *fileMap) {
  if (fileMap->end() == fileMap->find(fileName)) {
    cmGeneratedFileStream* temp(new cmGeneratedFileStream);
    temp->open(fileName.c_str());
    (*fileMap)[mapKeyName] = temp;
  }
}

void cmGlobalGhsMultiGenerator::AddFilesUpToPathNewBuildFile(
    cmGeneratedFileStream *mainBuildFile,
    std::map<std::string, cmGeneratedFileStream*> *
        targetFolderBuildStreams,
    char const *homeOutputDirectory, std::string const &pathUpTo,
    bool const isFirst, std::string const &relPath,
    GhsMultiGpj::Types const projType) {
  // create folders up to file path
  std::string absPath = std::string(homeOutputDirectory) + "/" + relPath;
  std::string newPath = absPath + pathUpTo;
  if (!cmSystemTools::FileExists(newPath.c_str())) {
    cmSystemTools::MakeDirectory(newPath.c_str());
  }

  // Write out to filename for first time
  std::string relFilename(GetFileNameFromPath(pathUpTo));
  std::string absFilename = absPath + relFilename;
  Open(pathUpTo, absFilename, targetFolderBuildStreams);
  OpenBuildFileStream((*targetFolderBuildStreams)[pathUpTo]);
  GhsMultiGpj::WriteGpjTag(projType,
                           (*targetFolderBuildStreams)[pathUpTo]);
  WriteDisclaimer((*targetFolderBuildStreams)[pathUpTo]);

  // Add to main build file
  if (isFirst) {
    *mainBuildFile << relFilename << " ";
    GhsMultiGpj::WriteGpjTag(projType, mainBuildFile);
  }
}

void cmGlobalGhsMultiGenerator::AddFilesUpToPathAppendNextFile(
    std::map<std::string, cmGeneratedFileStream*> *
        targetFolderBuildStreams,
    std::string const &pathUpTo,
    std::vector<cmsys::String>::const_iterator splitPathI,
    std::vector<cmsys::String>::const_iterator end,
    GhsMultiGpj::Types const projType) {
  std::vector<cmsys::String>::const_iterator splitPathNextI = splitPathI + 1;
  if (end != splitPathNextI &&
      targetFolderBuildStreams->end() ==
          targetFolderBuildStreams->find(pathUpTo + "/" + *splitPathNextI)) {
    std::string nextFilename(*splitPathNextI);
    nextFilename = GetFileNameFromPath(nextFilename);
    *(*targetFolderBuildStreams)[pathUpTo] << nextFilename << " ";
    GhsMultiGpj::WriteGpjTag(projType,
                             (*targetFolderBuildStreams)[pathUpTo]);
  }
}

std::string
cmGlobalGhsMultiGenerator::GetFileNameFromPath(std::string const &path) {
  std::string output(path);
  if (path.length() > 0) {
    cmSystemTools::ConvertToUnixSlashes(output);
    std::vector<cmsys::String> splitPath = cmSystemTools::SplitString(output);
    output += "/" + splitPath.back() + FILE_EXTENSION;
  }
  return output;
}

void cmGlobalGhsMultiGenerator::UpdateBuildFiles(
    cmGeneratorTargetsType const &tgts) {
  for (cmGeneratorTargetsType::const_iterator tgtsI = tgts.begin();
       tgtsI != tgts.end(); ++tgtsI) {
    cmGhsMultiTargetGenerator gmtg(tgtsI->first);
    if (gmtg.GetSources().size() > 0 && gmtg.IncludeThisTarget()) {
      char const *rawFolderName = tgtsI->first->GetProperty("FOLDER");
      if (NULL == rawFolderName) {
        rawFolderName = "";
      }
      std::string folderName(rawFolderName);
      if (TargetFolderBuildStreams.end() ==
          TargetFolderBuildStreams.find(folderName)) {
        AddFilesUpToPath(GetBuildFileStream(), &TargetFolderBuildStreams,
                         this->GetCMakeInstance()->GetHomeOutputDirectory(),
                         folderName, GhsMultiGpj::PROJECT);
      }
      std::vector<cmsys::String> splitPath =
          cmSystemTools::SplitString(gmtg.GetRelBuildFileName());
      std::string foldNameRelBuildFile(*(splitPath.end() - 2) + "/" +
                                       splitPath.back());
      *TargetFolderBuildStreams[folderName] << foldNameRelBuildFile << " ";
      GhsMultiGpj::WriteGpjTag(gmtg.GetGpjTag(),
                               TargetFolderBuildStreams[folderName]);
    }
  }
}

std::string cmGlobalGhsMultiGenerator::trimQuotes(std::string const &str) {
  std::string result;
  result.reserve(str.size());
  for (const char *ch = str.c_str(); *ch != '\0'; ++ch) {
    if (*ch != '"') {
      result += *ch;
    }
  }
  return result;
}
