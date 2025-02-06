/* Copyright (c) 2025 Dreamy Cecil
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

// Common command line options

static void OptionGame(const CommandLineArgs_t &aArgs) {
  // [Cecil] TEMP: Use base directory for the default mod
  if (aArgs[0] != "SeriousSam") {
    _fnmMod = "Mods\\" + aArgs[0] + "\\";
  }
};

static void OptionLogFile(const CommandLineArgs_t &aArgs) {
  _strLogFile = aArgs[0];
};

// Separate a string into multiple arguments (e.g. command line arguments)
// Implemented according to the rules from Microsoft docs:
// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170
static void StringToArgs(const char *str, CommandLineArgs_t &aArgs)
{
  CStaticStackArray<char> aCurrentString; // Current string as an array of characters
  bool bString = false; // String within double quotes

  while (*str != '\0') {
    // Hit a double quote
    if (*str == '"') {
      ++str;

      // A pair of double quotes within a string
      if (bString && *str == '"') {
        // Just add quotes to the argument
        aCurrentString.Add('"');
        ++str;

      // Enter or exit the string
      } else {
        bString = !bString;
      }

    // Hit an escape character
    } else if (*str == '\\') {
      // Within a string
      if (bString) {
        // Skip if it's followed by another one or a quote
        if (str[1] == '\\' || str[1] == '"') {
          ++str;
        }

        // Add the backslash or the escaped character and advance
        aCurrentString.Add(*str);
        ++str;

      } else {
        // Check if escape characters preceed a double quote
        bool bUntilQuote = false;
        const char *pchCheck = str + 1;

        while (*pchCheck != '\0') {
          // Hit a double quote
          if (*pchCheck == '"') {
            bUntilQuote = true;
            break;

          // Hit something else
          } else if (*pchCheck != '\\') {
            break;
          }

          ++pchCheck;
        }

        if (bUntilQuote) {
          // Convert \\ -> \ and \" -> "
          do {
            aCurrentString.Add(str[1]);
            str += 2;
          } while (*str == '\\');

        } else {
          // Add each espace character
          do {
            aCurrentString.Add(*str);
            ++str;
          } while (*str == '\\');
        }
      }

    // Hit an argument separator outside a string
    } else if (!bString && isspace(static_cast<UBYTE>(*str))) {
      // Save current null-terminated string and reset it
      aCurrentString.Add('\0');
      aArgs.Add(aCurrentString.sa_Array);
      aCurrentString.PopAll();

      // Go until the next valid character
      do {
        ++str;
      } while (isspace(static_cast<UBYTE>(*str)));

    // Add any other character to the string and advance
    } else {
      aCurrentString.Add(*str);
      ++str;
    }
  }

  // Last argument
  if (aCurrentString.Count() != 0) {
    aCurrentString.Add('\0');
    aArgs.Add(aCurrentString.sa_Array);
  }
};

// Add common command line option processors
inline void AddCommonProcessors(CommandLineSetup *pCmd) {
  pCmd->AddCommand("+game",    &OptionGame,    1);
  pCmd->AddCommand("+logfile", &OptionLogFile, 1);
};

// Constructor for C-like command line
CommandLineSetup::CommandLineSetup(int argc, char **argv) :
  pInitialArgs(NULL, -1), pUnknownOption(NULL)
{
  GetArguments(argc, argv, aArgs);
  AddCommonProcessors(this);

  // Cache the entire command line as a single string
  strCommandLine = "";

  for (int i = 1; i < argc; i++) {
    // Next argument (separated with spaces)
    if (i != 1) strCommandLine += " ";

    // Print arguments within quotes
    strCommandLine += "\"" + CTString(argv[i]) + "\"";
  }
};

// Constructor for the entire command line
CommandLineSetup::CommandLineSetup(const char *strCmd) :
  pInitialArgs(NULL, -1), pUnknownOption(NULL)
{
  GetArguments(strCmd, aArgs);
  AddCommonProcessors(this);
  strCommandLine = strCmd;
};

// Gather command line arguments from a C-like command line
void CommandLineSetup::GetArguments(int argc, char **argv, CommandLineArgs_t &aArgs) {
  // Skip the executable filename from the first argument
  for (int i = 1; i < argc; i++) {
    // Stop at non-existent strings (should only be at argv[argc], which isn't parsed here)
    ASSERT(argv[i] != NULL);
    if (argv[i] == NULL) break;

    aArgs.Add(argv[i]);
  }
};

// Gather command line arguments from the entire command line
void CommandLineSetup::GetArguments(const char *strCmd, CommandLineArgs_t &aArgs) {
  StringToArgs(strCmd, aArgs);
};

// Parse the command line according to the setup
// Returns TRUE if managed to handle at least one command
BOOL CommandLineSetup::Parse(CTString &strOutput) const {
  BOOL bResult = FALSE;

  // Arguments for the current command
  CommandLineArgs_t aCmdArgs;
  const INDEX ct = Count();

  for (INDEX i = 0; i < ct; i++) {
    const char *strCmd = NULL;
    const CommandLineFunction_t *pCmdFunc = NULL;

    // Parse initial arguments
    if (i == 0 && pInitialArgs.pFunc != NULL) {
      strCmd = "Program executable";
      pCmdFunc = &pInitialArgs;

      // Pretend that this is a real option so it pushes arguments starting from 0
      i--;

    } else {
      // Get command name
      strCmd = aArgs[i].ConstData();

      // Try to find the command
      CommandLineSetup::Commands_t::const_iterator itCmd = mapCommands.find(strCmd);

      // Command not found
      if (itCmd == mapCommands.end()) {
        // Handle unknown commands
        BOOL bHandled = FALSE;

        if (pUnknownOption != NULL) {
          bHandled = pUnknownOption(strCmd);
        }

        // Not handled
        if (!bHandled) {
          strOutput += CTString(0, TRANS("  Unknown option: '%s'\n"), strCmd);

        // Handled at least one command
        } else {
          bResult = TRUE;
        }

        // Skip to the next argument
        continue;
      }

      // Get command line function from the command
      pCmdFunc = &itCmd->second;
    }

    // Get command function
    FCommandLineCallback pFunc = pCmdFunc->pFunc;

    INDEX ctArgs = pCmdFunc->ctArgs;
    INDEX ctLeft = ct - (i + 1);

    // Not enough arguments for the last command
    if (ctArgs > ctLeft) {
      ASSERTALWAYS("Not enough arguments in the command line!");
      strOutput += CTString(0, TRANS("  '%s' requires %d arguments!\n"), strCmd, ctArgs);
      break; // Don't parse next arguments as commands
    }

    // Retrieve required amount of arguments for the command
    while (--ctArgs >= 0) {
      aCmdArgs.Add(aArgs[++i]);
    }

    // Call command function with the arguments
    pFunc(aCmdArgs);

    // Handled at least one command
    bResult = TRUE;

    // Clear arguments for the next command
    aCmdArgs.PopAll();
  }

  return bResult;
};

// Command line parser output
static CTString _strCmdOutput = "";

// Parse the command line according to the setup (can be done before initializing the engine)
void SE_ParseCommandLine(const CommandLineSetup &cmd)
{
  _strCmdOutput.PrintF(TRANS("Command line: '%s'\n"), cmd.strCommandLine.ConstData());
  cmd.Parse(_strCmdOutput);
};

// Get output generated by the command line parser (e.g. for printing upon engine initialization)
CTString &SE_CommandLineOutput(void) {
  return _strCmdOutput;
};
