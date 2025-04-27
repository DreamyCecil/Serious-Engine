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

static void RefreshSoundFormat(void) {
  CAudioOptionsMenu &gmCurrent = _pGUIM->gmAudioOptionsMenu;

  switch (_pSound->GetFormat()) {
    default:
    case CSoundLibrary::SF_NONE:     gmCurrent.gm_mgFrequencyTrigger.mg_iSelected = 0; break;
    case CSoundLibrary::SF_11025_16: gmCurrent.gm_mgFrequencyTrigger.mg_iSelected = 1; break;
    case CSoundLibrary::SF_22050_16: gmCurrent.gm_mgFrequencyTrigger.mg_iSelected = 2; break;
    case CSoundLibrary::SF_44100_16: gmCurrent.gm_mgFrequencyTrigger.mg_iSelected = 3; break;
  }

  gmCurrent.gm_mgAudioAutoTrigger.mg_iSelected = Clamp(sam_bAutoAdjustAudio, 0, 1);
  gmCurrent.gm_mgAudioAPITrigger.mg_iSelected = Clamp(_pShell->GetINDEX("snd_iInterface"), 0L, CAbstractSoundAPI::E_SND_MAX - 1);

  gmCurrent.gm_mgWaveVolume.mg_iMinPos = 0;
  gmCurrent.gm_mgWaveVolume.mg_iMaxPos = VOLUME_STEPS;
  gmCurrent.gm_mgWaveVolume.mg_iCurPos = (INDEX)(_pShell->GetFLOAT("snd_fSoundVolume")*VOLUME_STEPS + 0.5f);
  gmCurrent.gm_mgWaveVolume.ApplyCurrentPosition();

  gmCurrent.gm_mgMPEGVolume.mg_iMinPos = 0;
  gmCurrent.gm_mgMPEGVolume.mg_iMaxPos = VOLUME_STEPS;
  gmCurrent.gm_mgMPEGVolume.mg_iCurPos = (INDEX)(_pShell->GetFLOAT("snd_fMusicVolume")*VOLUME_STEPS + 0.5f);
  gmCurrent.gm_mgMPEGVolume.ApplyCurrentPosition();

  gmCurrent.gm_mgAudioAutoTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgAudioAPITrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgFrequencyTrigger.ApplyCurrentSelection();
};

static void ApplyAudioOptions(void) {
  CAudioOptionsMenu &gmCurrent = _pGUIM->gmAudioOptionsMenu;
  sam_bAutoAdjustAudio = gmCurrent.gm_mgAudioAutoTrigger.mg_iSelected;

  if (sam_bAutoAdjustAudio) {
    _pShell->Execute("include \"Scripts\\Addons\\SFX-AutoAdjust.ini\"");
  } else {
    _pShell->SetINDEX("snd_iInterface", gmCurrent.gm_mgAudioAPITrigger.mg_iSelected);

    switch (gmCurrent.gm_mgFrequencyTrigger.mg_iSelected) {
      // [Cecil] Report reinitialization
      case 0:  _pSound->SetFormat(CSoundLibrary::SF_NONE, TRUE); break;
      case 1:  _pSound->SetFormat(CSoundLibrary::SF_11025_16, TRUE); break;
      case 2:  _pSound->SetFormat(CSoundLibrary::SF_22050_16, TRUE); break;
      case 3:  _pSound->SetFormat(CSoundLibrary::SF_44100_16, TRUE); break;
      default: _pSound->SetFormat(CSoundLibrary::SF_NONE, TRUE);
    }
  }

  RefreshSoundFormat();
  snd_iFormat = _pSound->GetFormat();
};

static void OnWaveVolumeChange(INDEX iCurPos) {
  _pShell->SetFLOAT("snd_fSoundVolume", iCurPos / FLOAT(VOLUME_STEPS));
};

static void WaveSliderChange(void) {
  CAudioOptionsMenu &gmCurrent = _pGUIM->gmAudioOptionsMenu;

  gmCurrent.gm_mgWaveVolume.mg_iCurPos -= 5;
  gmCurrent.gm_mgWaveVolume.ApplyCurrentPosition();
};

static void FrequencyTriggerChange(INDEX iDummy) {
  CAudioOptionsMenu &gmCurrent = _pGUIM->gmAudioOptionsMenu;

  sam_bAutoAdjustAudio = 0;
  gmCurrent.gm_mgAudioAutoTrigger.mg_iSelected = 0;
  gmCurrent.gm_mgAudioAutoTrigger.ApplyCurrentSelection();
};

static void MPEGSliderChange(void) {
  CAudioOptionsMenu &gmCurrent = _pGUIM->gmAudioOptionsMenu;

  gmCurrent.gm_mgMPEGVolume.mg_iCurPos -= 5;
  gmCurrent.gm_mgMPEGVolume.ApplyCurrentPosition();
};

static void OnMPEGVolumeChange(INDEX iCurPos) {
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
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

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
  gm_lhGadgets.AddTail(gm_mgWaveVolume.mg_lnNode);

  gm_mgMPEGVolume.mg_boxOnScreen = BoxMediumRow(4);
  gm_mgMPEGVolume.SetText(TRANS("MUSIC VOLUME"));
  gm_mgMPEGVolume.mg_strTip = TRANS("adjust volume of in-game music");
  gm_mgMPEGVolume.mg_pmgUp = &gm_mgWaveVolume;
  gm_mgMPEGVolume.mg_pmgDown = &gm_mgApply;
  gm_mgMPEGVolume.mg_pOnSliderChange = &OnMPEGVolumeChange;
  gm_mgMPEGVolume.mg_pActivatedFunction = &MPEGSliderChange;
  gm_lhGadgets.AddTail(gm_mgMPEGVolume.mg_lnNode);

  gm_mgApply.mg_bfsFontSize = BFS_LARGE;
  gm_mgApply.mg_boxOnScreen = BoxBigRow(4);
  gm_mgApply.SetText(TRANS("APPLY"));
  gm_mgApply.mg_strTip = TRANS("activate selected options");
  gm_lhGadgets.AddTail(gm_mgApply.mg_lnNode);
  gm_mgApply.mg_pmgUp = &gm_mgMPEGVolume;
  gm_mgApply.mg_pmgDown = &gm_mgAudioAutoTrigger;
  gm_mgApply.mg_pActivatedFunction = &ApplyAudioOptions;
}

void CAudioOptionsMenu::StartMenu(void)
{
  RefreshSoundFormat();
  CGameMenu::StartMenu();
}

// [Cecil] Change to the menu
void CAudioOptionsMenu::ChangeTo(void) {
  ChangeToMenu(&_pGUIM->gmAudioOptionsMenu);
};
