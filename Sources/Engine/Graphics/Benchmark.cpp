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

#include <Engine/Base/Console.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/Priority.inl>
#include <Engine/Graphics/GfxLibrary.h>

#include <Engine/Graphics/ViewPort.h>
#include <Engine/Graphics/DrawPort.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>


static CViewPort *_pvp = NULL;
static CDrawPort *_pdp = NULL;
static PIX _pixSizeI;
static PIX _pixSizeJ;
static CTimerValue _tv;
static BOOL _bBlend = FALSE;
static BOOL _bVisible = FALSE;
static BOOL _bTexture = FALSE;
static BOOL _bDepth = FALSE;
static BOOL _bMultiTexture = FALSE;
static UBYTE *_pubTexture;
static ULONG _ulTexObject;
static ULONG _ulTexFormat;
static BOOL _bSubImage = FALSE;
static INDEX _ctR = 100;
static INDEX _ctC = 100;
static CTexParams _tpLocal;

static CStaticStackArray<GFXVertex>   _avtx;
static CStaticStackArray<GFXTexCoord> _atex;
static CStaticStackArray<GFXColor>    _acol;
static CStaticStackArray<INDEX> _aiElements;



BOOL _bStarted = FALSE;
static __forceinline void StartTimer(void)
{
  ASSERT(!_bStarted);
  _tv = _pTimer->GetHighPrecisionTimer();
  _bStarted = TRUE;
}


static __forceinline DOUBLE StopTimer(void)
{
  ASSERT(_bStarted);
  _bStarted = FALSE;
  return (_pTimer->GetHighPrecisionTimer()-_tv).GetSeconds();
}


// fill rate benchmark
static DOUBLE FillRatePass(INDEX ct)
{
  if( !_pdp->Lock()) {
    ASSERT(FALSE);
    return 0;
  }

  StartTimer();

  _pdp->Fill(C_GRAY|255);
  _pdp->FillZBuffer(1.0f);

  GFXVertex avtx[4];
  avtx[0].x = 0;          avtx[0].y = 0;          avtx[0].z = 0.5f;  
  avtx[1].x = 0;          avtx[1].y = _pixSizeJ;  avtx[1].z = 0.5f;  
  avtx[2].x = _pixSizeI;  avtx[2].y = _pixSizeJ;  avtx[2].z = 0.5f;  
  avtx[3].x = _pixSizeI;  avtx[3].y = 0;          avtx[3].z = 0.5f;  
  GFXTexCoord atex[4] = { {0,0}, {0,1}, {1,1}, {1,0} };
  GFXColor    acol[4] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFF00FF };
  INDEX       aidx[6] = { 0,1,2, 0,2,3};
  _pGfx->GetInterface()->SetVertexArray(&avtx[0], 4);
  _pGfx->GetInterface()->SetTexCoordArray(&atex[0], FALSE);
  _pGfx->GetInterface()->SetColorArray(&acol[0]);

  if(_bTexture) {
    _pGfx->GetInterface()->EnableTexture();
    if(_bMultiTexture) {
      _pGfx->GetInterface()->SetTextureUnit(1);
      _pGfx->GetInterface()->EnableTexture();
      _pGfx->GetInterface()->SetTexture(_ulTexObject, _tpLocal);
      _pGfx->GetInterface()->SetTexCoordArray(atex, FALSE);
      _pGfx->GetInterface()->SetTextureUnit(0);
    }
  } else {
    _pGfx->GetInterface()->DisableTexture();
  }

  if(_bBlend) {
    _pGfx->GetInterface()->EnableBlend();
    if(_bTexture) {
      _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA); 
    } else {
      _pGfx->GetInterface()->BlendFunc(GFX_ONE, GFX_ONE);
    }
  } else {
    _pGfx->GetInterface()->DisableBlend();
  }

  if(_bDepth) {
    _pGfx->GetInterface()->EnableDepthTest();
    _pGfx->GetInterface()->EnableDepthWrite();
  } else {
    _pGfx->GetInterface()->DisableDepthTest();
    _pGfx->GetInterface()->DisableDepthWrite();
  }
  _pGfx->GetInterface()->DisableAlphaTest();

  for (INDEX i = 0; i < ct; i++) {
    _pGfx->GetInterface()->DrawElements(6, &aidx[0]);
  }

  if(_bMultiTexture) {
    _pGfx->GetInterface()->SetTextureUnit(1);
    _pGfx->GetInterface()->DisableTexture();
    _pGfx->GetInterface()->SetTextureUnit(0);
  }
  _pdp->Unlock();

  _pGfx->GetInterface()->Finish();
  _pvp->SwapBuffers();

  return StopTimer();
}


