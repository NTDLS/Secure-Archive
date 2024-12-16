///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2023, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NSWFL_H_
#define _NSWFL_H_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>

//The memory pool class is used to track memory allocations to ease the task of memory leak detection. It can be removed from release code with preprocessors.
#include "NSWFL_MemoryPool.H"

//Type conversions and parsers. Dates, strings, int, float, boolean, you name it.
#include "NSWFL_Conversion.H"

//Getting, comparing and converting various date/times.
#include "NSWFL_DateTime.H"

//Random number an string generator.
#include "NSWFL_KeyGeneration.H"

//Functions for working with WinAPI list boxes.
#include "NSWFL_ListBox.H"

//Its math. What did you expect?
#include "NSWFL_Math.H"

//Memory operations, clear, set, copy, etc.
#include "NSWFL_Memory.H"

//Everything you need to access the registry.
#include "NSWFL_Registry.H"

//String manipulation.
#include "NSWFL_String.H"

//OS level functions, like get system name.
#include "NSWFL_System.H"

//Lots of functions for managing windows or anything with an HWND.
#include "NSWFL_Windows.H"

//File access functions.
#include "NSWFL_File.H"

//Functions for working with WinAPI menus.
#include "NSWFL_Menu.H"

//Functions for working with WinAPI list views.
#include "NSWFL_ListView.H"

//Debugging functions/
#include "NSWFL_Debug.H"

//Functions for parsing arguments passed to the command line.
#include "NSWFL_CommandLineParser.H"

//A reasonable string builder for C++, finally!
#include "NSWFL_StringBuilder.H"

//CRC32 (cyclic redundancy check) calculation.
#include "NSWFL_CRC32.H"

//SHA1 hashing functions.
#include "NSWFL_SHA1.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
