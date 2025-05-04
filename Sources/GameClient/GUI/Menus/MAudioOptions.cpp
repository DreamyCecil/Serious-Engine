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

#include "MenuPrinting.h"
#include "MenuStuff.h"
#include "MAudioOptions.h"

ENGINE_API extern INDEX snd_iFormat;

#define VOLUME_STEPS 50

static void RefreshSoundFormat(CAudioOptionsMenu &gmAudio) {
  switch (_pSound->GetFormat()) {
    default:
    case CSoundLibrary::SF_NONE:     gmAudio.gm_mgFrequencyTrigger.mg_iSelected = 0; break;
    case CSoundLibrary::SF_11025_16: gmAudio.gm_mgFrequencyTrigger.mg_iSelected = 1; break;
    case CSoundLibrary::SF_22050_16: gmAudio.gm_mgFrequencyTrigger.mg_iSelected = 2; break;
    case CSoundLibrary::SF_44100_16: gmAudio.gm_mgFrequencyTrigger.mg_iSelected = 3; break;
  }

  gmAudio.gm_mgAudioAutoTrigger.mg_iSelected = Clamp(sam_bAutoAdjustAudio, 0, 1);
  gmAudio.gm_mgAudioAPITrigger.mg_iSelected = Clamp(_pShell->GetINDEX("snd_iInterface"), 0L, CAbstractSoundAPI::E_SND_MAX - 1);

  gmAudio.gm_mgWaveVolume.mg_iMinPos = 0;
  gmAudio.gm_mgWaveVolume.mg_iMaxPos = VOLUME_STEPS;
  gmAudio.gm_mgWaveVolume.mg_iCurPos = (INDEX)(_pShell->GetFLOAT("snd_fSoundVolume")*VOLUME_STEPS + 0.5f);
  gmAudio.gm_mgWaveVolume.ApplyCurrentPosition();

  gmAudio.gm_mgMPEGVolume.mg_iMinPos = 0;
  gmAudio.gm_mgMPEGVolume.mg_iMaxPos = VOLUME_STEPS;
  gmAudio.gm_mgMPEGVolume.mg_iCurPos = (INDEX)(_pShell->GetFLOAT("snd_fMusicVolume")*VOLUME_STEPS + 0.5f);
  gmAudio.gm_mgMPEGVolume.ApplyCurrentPosition();

  gmAudio.gm_mgAudioAutoTrigger.ApplyCurrentSelection();
  gmAudio.gm_mgAudioAPITrigger.ApplyCurrentSelection();
  gmAudio.gm_mgFrequencyTrigger.ApplyCurrentSelection();
};

static void ApplyAudioOptions(CMenuGadget *pmg) {
  CAudioOptionsMenu &gmAudio = *(CAudioOptionsMenu *)pmg->GetParentMenu();
  sam_bAutoAdjustAudio = gmAudio.gm_mgAudioAutoTrigger.mg_iSelected;

  if (sam_bAutoAdjustAudio) {
    _pShell->Execute("include \"Scripts\\Addons\\SFX-AutoAdjust.ini\"");
  } else {
    _pShell->SetINDEX("snd_iInterface", gmAudio.gm_mgAudioAPITrigger.mg_iSelected);

    switch (gmAudio.gm_mgFrequencyTrigger.mg_iSelected) {
      // [Cecil] Report reinitialization
      case 0:  _pSound->SetFormat(CSoundLibrary::SF_NONE, TRUE); break;
      case 1:  _pSound->SetFormat(CSoundLibrary::SF_11025_16, TRUE); break;
      case 2:  _pSound->SetFormat(CSoundLibrary::SF_22050_16, TRUE); break;
      case 3:  _pSound->SetFormat(CSoundLibrary::SF_44100_16, TRUE); break;
      default: _pSound->SetFormat(CSoundLibrary::SF_NONE, TRUE);
    }
  }

  RefreshSoundFormat(gmAudio);
  snd_iFormat = _pSound->GetFormat();
};

static void OnWaveVolumeChange(CMenuGadget *, INDEX iCurPos) {
  _pShell->SetFLOAT("snd_fSoundVolume", iCurPos / FLOAT(VOLUME_STEPS));
};

static void WaveSliderChange(CMenuGadget *pmg) {
  CMGSlider &mgSlider = *(CMGSlider *)pmg;
  mgSlider.mg_iCurPos -= 5;
  mgSlider.ApplyCurrentPosition();
};

static void FrequencyTriggerChange(CMenuGadget *pmg, INDEX iDummy) {
  CAudioOptionsMenu &gmAudio = *(CAudioOptionsMenu *)pmg->GetParentMenu();

  sam_bAutoAdjustAudio = 0;
  gmAudio.gm_mgAudioAutoTrigger.mg_iSelected = 0;
  gmAudio.gm_mgAudioAutoTrigger.ApplyCurrentSelection();
};

static void MPEGSliderChange(CMenuGadget *pmg) {
  CMGSlider &mgSlider = *(CMGSlider *)pmg;
  mgSlider.mg_iCurPos -= 5;
  mgSlider.ApplyCurrentPosition();
};

static void OnMPEGVolumeChange(CMenuGadget *, INDEX iCurPos) {
  _pShell->SetFLOAT("snd_fMusicVolume", iCurPos / FLOAT(VOLUME_STEPS));
};

