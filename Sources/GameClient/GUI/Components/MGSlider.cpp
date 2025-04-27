/* /* Copyright (c) 2002-2012 Croteam Ltd. 
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

#include "MGSlider.h"

extern FLOAT _fCursorPosI;
extern FLOAT _fCursorPosJ;
extern INDEX sam_bWideScreen;


CMGSlider::CMGSlider()
{
  mg_iMinPos = 0;
  mg_iMaxPos = 16;
  mg_iCurPos = 8;
  mg_pOnSliderChange = NULL;
  mg_fFactor = 1.0f;
}

void CMGSlider::ApplyCurrentPosition(void)
{
  mg_iCurPos = Clamp(mg_iCurPos, mg_iMinPos, mg_iMaxPos);
  FLOAT fStretch = FLOAT(mg_iCurPos) / (mg_iMaxPos - mg_iMinPos);
  mg_fFactor = fStretch;

  if (mg_pOnSliderChange != NULL) {
    mg_pOnSliderChange(mg_iCurPos);
  }
}

void CMGSlider::ApplyGivenPosition(INDEX iMin, INDEX iMax, INDEX iCur)
{
  mg_iMinPos = iMin;
  mg_iMaxPos = iMax;
  mg_iCurPos = iCur;
  ApplyCurrentPosition();
}


BOOL CMGSlider::OnKeyDown(PressedMenuButton pmb)
{
  // [Cecil] Increase/decrease the value
  const INDEX iPower = pmb.ChangeValue();

  if (iPower != 0) {
    mg_iCurPos = Clamp(mg_iCurPos + iPower, mg_iMinPos, mg_iMaxPos);
    ApplyCurrentPosition();
    return TRUE;
  }

  // if lmb pressed
  if (pmb.iMouse == SDL_BUTTON_LEFT) {
    // get position of slider box on screen
    PIXaabbox2D boxSlider = GetSliderBox();
    // if mouse is within
    if (boxSlider >= PIX2D(_fCursorPosI, _fCursorPosJ)) {
      // set new position exactly where mouse pointer is
      FLOAT fRatio = FLOAT(_fCursorPosI - boxSlider.Min()(1)) / boxSlider.Size()(1);
      fRatio = (fRatio - 0.01f) / (0.99f - 0.01f);
      fRatio = Clamp(fRatio, 0.0f, 1.0f);
      mg_iCurPos = fRatio*(mg_iMaxPos - mg_iMinPos) + mg_iMinPos;
      ApplyCurrentPosition();
      return TRUE;
    }
  }

  return CMenuGadget::OnKeyDown(pmb);
}

PIXaabbox2D CMGSlider::GetSliderBox(void)
{
  extern CDrawPort *pdp;
  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIR = box.Min()(1) + box.Size()(1)*0.55f;
  PIX pixJ = box.Min()(2);
  PIX pixJSize = box.Size()(2)*0.95f;
  PIX pixISizeR = box.Size()(1)*0.45f;
  if (sam_bWideScreen) pixJSize++;
  return PIXaabbox2D(PIX2D(pixIR + 1, pixJ + 1), PIX2D(pixIR + pixISizeR - 2, pixJ + pixJSize - 2));
}

void CMGSlider::Render(CDrawPort *pdp)
{
  SetFontMedium(pdp);

  // get geometry
  COLOR col = GetCurrentColor();
  PIXaabbox2D box = FloatBoxToPixBox(pdp, mg_boxOnScreen);
  PIX pixIL = box.Min()(1) + box.Size()(1)*0.45f;
  PIX pixIR = box.Min()(1) + box.Size()(1)*0.55f;
  PIX pixJ = box.Min()(2);
  PIX pixJSize = box.Size()(2)*0.95f;
  PIX pixISizeR = box.Size()(1)*0.45f;
  if (sam_bWideScreen) pixJSize++;

  // print text left of slider
  pdp->PutTextR(GetText(), pixIL, pixJ, col);

  // draw box around slider
  _pGame->LCDDrawBox(0, -1, PIXaabbox2D(PIX2D(pixIR + 1, pixJ), PIX2D(pixIR + pixISizeR - 2, pixJ + pixJSize - 2)),
    _pGame->LCDGetColor(C_GREEN | 255, "slider box"));

  // draw filled part of slider
  pdp->Fill(pixIR + 2, pixJ + 1, (pixISizeR - 5)*mg_fFactor, (pixJSize - 4), col);

  // print percentage text
  CTString strPercentage;
  strPercentage.PrintF("%d%%", (int)floor(mg_fFactor * 100 + 0.5f));
  pdp->PutTextC(strPercentage, pixIR + pixISizeR / 2, pixJ + 1, col);
}