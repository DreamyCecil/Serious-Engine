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

#include "MGEdit.h"

CMGEdit::CMGEdit(void)
{
  mg_pstrToChange = NULL;
  mg_ctMaxStringLen = 70;
  Clear();
}

void CMGEdit::Clear(void)
{
  mg_iCursorPos = 0;
  mg_bEditing = FALSE;

  // [Cecil] Because of constructor
  if (_pGUIM != NULL) {
    _pGUIM->m_bEditingString = FALSE;
  }
}

void CMGEdit::OnActivate(void)
{
  if (!mg_bEnabled) {
    // [Cecil] Textbox is disabled
    _pGUIM->PlayMenuSound(CMenuManager::E_MSND_DISABLED);
    return;
  }
  ASSERT(mg_pstrToChange != NULL);
  _pGUIM->PlayMenuSound(CMenuManager::E_MSND_PRESS);
  mg_iCursorPos = GetText().Length();
  mg_bEditing = TRUE;
  _pGUIM->m_bEditingString = TRUE;
}

// focus lost
void CMGEdit::OnKillFocus(void)
{
  // go out of editing mode
  if (mg_bEditing) {
    OnKeyDown(PressedMenuButton(SE1K_RETURN, -1, -1));
    Clear();
  }
  // proceed
  CMenuGadget::OnKillFocus();
}

// helper function for deleting char(s) from string
static void Key_BackDel(CTString &str, INDEX &iPos, BOOL bShift, BOOL bRight)
{
  // do nothing if string is empty
  INDEX ctChars = str.Length();
  if (ctChars == 0) return;
  if (bRight && iPos<ctChars) {  // DELETE key
    if (bShift) {
      // delete to end of line
      str.TrimRight(iPos);
    } else {
      // delete only one char
      str.DeleteChar(iPos);
    }
  }
  if (!bRight && iPos>0) {       // BACKSPACE key
    if (bShift) {
      // delete to start of line
      str.TrimLeft(ctChars - iPos);
      iPos = 0;
    } else {
      // delete only one char
      str.DeleteChar(iPos - 1);
      iPos--;
    }
  }
}

// key/mouse button pressed
BOOL CMGEdit::OnKeyDown(PressedMenuButton pmb)
{
  // if not in edit mode
  if (!mg_bEditing) {
    // behave like normal gadget
    return CMenuGadget::OnKeyDown(pmb);
  }

  CTString strText = GetText();

  // [Cecil] Apply changes
  if (pmb.Apply(TRUE)) {
    *mg_pstrToChange = strText;
    Clear();
    OnStringChanged();
    return TRUE;
  }

  // [Cecil] Discard changes
  if (pmb.Back(TRUE)) {
    SetText(*mg_pstrToChange);
    Clear();
    OnStringCanceled();
    return TRUE;
  }

  // [Cecil] Move left
  if (pmb.Up() || pmb.Left()) {
    if (mg_iCursorPos > 0) mg_iCursorPos--;
    return TRUE;
  }

  // [Cecil] Move right
  if (pmb.Down() || pmb.Right()) {
    if (mg_iCursorPos < strText.Length()) mg_iCursorPos++;
    return TRUE;
  }

  const BOOL bShift = !!(OS::GetKeyState(SE1K_LSHIFT) & 0x8000) || !!(OS::GetKeyState(SE1K_RSHIFT) & 0x8000);

  switch (pmb.iKey) {
    case SE1K_HOME: mg_iCursorPos = 0; break;
    case SE1K_END:  mg_iCursorPos = strText.Length(); break;

    case SE1K_BACKSPACE: {
      Key_BackDel(strText, mg_iCursorPos, bShift, FALSE);
      SetText(strText);
    } break;

    case SE1K_DELETE: {
      Key_BackDel(strText, mg_iCursorPos, bShift, TRUE);
      SetText(strText);
    } break;
  }

  // Ignore all other special keys and mark them as handled
  return TRUE;
}

// char typed
BOOL CMGEdit::OnChar(const OS::SE1Event &event)
{
  // if not in edit mode
  if (!mg_bEditing) {
    // behave like normal gadget
    return CMenuGadget::OnChar(event);
  }

  CTString strText = GetText();

  // only chars are allowed
  const INDEX ctFullLen = strText.Length();
  const INDEX ctNakedLen = strText.LengthNaked();
  mg_iCursorPos = Clamp(mg_iCursorPos, 0L, ctFullLen);
  int iVKey = event.key.code;
  if (isprint(iVKey) && ctNakedLen <= mg_ctMaxStringLen) {
    strText.InsertChar(mg_iCursorPos, (char)iVKey);
    SetText(strText);
    mg_iCursorPos++;
  }
  // key is handled
  return TRUE;
}

void CMGEdit::Render(CDrawPort *pdp)
{
  if (mg_bEditing) {
    mg_iTextMode = -1;
  } else if (mg_bFocused) {
    mg_iTextMode = 0;
  } else {
    mg_iTextMode = 1;
  }

  if (GetText() == "" && !mg_bEditing) {
    if (mg_bfsFontSize == BFS_SMALL) {
      SetText("*");
    } else {
      SetText(TRANS("<none>"));
    }
    CMGButton::Render(pdp);
    SetText("");
  } else {
    CMGButton::Render(pdp);
  }
}

void CMGEdit::OnStringChanged(void)
{
}

void CMGEdit::OnStringCanceled(void)
{
}