CTString FillRateString(void)
{
  CTString str;
  str.PrintF( "%s, %s, %s ", 
    _bTexture ? (_bMultiTexture ? "multitexture" : "texture") : "no texture", 
    _bBlend ? "blending"  : "no blending", 
    _bDepth ? "z-buffer:" : "no z-buffer:");
  return str;
}


static DOUBLE FillRate(void)
{
  DOUBLE dDelta = 0;
  for( INDEX i=0; i<10; i++) {
    dDelta += FillRatePass(3) - FillRatePass(2);
  }
  dDelta /= 10.0;
  return (_pixSizeI*_pixSizeJ)/dDelta;
}


static void InitTexture(void)
{
  const SLONG slSize = 256*256 *4 *4/3 +16;
  _pubTexture = (UBYTE*)AllocMemory(slSize);
  for( INDEX i=0; i<256; i++) {
    for( INDEX j=0; j<256; j++) {
      _pubTexture[(j*256+i)*4+0] = i;  
      _pubTexture[(j*256+i)*4+1] = j;  
      _pubTexture[(j*256+i)*4+2] = i+j;
      _pubTexture[(j*256+i)*4+3] = i-j;
    }
  }
  MakeMipmaps( 15, (ULONG*)_pubTexture, 256,256);
  _tpLocal.tp_bSingleMipmap = FALSE;
  _pGfx->GetInterface()->GenerateTexture(_ulTexObject);
  _pGfx->GetInterface()->SetTexture(_ulTexObject, _tpLocal);
}


static void EndTexture(void)
{
  _pGfx->GetInterface()->DeleteTexture(_ulTexObject);
  FreeMemory(_pubTexture);
  _pubTexture = NULL;
}


static void InitTris(void)
{
  INDEX iR, iC;
  INDEX ctVx = _ctR*_ctC;
  _avtx.Push(ctVx);
  _atex.Push(ctVx);
  _acol.Push(ctVx);
  for( iR=0; iR<_ctR; iR++) {
    for( iC=0; iC<_ctC; iC++) {
      INDEX ivx = iR*_ctC+iC;
      _avtx[ivx].x =  FLOAT(iC) / _ctC*4 -2.0f;
      _avtx[ivx].y = -FLOAT(iR) / _ctR*4 +2.0f;
      _avtx[ivx].z = -1.0f;
      _atex[ivx].s = (iC+iR) % 2;
      _atex[ivx].t = (iR)    % 2;
      _acol[ivx].abgr = 0xFFFFFFFF;
    }
  }
  INDEX ctTri = (_ctR-1)*(_ctC-1)*2;
  _aiElements.Push(ctTri*3);
  for( iR=0; iR<_ctR-1; iR++) {
    for( iC=0; iC<_ctC-1; iC++) {
      INDEX iq = iR*(_ctC-1)+iC;
      _aiElements[iq*6+0] = (iR+1) * _ctC + (iC+0);
      _aiElements[iq*6+1] = (iR+1) * _ctC + (iC+1);
      _aiElements[iq*6+2] = (iR+0) * _ctC + (iC+0);
      _aiElements[iq*6+3] = (iR+0) * _ctC + (iC+0);
      _aiElements[iq*6+4] = (iR+1) * _ctC + (iC+1);
      _aiElements[iq*6+5] = (iR+0) * _ctC + (iC+1);
    }
  }
}


static void EndTris(void)
{
  _avtx.Clear();
  _atex.Clear();
  _acol.Clear();
  _aiElements.Clear();
}


