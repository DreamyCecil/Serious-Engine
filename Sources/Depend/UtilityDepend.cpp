/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "StdH.h"
#include "Dependency.h"

// Depend - extract dependencies and create group file utility

static void PrintUsage(void) {
  printf( 
  "\nUSAGE:\n"
  "Depend d <application dir> <file 1> <file 2> <output file>\n"
  "  make difference beetween two dependency files (result = file 1 - file 2)\n"
  "Depend i <application dir> <lst file> <dep file>\n"
  "  create dependencies for files listed in given list file\n"
  "Depend u <application dir> <in dep file> <out dep file>\n"
  "  remove updated files from dependency file\n"
  "Depend e <application dir> <dep file> <lst file>\n"
  "  export dependency file to ascii list file\n"
  "Depend t <application dir> <lst file> <translation file>\n"
  "  export strings for translation\n"
  "\n");
  exit( EXIT_FAILURE);
};

// [Cecil] Command line arguments
static char _chMode;
static CTString _fnmAppDir;
static CTString _afnmFiles[3];

// [Cecil] Parsed arguments
static INDEX _ctParsedArgs = 0;

// [Cecil] Handle unknown commands
static BOOL HandleUnknownOption(const CTString &strCmd) {
  // Handle arguments one after another
  const INDEX iThisArg = _ctParsedArgs++;

  switch (iThisArg) {
    // Application mode must be a single character
    case 0: {
      if (strCmd.Length() != 1) {
        printf("First argument must be letter representing wanted option.\n\n");
        PrintUsage();
        return FALSE;
      }

      _chMode = (char)tolower(static_cast<UBYTE>(strCmd[0]));
    } break;

    // Set application directory
    case 1: _fnmAppDir = strCmd; break;

    // Set files
    case 2: _afnmFiles[0] = strCmd; break;
    case 3: _afnmFiles[1] = strCmd; break;
    case 4: _afnmFiles[2] = strCmd; break;

    default: return FALSE;
  }

  return TRUE;
};

void SubMain(int argc, char **argv) {
  // [Cecil] Parse command line arguments
  {
    CommandLineSetup cmd(argc, argv);
    cmd.AddUnknownHandler(&HandleUnknownOption);
    SE_ParseCommandLine(cmd);
  }

  // [Cecil] Command line output in the console
  printf("%s", SE_CommandLineOutput().ConstData());

  // there must be 4 or 5 parameters
  if (_ctParsedArgs < 3 || _ctParsedArgs > 5) {
    PrintUsage();
  }

  // [Cecil] Initialize engine with application path from the command line
  SeriousEngineSetup se1setup("Depend");
  se1setup.eAppType = SeriousEngineSetup::E_OTHER;
  se1setup.strSetupRootDir = _fnmAppDir;
  SE_InitEngine(se1setup);

  // Amount of filename arguments
  const INDEX ctFiles = _ctParsedArgs - 2;

  // More than necessary
  if (ctFiles > ARRAYCOUNT(_afnmFiles)) {
    PrintUsage();
  }

  // try to
  try {
    // remove application paths
    for (INDEX iFile = 0; iFile < ctFiles; iFile++) {
      AdjustFilePath_t(_afnmFiles[iFile]);
    }

    // see what option was requested
    switch (_chMode) {
    case 'i': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      CTFileStream strmDep;

      // import files into dependency list from given ascii file
      dl.ImportASCII(_afnmFiles[0]);
      // extract dependencies
      dl.ExtractDependencies();
      // write dependency list
      strmDep.Create_t(_afnmFiles[1]);
      dl.Write_t( &strmDep);
              } break;
    case 'e': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      CTFileStream strmDepIn;

      // read dependency list
  	  strmDepIn.Open_t(_afnmFiles[0], CTStream::OM_READ);
      dl.Read_t( &strmDepIn);
      strmDepIn.Close();
      // export file suitable for archivers
      dl.ExportASCII_t(_afnmFiles[1]);
              } break;
    case 'u': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      CTFileStream strmDepIn, strmDepOut;

      // read dependency list
  	  strmDepIn.Open_t(_afnmFiles[0], CTStream::OM_READ);
      dl.Read_t( &strmDepIn);
      strmDepIn.Close();
      // remove updated files from list
      dl.RemoveUpdatedFiles();
      // write dependency list
      strmDepOut.Create_t(_afnmFiles[1]);
      dl.Write_t( &strmDepOut);
              } break;
    case 'd': {   // UNTESTED!!!!
      if( ctFiles != 3) PrintUsage();
      // load dependency lists
      CDependencyList dl1, dl2;
      CTFileStream strmDep1, strmDep2, strmDepDiff;
  	  strmDep1.Open_t(_afnmFiles[0], CTStream::OM_READ);
  	  strmDep2.Open_t(_afnmFiles[1], CTStream::OM_READ);
      dl1.Read_t( &strmDep1);
      dl2.Read_t( &strmDep2);
      strmDep1.Close();
      strmDep2.Close();
      // calculate difference dependency 1 and 2 (res = 1-2)
      dl1.Substract( dl2);
      // save the difference dependency list
      strmDepDiff.Create_t(_afnmFiles[2]);
      dl1.Write_t( &strmDepDiff);
              } break;
    case 't': {
      if( ctFiles != 2) PrintUsage();
      CDependencyList dl;
      // read file list
      dl.ImportASCII(_afnmFiles[0]);
      // extract translations
      dl.ExtractTranslations_t(_afnmFiles[1]);
              } break;
    default: {
      printf( "Unrecognizable option requested.\n\n");
      PrintUsage();
             }
    }
  }
  catch( char *pError)
  {
    printf( "Error occured.\n%s\n", pError);
  }
}

int main(int argc, char **argv) {
  CTSTREAM_BEGIN {
    SubMain(argc, argv);
  } CTSTREAM_END;

  return 0;
};
