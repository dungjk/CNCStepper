////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2018 Herbert Aitenbichler

  CNCLib is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  CNCLib is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  http://www.gnu.org/licenses/
*/
////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arduino.h>
#include <U8glib.h>

#include <CNCLib.h>
#include <CNCLibEx.h>

#include "Beep.h"
#include "RotaryButton.h"

#include "Menu3D.h"
#include "U8GLCD.h"

#include "GCodeParser.h"
#include "GCode3DParser.h"
#include "GCodeBuilder.h"
#include "SDDirReader.h"

////////////////////////////////////////////////////////////
//
// used full graphic controller for Ramps 1.4 or FD
//
////////////////////////////////////////////////////////////

void CU8GLcd::SetMenuPage()
{
	_currentpage = GetPageCount()-1;	// TODO: last is default menu
	GetMenu().SetMainMenu();
	SetRotaryFocusMenuPage();
	OKBeep();
}

////////////////////////////////////////////////////////////

void CU8GLcd::ButtonPressShowMenu()
{
	SetMenuPage();
}

////////////////////////////////////////////////////////////

uint8_t CU8GLcd::GetMenuIdx(uint8_t addidx)
{
	if (_rotaryFocus == RotaryMenuPage)
	{
		uint8_t menu = _rotarybutton.GetPageIdx(GetMenu().GetMenuItemCount() + addidx);
		if (menu != GetMenu().GetPosition())
		{
			GetMenu().SetPosition(menu);
		}
	}

	return GetMenu().GetPosition();
}

////////////////////////////////////////////////////////////

void CU8GLcd::SetRotaryFocusMenuPage()
{
	bool isFileSelectMenu = 255 != GetMenu().GetMenuDef()->FindMenuIdx(0, [](const CMenuBase::SMenuItemDef* item, uintptr_t) -> bool
	{
		return item->GetText() == MENUENTRY_SDFILES;
	});
	_addMenuItems = 0;
	if (isFileSelectMenu)
	{
		if (_SDFileCount == 255)
		{
			_SDFileCount = 0;
			CSDDirReader dirreader("/", [](File*file) -> bool { return file->isDirectory(); });
			while (dirreader.MoveNext())
			{
				_SDFileCount++;
			}
		}
		_addMenuItems = _SDFileCount - 1;
	}
	else
	{
		_SDFileCount = 255;
	}

	_rotarybutton.SetPageIdx(GetMenu().GetPosition()); _rotarybutton.SetMinMax(0, GetMenu().GetMenuItemCount() - 1 + _addMenuItems, false);
	_rotaryFocus = RotaryMenuPage;
}

////////////////////////////////////////////////////////////

void CU8GLcd::ButtonPressMenuPage()
{
	switch (_rotaryFocus)
	{
		case RotaryMainPage:	SetRotaryFocusMenuPage(); OKBeep();  break;
		case RotaryMenuPage:
		{
			if (!GetMenu().Select())
			{
				ErrorBeep();
			}

			break;
		}
	}
}

////////////////////////////////////////////////////////////

bool CU8GLcd::DrawLoopMenu(EnumAsByte(EDrawLoopType) type, uintptr_t data)
{
	if (type==DrawLoopHeader)			return true;
	if (type==DrawLoopQueryTimerout)	{ *((unsigned long*)data) = 200; return true; }
	if (type!=DrawLoopDraw)				return DrawLoopDefault(type,data);

	SetPosition(ToCol(0), ToRow(0) - HeadLineOffset());
	Print(F("Menu: "));
	Print(GetMenu().GetText());

	uint8_t selectedMenuIdx = 255;
	const uint8_t printFirstLine = 1;
	const uint8_t printLastLine = (TotalRows()- 1);
	const uint8_t menuEntries = GetMenu().GetMenuItemCount();

	if (_rotaryFocus == RotaryMenuPage)
	{
		selectedMenuIdx = GetMenuIdx(_addMenuItems);
	}

	GetMenu().GetMenuHelper().AdjustOffset(menuEntries + _addMenuItems, printFirstLine, printLastLine);

	uint8_t drawidx = 0;
	for (uint8_t menuidx = 0; menuidx < menuEntries; menuidx++)
	{
		auto menuItem = GetMenu().GetItemText(menuidx);
		if (menuItem == MENUENTRY_SDFILES)
		{
			CSDDirReader dirreader("/", [](File*file) -> bool { return file->isDirectory(); });

			while (dirreader.MoveNext())
			{
				uint8_t printtorow = GetMenu().GetMenuHelper().ToPrintLine(printFirstLine, printLastLine, drawidx);
				if (printtorow != 255)
				{
					bool isSelectedMenu = drawidx == selectedMenuIdx && _rotaryFocus == RotaryMenuPage;
					SetPosition(ToCol(isSelectedMenu ? 0 : 1), ToRow(printtorow) + PosLineOffset());
					if (isSelectedMenu)
						Print(F(">"));

					Print(dirreader.Current.name());
				}
				drawidx++;
			}
		}
		else
		{
			uint8_t printtorow = GetMenu().GetMenuHelper().ToPrintLine(printFirstLine, printLastLine, drawidx);
			if (printtorow != 255)
			{
				bool isSelectedMenu = drawidx == selectedMenuIdx && _rotaryFocus == RotaryMenuPage;
				SetPosition(ToCol(isSelectedMenu ? 0 : 1), ToRow(printtorow) + PosLineOffset());
				if (isSelectedMenu)
					Print(F(">"));

				Print(menuItem);
			}
			drawidx++;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////