static DOUBLE TrisTroughputPass(INDEX ct)
{
  if( !_pdp->Lock()) {
    ASSERT(FALSE);
    return 0;
  }

  StartTimer();

  _pGfx->GetInterface()->SetFrustum(-0.5f, +0.5f, -0.5f, +0.5f, 0.5f, 2.0f);
  _pGfx->GetInterface()->SetViewMatrix(NULL);
  _pGfx->GetInterface()->CullFace(GFX_NONE);

  _pdp->Fill(C_GRAY|255);
  _pdp->FillZBuffer(1.0f);

  if(_bTexture) {
    _pGfx->GetInterface()->EnableTexture();
  } else {
    _pGfx->GetInterface()->DisableTexture();
  }

  if(_bBlend) {
    _pGfx->GetInterface()->EnableBlend();
    _pGfx->GetInterface()->BlendFunc(GFX_ONE, GFX_ONE);
  } else {
    _pGfx->GetInterface()->DisableBlend();
  }

  if(_bDepth) {
    _pGfx->GetInterface()->EnableDepthTest();
    _pGfx->GetInterface()->EnableDepthWrite();
  } else {
    _pGfx->GetInterface()->DisableDepthTest();
    _pGfx->GetInterface()->DisableDepthWrite();
  }
  _pGfx->GetInterface()->DisableAlphaTest();

  _pGfx->GetInterface()->SetVertexArray(&_avtx[0], _avtx.Count());
  _pGfx->GetInterface()->LockArrays();
  _pGfx->GetInterface()->SetTexCoordArray(&_atex[0], FALSE);
  _pGfx->GetInterface()->SetColorArray(&_acol[0]);

  if(_bMultiTexture) {
    _pGfx->GetInterface()->SetTextureUnit(1);
    _pGfx->GetInterface()->EnableTexture();
    _pGfx->GetInterface()->SetTexture(_ulTexObject, _tpLocal);
    _pGfx->GetInterface()->SetTexCoordArray(&_atex[0], FALSE);
    _pGfx->GetInterface()->SetTextureUnit(0);
  }
  for (INDEX i = 0; i < ct; i++) {
    _pGfx->GetInterface()->DrawElements(_aiElements.Count(), &_aiElements[0]);
  }
  _pGfx->GetInterface()->UnlockArrays();

  if(_bMultiTexture) {
    _pGfx->GetInterface()->SetTextureUnit(1);
    _pGfx->GetInterface()->DisableTexture();
    _pGfx->GetInterface()->SetTextureUnit(0);
  }
  _pdp->Unlock();

  _pGfx->GetInterface()->Finish();
  _pvp->SwapBuffers();

  return StopTimer();
}


static DOUBLE TrisTroughput(void)
{
  DOUBLE dDelta = 0;
  for( INDEX i=0; i<10; i++) {
    dDelta += TrisTroughputPass(3) - TrisTroughputPass(2);
  }
  dDelta /= 10.0;
  return ((_ctR-1)*(_ctC-1)*2)/dDelta;
}



static DOUBLE TextureUpload(void)
{
  StartTimer();
  _pGfx->GetInterface()->UploadTexture((ULONG *)_pubTexture, 256, 256, _ulTexFormat, _bSubImage);
  const SLONG slTotal = 256*256*4 *4/3;
  return slTotal/StopTimer();
}



static DOUBLE _dX;
static DOUBLE _dD;
static void RunTest(DOUBLE (*pTest)(void), INDEX ct)
{
#if SE1_WIN
  CSetPriority sp(REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL);
#endif

  DOUBLE dSum  = 0;
  DOUBLE dSum2 = 0;
  for(INDEX i=0; i<(ct+5); i++) {
    DOUBLE d = pTest();
    // must ignore 1st couple of passes due to API queue
    if( i>4) dSum  += d;
    if( i>4) dSum2 += d*d;
  }
  _dX = dSum/ct;
  _dD = Sqrt((dSum2-2*dSum*_dX+ct*_dX*_dX)/(ct-1));
}