void CAudioOptionsMenu::Initialize_t(void)
{
  // [Cecil] Create array of available sound APIs
  const INDEX ctAPIs = CAbstractSoundAPI::E_SND_MAX;

  if (astrSoundAPIRadioTexts == NULL) {
    astrSoundAPIRadioTexts = new CTString[ctAPIs];

    for (INDEX iAPI = 0; iAPI < ctAPIs; iAPI++) {
      const CTString &strAPI = CAbstractSoundAPI::GetApiName((CAbstractSoundAPI::ESoundAPI)iAPI);
      astrSoundAPIRadioTexts[iAPI] = TRANSV(strAPI);
    }
  }

  // intialize Audio options menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("AUDIO"));
  AddChild(&gm_mgTitle);

  TRIGGER_MG(gm_mgAudioAutoTrigger, 0,
    gm_mgApply, gm_mgFrequencyTrigger, TRANS("AUTO-ADJUST"), astrNoYes);
  gm_mgAudioAutoTrigger.mg_strTip = TRANS("adjust quality to fit your system");

  TRIGGER_MG(gm_mgFrequencyTrigger, 1,
    gm_mgAudioAutoTrigger, gm_mgAudioAPITrigger, TRANS("FREQUENCY"), astrFrequencyRadioTexts);
  gm_mgFrequencyTrigger.mg_strTip = TRANS("select sound quality or turn sound off");
  gm_mgFrequencyTrigger.mg_pOnTriggerChange = &FrequencyTriggerChange;

  TRIGGER_MG(gm_mgAudioAPITrigger, 2,
    gm_mgFrequencyTrigger, gm_mgWaveVolume, TRANS("SOUND SYSTEM"), astrSoundAPIRadioTexts);
  gm_mgAudioAPITrigger.mg_strTip = TRANS("choose sound system (API) to use");
  gm_mgAudioAPITrigger.mg_pOnTriggerChange = NULL;
  gm_mgAudioAPITrigger.mg_ctTexts = ctAPIs; // [Cecil] Amount of available APIs

  gm_mgWaveVolume.mg_boxOnScreen = BoxMediumRow(3);
  gm_mgWaveVolume.SetText(TRANS("SOUND EFFECTS VOLUME"));
  gm_mgWaveVolume.mg_strTip = TRANS("adjust volume of in-game sound effects");
  gm_mgWaveVolume.mg_pmgUp = &gm_mgAudioAPITrigger;
  gm_mgWaveVolume.mg_pmgDown = &gm_mgMPEGVolume;
  gm_mgWaveVolume.mg_pOnSliderChange = &OnWaveVolumeChange;
  gm_mgWaveVolume.mg_pActivatedFunction = &WaveSliderChange;
  AddChild(&gm_mgWaveVolume);

  gm_mgMPEGVolume.mg_boxOnScreen = BoxMediumRow(4);
  gm_mgMPEGVolume.SetText(TRANS("MUSIC VOLUME"));
  gm_mgMPEGVolume.mg_strTip = TRANS("adjust volume of in-game music");
  gm_mgMPEGVolume.mg_pmgUp = &gm_mgWaveVolume;
  gm_mgMPEGVolume.mg_pmgDown = &gm_mgApply;
  gm_mgMPEGVolume.mg_pOnSliderChange = &OnMPEGVolumeChange;
  gm_mgMPEGVolume.mg_pActivatedFunction = &MPEGSliderChange;
  AddChild(&gm_mgMPEGVolume);

  gm_mgApply.mg_bfsFontSize = BFS_LARGE;
  gm_mgApply.mg_boxOnScreen = BoxBigRow(4);
  gm_mgApply.SetText(TRANS("APPLY"));
  gm_mgApply.mg_strTip = TRANS("activate selected options");
  AddChild(&gm_mgApply);
  gm_mgApply.mg_pmgUp = &gm_mgMPEGVolume;
  gm_mgApply.mg_pmgDown = &gm_mgAudioAutoTrigger;
  gm_mgApply.mg_pActivatedFunction = &ApplyAudioOptions;
}

void CAudioOptionsMenu::StartMenu(void)
{
  RefreshSoundFormat(*this);
  CGameMenu::StartMenu();
}

// [Cecil] Render menu background
void CAudioOptionsMenu::RenderBackground(CDrawPort *pdp, bool bSubmenu) {
  CGameMenu::RenderBackground(pdp, bSubmenu);

  const FLOAT fScaleW = (FLOAT)pdp->GetWidth() / 640.0f;
  const FLOAT fScaleH = (FLOAT)pdp->GetHeight() / 480.0f;

  if (_pGUIM->m_toLogoEAX.GetData() != NULL) {
    CTextureData &td = (CTextureData &)*_pGUIM->m_toLogoEAX.GetData();
    const FLOAT fLogoSize = 95;
    const PIX pixLogoW = fLogoSize * pdp->dp_fWideAdjustment;
    const PIX pixLogoH = fLogoSize * td.GetHeight() / td.GetWidth();

    PIX2D vStart((640 - pixLogoW - 35) * fScaleW, (480 - pixLogoH - 7) * fScaleH);
    PIX2D vSize(pixLogoW * fScaleW, pixLogoH * fScaleH);

    pdp->PutTexture(&_pGUIM->m_toLogoEAX, PIXaabbox2D(vStart, vStart + vSize));
  }
};

// [Cecil] Change to the menu
void CAudioOptionsMenu::ChangeTo(void) {
  _pGUIM->ChangeToMenu(&_pGUIM->gmAudioOptionsMenu);
};
