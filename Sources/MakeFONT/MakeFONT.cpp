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


// MakeFONT - Font table File Creator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Engine/Engine.h>

// Command line arguments
static CTString _fnmTexture;
static ULONG _ulCharWidth;
static ULONG _ulCharHeight;
static CTString _fnmOrder;
static BOOL _bUseAlpha = TRUE; // Use alpha by default

// [Cecil] Parsed arguments
static INDEX _ctParsedArgs = 0;

// [Cecil] Handle program's launch arguments
static void HandleInitialArgs(const CommandLineArgs_t &aArgs) {
  _ctParsedArgs = aArgs.Count();

  _fnmTexture = aArgs[0];
  _ulCharWidth  = strtoul(aArgs[1].ConstData(), NULL, 10);
  _ulCharHeight = strtoul(aArgs[2].ConstData(), NULL, 10);
  _fnmOrder = aArgs[3];

  if (_ctParsedArgs > 4 && aArgs[4].HasPrefix("-A")) {
    _bUseAlpha = FALSE;
  }
};

void SubMain(int argc, char **argv) {
  // [Cecil] Parse command line arguments
  {
    CommandLineSetup cmd(argc, argv);
    cmd.AddInitialParser(&HandleInitialArgs, (argc > 5) ? 5 : 4); // Take optional argument into account
    SE_ParseCommandLine(cmd);
  }

  printf("\nMakeFONT - Font Tables Maker (2.51)\n");
  printf(  "           (C)1999 CROTEAM Ltd\n\n");

  // [Cecil] Command line output in the console
  printf("%s", SE_CommandLineOutput().ConstData());

  // [Cecil] Should only parse 4 or 5 of own arguments
  if (_ctParsedArgs < 4 || _ctParsedArgs > 5)
  {
    printf( "USAGE: MakeFont <texture_file> <char_width> <char_height> ");
    printf( "<char_order_file> [-A]\n");
    printf( "\n");
    printf( "texture_file: FULL PATH to texture file that represents font\n");
    printf( "char_width: maximum width (in pixels) of single character\n");
    printf( "char_height: maximum height (in pixels) of single character\n");
    printf( "char_order_file: full path to ASCII file that shows\n");
    printf( "                 graphical order of character in font texture\n");
    printf( "-A: do not include alpha channel when determining character width \n");
    printf( "\n");
    printf( "NOTES: - out file will have the name as texture file, but \".fnt\" extension\n");
    printf( "       - texture file must begin with character that will be a replacement for\n");
    printf( "         each character that hasn't got definition in this texture file\n");
    exit( EXIT_FAILURE);
  }

  // initialize engine
  SeriousEngineSetup se1setup("MakeFONT");
  se1setup.eAppType = SeriousEngineSetup::E_OTHER;
  SE_InitEngine(se1setup);

  // font generation starts
  printf( "- Generating font table.\n");
  // try to create font
  CFontData fdFontData;
  try
  { 
    // remove application path from font texture file
    _fnmTexture.RemoveApplicationPath_t();
    // create font
    fdFontData.Make_t(_fnmTexture, _ulCharWidth, _ulCharHeight, _fnmOrder, _bUseAlpha);
  }
  // catch and report errors
  catch(char *strError)
  {
    printf( "! Cannot create font.\n  %s\n", strError);
    exit(EXIT_FAILURE);
  }
  
  // save processed data
  printf( "- Saving font table file.\n");
  // create font name
  CTFileName strFontFileName = _fnmTexture.NoExt() + ".fnt";
  // try to
  try
  {
    fdFontData.Save_t( strFontFileName);
  }
  catch(char *strError)
  {
    printf("! Cannot save font.\n  %s\n", strError);
    exit(EXIT_FAILURE);
  }
  printf("- '%s' created successfuly.\n", strFontFileName.ConstData());
  
  exit( EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  CTSTREAM_BEGIN {
    SubMain(argc, argv);
  } CTSTREAM_END;

  getch();
  return 0;
}