/* Benchmark current driver. */
void CGfxLibrary::Benchmark(CViewPort *pvp, CDrawPort *pdp)
{
  // remember drawport/viewport
  _pdp = pdp;
  _pvp = pvp;
  _pixSizeI = pdp->GetWidth();
  _pixSizeJ = pdp->GetHeight();

  // [Cecil] API name
  CTString strAPI = _pGfx->GetApiName(_pGfx->GetCurrentAPI());

  CPrintF("=====================================\n");
  CPrintF("%s performance testing ...\n", strAPI.ConstData());

  InitTexture();
  InitTris();

  CPrintF("\n--- Texture upload\n");

  _ulTexFormat = TS.ts_tfRGBA8;
  _bSubImage = FALSE;
  RunTest(TextureUpload, 10);
  CPrintF("RGBA8 full: %6.02f +- %5.02f Mtex/s;" , _dX/1000/1000, _dD/1000/1000);
  _bSubImage = TRUE;
  RunTest(TextureUpload, 10);
  CPrintF(    "   sub: %6.02f +- %5.02f Mtex/s\n", _dX/1000/1000, _dD/1000/1000);

  _ulTexFormat = TS.ts_tfRGB8;
  _bSubImage = FALSE;
  RunTest(TextureUpload, 10);
  CPrintF("RGB8  full: %6.02f +- %5.02f Mtex/s;" , _dX/1000/1000, _dD/1000/1000);
  _bSubImage = TRUE;
  RunTest(TextureUpload, 10);
  CPrintF(    "   sub: %6.02f +- %5.02f Mtex/s\n", _dX/1000/1000, _dD/1000/1000);

  _ulTexFormat = TS.ts_tfRGBA4;
  _bSubImage = FALSE;
  RunTest(TextureUpload, 10);
  CPrintF("RGBA4 full: %6.02f +- %5.02f Mtex/s;" , _dX/1000/1000, _dD/1000/1000);
  _bSubImage = TRUE;
  RunTest(TextureUpload, 10);
  CPrintF(    "   sub: %6.02f +- %5.02f Mtex/s\n", _dX/1000/1000, _dD/1000/1000);

  _ulTexFormat = TS.ts_tfRGB5;
  _bSubImage = FALSE;
  RunTest(TextureUpload, 10);
  CPrintF("RGB5  full: %6.02f +- %5.02f Mtex/s;" , _dX/1000/1000, _dD/1000/1000);
  _bSubImage = TRUE;
  RunTest(TextureUpload, 10);
  CPrintF(    "   sub: %6.02f +- %5.02f Mtex/s\n", _dX/1000/1000, _dD/1000/1000);

  CPrintF("\n--- Fill rate\n");
  _bMultiTexture = 0;
  _bBlend = 0; _bDepth = 0; _bTexture = 0;
  RunTest(FillRate, 10);
  CPrintF("%-38s %6.02f +- %5.02f Mpix/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);
  _bBlend = 0; _bDepth = 0; _bTexture = 1;
  RunTest(FillRate, 10);
  CPrintF("%-38s %6.02f +- %5.02f Mpix/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);
  _bBlend = 0; _bDepth = 1; _bTexture = 1;
  RunTest(FillRate, 10);
  CPrintF("%-38s %6.02f +- %5.02f Mpix/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);
  _bBlend = 1; _bDepth = 1; _bTexture = 1;
  RunTest(FillRate, 10);
  CPrintF("%-38s %6.02f +- %5.02f Mpix/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);

  if( _pGfx->gl_ctTextureUnits>1) {
    _bMultiTexture = 1;
    RunTest(FillRate, 10);
    CPrintF("%-38s %6.02f +- %5.02f Mpix/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);
  }

  CPrintF("\n--- Geometry speed (%dpix tris)\n", (_pixSizeI/_ctR)*(_pixSizeI/_ctC)/2);
  _bMultiTexture = 0;
  _bBlend = 0; _bDepth = 1; _bTexture = 1;
  RunTest(TrisTroughput, 10);
  CPrintF("%-34s %6.02f +- %5.02f Mtri/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);
  _bBlend = 1; _bDepth = 1; _bTexture = 1;
  RunTest(TrisTroughput, 10);
  CPrintF("%-34s %6.02f +- %5.02f Mtri/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);

  if( _pGfx->gl_ctTextureUnits>1) {
    _bMultiTexture = 1;
    RunTest(TrisTroughput, 10);
    CPrintF("%-34s %6.02f +- %5.02f Mtri/s\n", FillRateString().ConstData(), _dX/1000/1000, _dD/1000/1000);
  }

  EndTris();
  EndTexture();
}
