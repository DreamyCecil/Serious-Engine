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

#include "MGFileButton.h"

CMGFileButton::CMGFileButton(void)
{
  mg_iState = FBS_NORMAL;
}

// [Cecil] TEMP: Get load/save menu of this gadget
CLoadSaveMenu *CMGFileButton::GetLSMenu(void) const {
  ASSERT(GetParentMenu()->GetName() == CTString("LoadSave"));
  return (CLoadSaveMenu *)GetParentMenu();
};

// refresh current text from description
void CMGFileButton::RefreshText(void)
{
  CTString strText = mg_strDes;
  strText.OnlyFirstLine();
  SetText(strText);
  mg_strInfo = mg_strDes;
  mg_strInfo.RemovePrefix(strText);
  mg_strInfo.DeleteChar(0);
}

void CMGFileButton::SaveDescription(void)
{
  CTFileName fnFileNameDescription = mg_fnm.NoExt() + ".des";
  try {
    mg_strDes.Save_t(fnFileNameDescription);
  } catch (char *strError) {
    CPrintF("%s\n", strError);
  }
}

CMGFileButton *_pmgFileToSave = NULL;

static void OnFileSaveOK(void) {
  if (_pmgFileToSave != NULL) {
    _pmgFileToSave->SaveYes();
  }
};

static void SaveConfirm(void) {
  CConfirmMenu::ChangeTo(TRANS("OVERWRITE?"), &OnFileSaveOK, NULL, TRUE);
};

void CMGFileButton::DoSave(void)
{
  if (FileExistsForWriting(mg_fnm)) {
    _pmgFileToSave = this;
    SaveConfirm();
  } else {
    SaveYes();
  }
}

void CMGFileButton::SaveYes(void)
{
  CLoadSaveMenu *pgmLoadSave = GetLSMenu();
  ASSERT(pgmLoadSave->gm_bSave);

  // call saving function
  BOOL bSucceeded = pgmLoadSave->gm_pAfterFileChosen(pgmLoadSave, mg_fnm);

  // if saved
  if (bSucceeded) {
    // save the description too
    SaveDescription();
  }
}

void CMGFileButton::DoLoad(void)
{
  CLoadSaveMenu *pgmLoadSave = GetLSMenu();
  ASSERT(!pgmLoadSave->gm_bSave);

  // if no file
  if (!FileExists(mg_fnm)) {
    // do nothing
    return;
  }

  // call loading function
  BOOL bSucceeded = pgmLoadSave->gm_pAfterFileChosen(pgmLoadSave, mg_fnm);
  ASSERT(bSucceeded);
}

static CTString _strTmpDescription;
static CTString _strOrgDescription;

void CMGFileButton::StartEdit(void)
{
  CMGEdit::OnActivate();
}

void CMGFileButton::OnActivate(void)
{
  if (mg_fnm == "") {
    return;
  }

  PlayMenuSound(E_MSND_PRESS);

  CLoadSaveMenu *pgmLoadSave = GetLSMenu();

  // if loading
  if (!pgmLoadSave->gm_bSave) {
    // load now
    DoLoad();
    // if saving
  } else {
    const CTString &strText = GetText();

    // switch to editing mode
    BOOL bWasEmpty = strText == EMPTYSLOTSTRING;
    mg_strDes = pgmLoadSave->gm_strSaveDes;
    RefreshText();
    _strOrgDescription = _strTmpDescription = strText;

    if (bWasEmpty) {
      _strOrgDescription = EMPTYSLOTSTRING;
    }

    mg_pstrToChange = &_strTmpDescription;
    StartEdit();
    mg_iState = FBS_SAVENAME;
  }
}

BOOL CMGFileButton::OnKeyDown(PressedMenuButton pmb)
{
  CLoadSaveMenu *pgmLoadSave = GetLSMenu();

  if (mg_iState == FBS_NORMAL) {
    if (pgmLoadSave->gm_bSave || pgmLoadSave->gm_bManage) {
      if (pmb.iKey == SE1K_F2) {
        if (FileExistsForWriting(mg_fnm)) {
          // switch to renaming mode
          const CTString &strText = GetText();
          _strOrgDescription = strText;
          _strTmpDescription = strText;
          mg_pstrToChange = &_strTmpDescription;
          StartEdit();
          mg_iState = FBS_RENAME;
        }
        return TRUE;

      } else if (pmb.iKey == SE1K_DELETE) {
        if (FileExistsForWriting(mg_fnm)) {
          // delete the file, its description and thumbnail
          RemoveFile(mg_fnm);
          RemoveFile(mg_fnm.NoExt() + ".des");
          RemoveFile(mg_fnm.NoExt() + "Tbn.tex");
          // refresh menu
          pgmLoadSave->ReloadMenu();
          OnSetFocus();
        }
        return TRUE;
      }
    }
    return CMenuGadget::OnKeyDown(pmb);
  } else {
    // go out of editing mode
    if (mg_bEditing) {
      if (pmb.Up() || pmb.Down()) {
        CMGEdit::OnKeyDown(PressedMenuButton(SE1K_ESCAPE, -1, -1));
      }
    }
    return CMGEdit::OnKeyDown(pmb);
  }
}

void CMGFileButton::OnSetFocus(void)
{
  CLoadSaveMenu *pgmLoadSave = GetLSMenu();
  mg_iState = FBS_NORMAL;

  if (pgmLoadSave->gm_bAllowThumbnails && mg_bEnabled) {
    SetThumbnail(mg_fnm);
  } else {
    ClearThumbnail();
  }

  _pGUIM->GetCurrentMenu()->KillAllFocuses();
  CMGButton::OnSetFocus();
}

void CMGFileButton::OnKillFocus(void)
{
  // go out of editing mode
  if (mg_bEditing) {
    OnKeyDown(PressedMenuButton(SE1K_ESCAPE, -1, -1));
  }

  CMGEdit::OnKillFocus();
}

// override from edit gadget
void CMGFileButton::OnStringChanged(void)
{
  CLoadSaveMenu *pgmLoadSave = GetLSMenu();

  // if saving
  if (mg_iState == FBS_SAVENAME) {
    // do the save
    mg_strDes = _strTmpDescription + "\n" + mg_strInfo;
    DoSave();
  // if renaming
  } else if (mg_iState == FBS_RENAME) {
    // do the rename
    mg_strDes = _strTmpDescription + "\n" + mg_strInfo;
    SaveDescription();
    // refresh menu
    pgmLoadSave->ReloadMenu();
    OnSetFocus();
  }
}

void CMGFileButton::OnStringCanceled(void)
{
  SetText(_strOrgDescription);
}

void CMGFileButton::Render(CDrawPort *pdp)
{
  // render original gadget first
  CMGEdit::Render(pdp);

  // if currently selected
  if (mg_bFocused && mg_bEnabled) {
    // add info at the bottom if screen
    SetFontMedium(pdp);

    PIXaabbox2D box = FloatBoxToPixBox(pdp, BoxSaveLoad(15.0));
    PIX pixI = box.Min()(1);
    PIX pixJ = box.Min()(2);

    COLOR col = _pGame->LCDGetColor(C_mlGREEN | 255, "file info");
    pdp->PutText(mg_strInfo, pixI, pixJ, col);
  }
}
