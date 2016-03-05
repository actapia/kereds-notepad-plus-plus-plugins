//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <shlwapi.h>
#include <string.h>
#include <strsafe.h>
#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
bool enable;
TCHAR iniFilePath[MAX_PATH];
void pluginInit(HANDLE hModule)
{
	GetModuleFileName((HMODULE)hModule, iniFilePath, sizeof(iniFilePath));

    // remove the module name : get plugins directory path
    PathRemoveFileSpec(iniFilePath);

	PathAppend(iniFilePath, TEXT("Config"));

	PathAppend(iniFilePath, NPP_PLUGIN_NAME);
	StringCchCat(iniFilePath, MAX_PATH, TEXT(".ini"));
	
	enable = GetPrivateProfileInt(TEXT("Settings"), TEXT("Enable"), 1, iniFilePath) != 0 ? true : false;
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
	WritePrivateProfileString(TEXT("Settings"), TEXT("Enable"), enable ? TEXT("1") : TEXT("0"), iniFilePath);
}


//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Enable"), toggle, NULL, enable);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    StringCchCopy(funcItem[index]._itemName, sizeof(funcItem[index]._itemName), cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void toggle()
{
	enable = !enable;

	HWND curScintilla = getCurrentScintilla();
	HMENU hMenu = GetMenu(nppData._nppHandle);
    if (hMenu)
    {
        CheckMenuItem(hMenu, funcItem[0]._cmdID, MF_BYCOMMAND | (enable ? MF_CHECKED : MF_UNCHECKED));
    }
}

void pythonAutoIndent()
{
	HWND curScintilla = getCurrentScintilla();

	int lang = -100;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&lang);

	if(enable && lang == L_PYTHON)
	{
		int position    = SendMessage(curScintilla, SCI_GETCURRENTPOS, 0, 0);
		int line_number = SendMessage(curScintilla, SCI_LINEFROMPOSITION, position, 0);

		SendMessage(curScintilla, SCI_BEGINUNDOACTION, 0, 0);

		if(shouldIndent(curScintilla, line_number - 1))
		{
			SendMessage(curScintilla, SCI_TAB, 0, 0 );
		}

		SendMessage(curScintilla, SCI_ENDUNDOACTION, 0, 0);
	}
}

/* Helper functions. */
HWND getCurrentScintilla()
{
    int which = -1;
    SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return NULL;
    if (which == 0)
    {
        return nppData._scintillaMainHandle;
    }
    else
    {
        return nppData._scintillaSecondHandle;
    }
    return nppData._scintillaMainHandle;
}

char* clauseHeaders[] = {"if", "elif", "else",
					     "for", "while",
					     "try", "except", "finally",
					     "with",
					     "def", "class",
	                     NULL};
size_t clauseHeadersLen[] = {strlen(clauseHeaders[0]),
	                         strlen(clauseHeaders[1]),
							 strlen(clauseHeaders[2]),
							 strlen(clauseHeaders[3]),
							 strlen(clauseHeaders[4]),
							 strlen(clauseHeaders[5]),
							 strlen(clauseHeaders[6]),
							 strlen(clauseHeaders[7]),
							 strlen(clauseHeaders[8]),
							 strlen(clauseHeaders[9]),
							 strlen(clauseHeaders[10])};

bool shouldIndent(HWND& curScintilla, int line_number)
{
	bool ret = false;

	int line_start = SendMessage(curScintilla, SCI_POSITIONFROMLINE, line_number, 0);
	int line_end   = SendMessage(curScintilla, SCI_GETLINEENDPOSITION, line_number, 0);

	int line_length = line_end - line_start;
	char* line = new char[line_length + 1];

	int save_position = SendMessage(curScintilla, SCI_GETCURRENTPOS, 0, 0);

	SendMessage(curScintilla, SCI_SETSEL, line_start, line_end);
	SendMessage(curScintilla, SCI_GETSELTEXT, 0, (LPARAM)line);

	/* Find the start of code. */
	int i;
	for(i = 0; i < line_length; i++)
	{
		if(line[i] != ' ' && line[i] != '\t')
		{
			break;
		}
	}

	/* Compare the first word in the code against the headers. Auto-indent only if
	   one of the headers is found. */
	for(int j = 0; clauseHeaders[j] != NULL; j++)
	{
		if(strncmp(&line[i], clauseHeaders[j], strlen(clauseHeaders[j])) == 0 && !isVarName(line[i+clauseHeadersLen[j]]))
		{
			ret = true;
			break;
		}
	}

	delete line;

	SendMessage(curScintilla, SCI_SETSEL, save_position, save_position);

	return ret;
}

/* Returns true if the character is valid in a variable name. */
bool isVarName(char c)
{
	return (c >= 'a' && c <= 'z') ||
	       (c >= 'A' && c <= 'Z') ||
		   (c >= '0' && c <= '9') ||
		   (c == '_');
}