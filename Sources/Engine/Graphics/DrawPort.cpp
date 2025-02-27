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

#include <Engine/Graphics/DrawPort.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/CRC.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/Projection.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Graphics/Raster.h>
#include <Engine/Graphics/GfxProfile.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/ImageInfo.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Graphics/Font.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

extern INDEX gfx_bDecoratedText;
extern INDEX ogl_iFinish;
extern INDEX d3d_iFinish;


// RECT HANDLING ROUTINES


static BOOL ClipToDrawPort( const CDrawPort *pdp, PIX &pixI, PIX &pixJ, PIX &pixW, PIX &pixH)
{
  if( pixI < 0) { pixW += pixI;  pixI = 0; }
  else if( pixI >= pdp->dp_Width) return FALSE;

  if( pixJ < 0) { pixH += pixJ;  pixJ = 0; }
  else if( pixJ >= pdp->dp_Height) return FALSE;

  if( pixW<1 || pixH<1) return FALSE;

  if( (pixI+pixW) > pdp->dp_Width)  pixW = pdp->dp_Width  - pixI;
  if( (pixJ+pixH) > pdp->dp_Height) pixH = pdp->dp_Height - pixJ;

  ASSERT( pixI>=0 && pixI<pdp->dp_Width);
  ASSERT( pixJ>=0 && pixJ<pdp->dp_Height);
  ASSERT( pixW>0  && pixH>0);
  return TRUE;
}

// DRAWPORT ROUTINES

// set cloned drawport dimensions
void CDrawPort::InitCloned( CDrawPort *pdpBase, DOUBLE rMinI,DOUBLE rMinJ, DOUBLE rSizeI,DOUBLE rSizeJ)
{
  // remember the raster structures
  dp_Raster = pdpBase->dp_Raster;
  // set relative dimensions to make it contain the whole raster
  dp_MinIOverRasterSizeI  = rMinI * pdpBase->dp_SizeIOverRasterSizeI
    +pdpBase->dp_MinIOverRasterSizeI;
  dp_MinJOverRasterSizeJ  = rMinJ * pdpBase->dp_SizeJOverRasterSizeJ
    +pdpBase->dp_MinJOverRasterSizeJ;
  dp_SizeIOverRasterSizeI = rSizeI * pdpBase->dp_SizeIOverRasterSizeI;
  dp_SizeJOverRasterSizeJ = rSizeJ * pdpBase->dp_SizeJOverRasterSizeJ;
	// calculate pixel dimensions from relative dimensions
  RecalculateDimensions();
  // clip scissor to origin drawport
  dp_ScissorMinI = Max( dp_MinI, pdpBase->dp_MinI);
	dp_ScissorMinJ = Max( dp_MinJ, pdpBase->dp_MinJ);
  dp_ScissorMaxI = Min( dp_MaxI, pdpBase->dp_MaxI);
	dp_ScissorMaxJ = Min( dp_MaxJ, pdpBase->dp_MaxJ);
  if( dp_ScissorMinI>dp_ScissorMaxI) dp_ScissorMinI = dp_ScissorMaxI = 0;
  if( dp_ScissorMinJ>dp_ScissorMaxJ) dp_ScissorMinJ = dp_ScissorMaxJ = 0;
	// clone some vars
  dp_FontData = pdpBase->dp_FontData;
  dp_pixTextCharSpacing = pdpBase->dp_pixTextCharSpacing;
  dp_pixTextLineSpacing = pdpBase->dp_pixTextLineSpacing;
  dp_fTextScaling = pdpBase->dp_fTextScaling;
  dp_fTextAspect  = pdpBase->dp_fTextAspect;
  dp_iTextMode    = pdpBase->dp_iTextMode;
  dp_bRenderingOverlay = pdpBase->dp_bRenderingOverlay;

  // [Cecil] Recalculate wide adjustment dynamically based on the aspect ratio (4:3 = 1.0)
  dp_fWideAdjustment = ((FLOAT)dp_Height / (FLOAT)dp_Width) * (4.0f / 3.0f);

  // reset rest of vars
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}

/* Create a drawport for full raster. */
CDrawPort::CDrawPort( CRaster *praBase)
{
  // remember the raster structures
  dp_Raster = praBase;
  dp_fWideAdjustment = 1.0f;
  dp_bRenderingOverlay = FALSE;
  // set relative dimensions to make it contain the whole raster
  dp_MinIOverRasterSizeI  = 0.0;
  dp_MinJOverRasterSizeJ  = 0.0;
  dp_SizeIOverRasterSizeI = 1.0;
  dp_SizeJOverRasterSizeJ = 1.0;
	// clear unknown values
  dp_FontData = NULL;
  dp_pixTextCharSpacing = 1;
  dp_pixTextLineSpacing = 0;
  dp_fTextScaling = 1.0f;
  dp_fTextAspect  = 1.0f;
  dp_iTextMode    = 1;
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}

/* Clone a drawport */
CDrawPort::CDrawPort( CDrawPort *pdpBase,
                      DOUBLE rMinI,DOUBLE rMinJ, DOUBLE rSizeI,DOUBLE rSizeJ)
{
  InitCloned( pdpBase, rMinI,rMinJ, rSizeI,rSizeJ);
}

CDrawPort::CDrawPort( CDrawPort *pdpBase, const PIXaabbox2D &box)
{
  // force dimensions
  dp_MinI   = box.Min()(1) +pdpBase->dp_MinI;
	dp_MinJ   = box.Min()(2) +pdpBase->dp_MinJ;
  dp_Width  = box.Size()(1);
	dp_Height = box.Size()(2);
  dp_MaxI   = dp_MinI+dp_Width  -1;
	dp_MaxJ   = dp_MinJ+dp_Height -1;
  // clip scissor to origin drawport
  dp_ScissorMinI = Max( dp_MinI, pdpBase->dp_MinI);
	dp_ScissorMinJ = Max( dp_MinJ, pdpBase->dp_MinJ);
  dp_ScissorMaxI = Min( dp_MaxI, pdpBase->dp_MaxI);
	dp_ScissorMaxJ = Min( dp_MaxJ, pdpBase->dp_MaxJ);
  if( dp_ScissorMinI>dp_ScissorMaxI) dp_ScissorMinI = dp_ScissorMaxI = 0;
  if( dp_ScissorMinJ>dp_ScissorMaxJ) dp_ScissorMinJ = dp_ScissorMaxJ = 0;
  // remember the raster structures
  dp_Raster = pdpBase->dp_Raster;
  // set relative dimensions to make it contain the whole raster
  dp_MinIOverRasterSizeI  = (DOUBLE)dp_MinI   / dp_Raster->ra_Width;
  dp_MinJOverRasterSizeJ  = (DOUBLE)dp_MinJ   / dp_Raster->ra_Height;
  dp_SizeIOverRasterSizeI = (DOUBLE)dp_Width  / dp_Raster->ra_Width;
  dp_SizeJOverRasterSizeJ = (DOUBLE)dp_Height / dp_Raster->ra_Height;
	// clone some vars
  dp_FontData = pdpBase->dp_FontData;
  dp_pixTextCharSpacing = pdpBase->dp_pixTextCharSpacing;
  dp_pixTextLineSpacing = pdpBase->dp_pixTextLineSpacing;
  dp_fTextScaling = pdpBase->dp_fTextScaling;
  dp_fTextAspect  = pdpBase->dp_fTextAspect;
  dp_iTextMode    = pdpBase->dp_iTextMode;
  dp_bRenderingOverlay = pdpBase->dp_bRenderingOverlay;

  // [Cecil] Recalculate wide adjustment dynamically based on the aspect ratio (4:3 = 1.0)
  dp_fWideAdjustment = ((FLOAT)dp_Height / (FLOAT)dp_Width) * (4.0f / 3.0f);

  // reset rest of vars
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}



// check if a drawport is dualhead
BOOL CDrawPort::IsDualHead(void)
{
  return GetWidth()*3 == GetHeight()*8;
}


// check if a drawport is already wide screen
BOOL CDrawPort::IsWideScreen(void)
{
  return GetWidth()*9 == GetHeight()*16;
}


// returns unique drawports number
ULONG CDrawPort::GetID(void)
{
  ULONG ulCRC;
  CRC_Start(ulCRC);
  CRC_AddLONG(ulCRC, PointerToID(dp_Raster));
  CRC_AddLONG(ulCRC, (ULONG)dp_MinI);
  CRC_AddLONG(ulCRC, (ULONG)dp_MinJ);
  CRC_AddLONG(ulCRC, (ULONG)dp_MaxI);
  CRC_AddLONG(ulCRC, (ULONG)dp_MaxJ);
  CRC_Finish(ulCRC);
  return ulCRC;
}


// dualhead cloning
CDrawPort::CDrawPort( CDrawPort *pdpBase, BOOL bLeft)
{
  // if it is not a dualhead drawport
  if (!pdpBase->IsDualHead()) {
    // always use entire drawport
    InitCloned(pdpBase, 0,0, 1,1);
  // if dualhead is on
  } else {
    // use left or right
    if (bLeft) {
      InitCloned(pdpBase, 0,0, 0.5,1);
    } else {
      InitCloned(pdpBase, 0.5,0, 0.5,1);
    }
  }
}


//  wide-screen cloning
void CDrawPort::MakeWideScreen(CDrawPort *pdp)
{
  // already wide?
  if( IsWideScreen()) {
    pdp->InitCloned( this, 0,0, 1,1);
    return;
  }
  // make wide!
  else {
    // get size
    const PIX pixSizeI = GetWidth();
    const PIX pixSizeJ = GetHeight();
    // make horiz width
    PIX pixSizeJW = pixSizeI*9/16;
    // if already too wide
    if (pixSizeJW>pixSizeJ-10) {
      // no wide screen
      pdp->InitCloned( this, 0,0, 1,1);
      return;
    }
    // clear upper and lower blanks
    const PIX pixJ0 = (pixSizeJ-pixSizeJW)/2;
    if( Lock()) {
      Fill(0, 0, pixSizeI, pixJ0, C_BLACK|CT_OPAQUE);
      Fill(0, pixJ0+pixSizeJW, pixSizeI, pixJ0, C_BLACK|CT_OPAQUE);
      Unlock();
    }
    // init
    pdp->InitCloned( this, 0, FLOAT(pixJ0)/pixSizeJ, 1, FLOAT(pixSizeJW)/pixSizeJ);
  }
}


/*****
 * Recalculate pixel dimensions from relative dimensions and raster size.
 */

void CDrawPort::RecalculateDimensions(void)
{
	const PIX pixRasterSizeI = dp_Raster->ra_Width;
	const PIX pixRasterSizeJ = dp_Raster->ra_Height;
  dp_Width  = (PIX)(dp_SizeIOverRasterSizeI*pixRasterSizeI);
	dp_Height = (PIX)(dp_SizeJOverRasterSizeJ*pixRasterSizeJ);
  dp_ScissorMinI = dp_MinI = (PIX)(dp_MinIOverRasterSizeI*pixRasterSizeI);
	dp_ScissorMinJ = dp_MinJ = (PIX)(dp_MinJOverRasterSizeJ*pixRasterSizeJ);
  dp_ScissorMaxI = dp_MaxI = dp_MinI+dp_Width  -1;
	dp_ScissorMaxJ = dp_MaxJ = dp_MinJ+dp_Height -1;
}


// set orthogonal projection
void CDrawPort::SetOrtho(void) const
{
  // finish all pending render-operations (if required)
  ogl_iFinish = Clamp( ogl_iFinish, 0L, 3L);
  d3d_iFinish = Clamp( d3d_iFinish, 0L, 3L);
  if ((ogl_iFinish == 3 && _pGfx->GetCurrentAPI() == GAT_OGL)
   || (d3d_iFinish == 3 && _pGfx->GetCurrentAPI() == GAT_D3D)) {
    _pGfx->GetInterface()->Finish();
  }

  // prepare ortho dimensions
  const PIX pixMinI  = dp_MinI;
  const PIX pixMinSI = dp_ScissorMinI;
  const PIX pixMaxSI = dp_ScissorMaxI;
  const PIX pixMaxJ  = dp_Raster->ra_Height -1 - dp_MinJ;
  const PIX pixMinSJ = dp_Raster->ra_Height -1 - dp_ScissorMaxJ;
  const PIX pixMaxSJ = dp_Raster->ra_Height -1 - dp_ScissorMinJ;

  // init matrices (D3D needs sub-pixel adjustment)
  _pGfx->GetInterface()->SetOrtho(pixMinSI-pixMinI, pixMaxSI-pixMinI+1, pixMaxJ-pixMaxSJ, pixMaxJ-pixMinSJ+1, 0.0f, -1.0f, TRUE);
  _pGfx->GetInterface()->DepthRange(0, 1);
  _pGfx->GetInterface()->SetViewMatrix(NULL);
  // disable face culling, custom clip plane and truform
  _pGfx->GetInterface()->CullFace(GFX_NONE);
  _pGfx->GetInterface()->DisableClipPlane();

#if SE1_TRUFORM
  _pGfx->GetInterface()->DisableTruform();
#endif
}


// set given projection
void CDrawPort::SetProjection(CAnyProjection3D &apr) const
{
  // finish all pending render-operations (if required)
  ogl_iFinish = Clamp( ogl_iFinish, 0L, 3L);
  d3d_iFinish = Clamp( d3d_iFinish, 0L, 3L);
  if ((ogl_iFinish == 3 && _pGfx->GetCurrentAPI() == GAT_OGL)
   || (d3d_iFinish == 3 && _pGfx->GetCurrentAPI() == GAT_D3D)) {
    _pGfx->GetInterface()->Finish();
  }

  // if isometric projection
  if( apr.IsIsometric()) {
    CIsometricProjection3D &ipr = (CIsometricProjection3D&)*apr;
    const FLOAT2D vMin  = ipr.pr_ScreenBBox.Min()-ipr.pr_ScreenCenter;
    const FLOAT2D vMax  = ipr.pr_ScreenBBox.Max()-ipr.pr_ScreenCenter;
    const FLOAT fFactor = 1.0f / (ipr.ipr_ZoomFactor*ipr.pr_fViewStretch);
    const FLOAT fNear   = ipr.pr_NearClipDistance;
    const FLOAT fLeft   = +vMin(1) *fFactor;
    const FLOAT fRight  = +vMax(1) *fFactor;
    const FLOAT fTop    = -vMin(2) *fFactor;
    const FLOAT fBottom = -vMax(2) *fFactor;
    // if far clip plane is not specified use maximum expected dimension of the world
    FLOAT fFar = ipr.pr_FarClipDistance;
    if( fFar<0) fFar = 1E5f;  // max size 32768, 3D (sqrt(3)), rounded up
    _pGfx->GetInterface()->SetOrtho(fLeft, fRight, fTop, fBottom, fNear, fFar, FALSE);
  }
  // if perspective projection
  else {
    ASSERT( apr.IsPerspective());
    CPerspectiveProjection3D &ppr = (CPerspectiveProjection3D&)*apr;
    const FLOAT fNear   = ppr.pr_NearClipDistance;
    const FLOAT fLeft   = ppr.pr_plClipL(3) / ppr.pr_plClipL(1) *fNear;
    const FLOAT fRight  = ppr.pr_plClipR(3) / ppr.pr_plClipR(1) *fNear;
    const FLOAT fTop    = ppr.pr_plClipU(3) / ppr.pr_plClipU(2) *fNear;
    const FLOAT fBottom = ppr.pr_plClipD(3) / ppr.pr_plClipD(2) *fNear;
    // if far clip plane is not specified use maximum expected dimension of the world
    FLOAT fFar = ppr.pr_FarClipDistance;
    if( fFar<0) fFar = 1E5f;  // max size 32768, 3D (sqrt(3)), rounded up
    _pGfx->GetInterface()->SetFrustum(fLeft, fRight, fTop, fBottom, fNear, fFar);
  }
  
  // set some rendering params
  _pGfx->GetInterface()->DepthRange(apr->pr_fDepthBufferNear, apr->pr_fDepthBufferFar);
  _pGfx->GetInterface()->CullFace(GFX_BACK);
  _pGfx->GetInterface()->SetViewMatrix(NULL);

#if SE1_TRUFORM
  _pGfx->GetInterface()->DisableTruform();
#endif
  
  // if projection is mirrored/warped and mirroring is allowed
  if( apr->pr_bMirror || apr->pr_bWarp) {
    // set custom clip plane 0 to mirror plane
    _pGfx->GetInterface()->EnableClipPlane();
    DOUBLE adViewPlane[4];
    adViewPlane[0] = +apr->pr_plMirrorView(1); 
    adViewPlane[1] = +apr->pr_plMirrorView(2); 
    adViewPlane[2] = +apr->pr_plMirrorView(3); 
    adViewPlane[3] = -apr->pr_plMirrorView.Distance(); 
    _pGfx->GetInterface()->ClipPlane(adViewPlane); // NOTE: view clip plane is multiplied by inverse modelview matrix at time when specified
  }
  // if projection is not mirrored
  else {
    // just disable custom clip plane 0
    _pGfx->GetInterface()->DisableClipPlane();
  }
}



// implementation for some drawport routines that uses raster class

void CDrawPort::Unlock(void)
{
  dp_Raster->Unlock();
  _pGfx->UnlockDrawPort(this);
}


BOOL CDrawPort::Lock(void)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_LOCKDRAWPORT);
  BOOL bRasterLocked = dp_Raster->Lock();
  if( bRasterLocked) {
    // try to lock drawport with driver
    BOOL bDrawportLocked = _pGfx->LockDrawPort(this);
    if( !bDrawportLocked) {
      dp_Raster->Unlock();
      bRasterLocked = FALSE;
    }
  } // done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_LOCKDRAWPORT);
  return bRasterLocked;
}



// DRAWING ROUTINES -------------------------------------



// draw one point
void CDrawPort::DrawPoint( PIX pixI, PIX pixJ, COLOR col, PIX pixRadius/*=1*/) const
{
  ASSERT( pixRadius>=0);
  if( pixRadius==0) return; // do nothing if radius is 0

  // setup rendering mode
  _pGfx->GetInterface()->DisableTexture(); 
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // set point color/alpha and radius
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->DrawPoint(pixI, pixJ, col, (FLOAT)pixRadius);
}


// draw one point in 3D
void CDrawPort::DrawPoint3D( FLOAT3D v, COLOR col, FLOAT fRadius/*=1.0f*/) const
{
  ASSERT( fRadius>=0);
  if( fRadius==0) return; // do nothing if radius is 0

  // setup rendering mode
  _pGfx->GetInterface()->DisableTexture(); 
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // set point color/alpha
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->DrawPoint3D(v, col, fRadius);
}



// draw one line
void CDrawPort::DrawLine( PIX pixI0, PIX pixJ0, PIX pixI1, PIX pixJ1, COLOR col, ULONG typ/*=_FULL*/) const
{
  // setup rendering mode
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  FLOAT fD;
  INDEX iTexFilter, iTexAnisotropy;
  if( typ==_FULL_) {
    // no pattern - just disable texturing
    _pGfx->GetInterface()->DisableTexture(); 
    fD = 0;
  } else {
    // revert to simple point-sample filtering without mipmaps
    INDEX iNewFilter=10, iNewAnisotropy=1;
    _pGfx->GetInterface()->GetTextureFiltering(iTexFilter, iTexAnisotropy);
    _pGfx->GetInterface()->SetTextureFiltering(iNewFilter, iNewAnisotropy);
    // prepare line pattern and mapping
    extern void gfxSetPattern( ULONG ulPattern); 
    gfxSetPattern(typ);
    fD = Max( Abs(pixI0-pixI1), Abs(pixJ0-pixJ1)) /32.0f;
  } 

  // set line color/alpha and go go go 
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->DrawLine(pixI0, pixJ0, pixI1, pixJ1, col, fD);

  // revert to old filtering
  if( typ!=_FULL_) _pGfx->GetInterface()->SetTextureFiltering(iTexFilter, iTexAnisotropy);
}



// draw one line in 3D
void CDrawPort::DrawLine3D( FLOAT3D v0, FLOAT3D v1, COLOR col) const
{
  // setup rendering mode
  _pGfx->GetInterface()->DisableTexture(); 
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // set line color/alpha and go go go 
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->DrawLine3D(v0, v1, col);
}



// draw border
void CDrawPort::DrawBorder( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, COLOR col, ULONG typ/*=_FULL_*/) const
{
  // setup rendering mode
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // for non-full lines, must have 
  FLOAT fD;
  INDEX iTexFilter, iTexAnisotropy;
  if( typ==_FULL_) {
    // no pattern - just disable texturing
    _pGfx->GetInterface()->DisableTexture(); 
    fD = 0;
  } else {
    // revert to simple point-sample filtering without mipmaps
    INDEX iNewFilter=10, iNewAnisotropy=1;
    _pGfx->GetInterface()->GetTextureFiltering(iTexFilter, iTexAnisotropy);
    _pGfx->GetInterface()->SetTextureFiltering(iNewFilter, iNewAnisotropy);
    // prepare line pattern
    extern void gfxSetPattern( ULONG ulPattern); 
    gfxSetPattern(typ);
    fD = Max( pixWidth, pixHeight) /32.0f;
  }

  // set line color/alpha and go go go 
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
  const FLOAT fX0 = pixI+0.5f;
  const FLOAT fY0 = pixJ+0.5f;
  const FLOAT fX1 = pixI-0.5f +pixWidth;
  const FLOAT fY1 = pixJ-0.5f +pixHeight;

  // [Cecil] Abstraction
  _pGfx->GetInterface()->DrawBorder(fX0, fY0, fX1, fY1, col, fD);

  // revert to old filtering
  if( typ!=_FULL_) _pGfx->GetInterface()->SetTextureFiltering(iTexFilter, iTexAnisotropy);
}
 


// fill part of a drawport with a given color
void CDrawPort::Fill( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, COLOR col) const
{
  // if color is tranlucent
  if( ((col&CT_AMASK)>>CT_ASHIFT) != CT_OPAQUE)
  { // draw thru polygon
    Fill( pixI,pixJ, pixWidth,pixHeight, col,col,col,col);
    return;
  }

  // clip and eventually reject
  const BOOL bInside = ClipToDrawPort( this, pixI, pixJ, pixWidth, pixHeight);
  if( !bInside) return;

  // draw thru fast clear for opaque colors
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->Fill(pixI, pixJ, pixI + pixWidth, pixJ + pixHeight, col, this);
}


// fill part of a drawport with a four corner colors
void CDrawPort::Fill( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, 
                      COLOR colUL, COLOR colUR, COLOR colDL, COLOR colDR) const
{
  // clip and eventually reject
  const BOOL bInside = ClipToDrawPort( this, pixI, pixJ, pixWidth, pixHeight);
  if( !bInside) return;

  // setup rendering mode
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->DisableTexture();
  // prepare colors and coords
  colUL = AdjustColor( colUL, _slTexHueShift, _slTexSaturation);
  colUR = AdjustColor( colUR, _slTexHueShift, _slTexSaturation);
  colDL = AdjustColor( colDL, _slTexHueShift, _slTexSaturation);
  colDR = AdjustColor( colDR, _slTexHueShift, _slTexSaturation);
  const FLOAT fX0 = pixI;  const FLOAT fX1 = pixI +pixWidth; 
  const FLOAT fY0 = pixJ;  const FLOAT fY1 = pixJ +pixHeight;

  // [Cecil] Abstraction
  _pGfx->GetInterface()->Fill(fX0, fY0, fX1, fY1, colUL, colUR, colDL, colDR);
}


// fill an entire drawport with a given color
void CDrawPort::Fill( COLOR col) const
{
  // if color is tranlucent
  if( ((col&CT_AMASK)>>CT_ASHIFT) != CT_OPAQUE)
  { // draw thru polygon
    Fill( 0,0, dp_Width,dp_Height, col,col,col,col);
    return;
  }

  // draw thru fast clear for opaque colors
  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->Fill(col);
}


// fill a part of Z-Buffer with a given value
void CDrawPort::FillZBuffer( PIX pixI, PIX pixJ, PIX pixWidth, PIX pixHeight, FLOAT zval) const
{
  // clip and eventually reject
  const BOOL bInside = ClipToDrawPort( this, pixI, pixJ, pixWidth, pixHeight);
  if( !bInside) return;

  _pGfx->GetInterface()->EnableDepthWrite();

  // [Cecil] Abstraction
  _pGfx->GetInterface()->FillZBuffer(pixI, pixJ, pixI + pixWidth, pixJ + pixHeight, zval, this);
}


// fill an entire Z-Buffer with a given value
void CDrawPort::FillZBuffer( FLOAT zval) const
{
  _pGfx->GetInterface()->EnableDepthWrite();

  // [Cecil] Abstraction
  _pGfx->GetInterface()->FillZBuffer(zval);
}


// grab screen
void CDrawPort::GrabScreen( class CImageInfo &iiGrabbedImage, INDEX iGrabZBuffer/*=0*/) const
{
  const GfxAPIType eAPI = _pGfx->GetCurrentAPI();

  extern INDEX ogl_bGrabDepthBuffer;
  const BOOL bGrabDepth = eAPI==GAT_OGL && ((iGrabZBuffer==1 && ogl_bGrabDepthBuffer) || iGrabZBuffer==2);

  // prepare image info's dimensions
  iiGrabbedImage.Clear();
  iiGrabbedImage.ii_Width  = dp_Width;
  iiGrabbedImage.ii_Height = dp_Height;
  iiGrabbedImage.ii_BitsPerPixel = bGrabDepth ? 32 : 24;

  // allocate memory for 24-bit raw picture and copy buffer context
  const PIX pixPicSize = iiGrabbedImage.ii_Width * iiGrabbedImage.ii_Height;
  const SLONG slBytes  = pixPicSize * iiGrabbedImage.ii_BitsPerPixel/8;
  iiGrabbedImage.ii_Picture = (UBYTE*)AllocMemory( slBytes);
  memset( iiGrabbedImage.ii_Picture, 128, slBytes);

  // [Cecil] Abstraction
  _pGfx->GetInterface()->GrabScreen(iiGrabbedImage, this, bGrabDepth);
}


BOOL CDrawPort::IsPointVisible( PIX pixI, PIX pixJ, FLOAT fOoK, INDEX iID, INDEX iMirrorLevel/*=0*/) const
{
  // must have raster!
  if( dp_Raster==NULL) { ASSERT(FALSE);  return FALSE; }

  // if the point is out or at the edge of drawport, it is not visible by default
  if( pixI<1 || pixI>dp_Width-2 || pixJ<1 || pixJ>dp_Height-2) return FALSE;

  // check API
  const GfxAPIType eAPI = _pGfx->GetCurrentAPI();
  _pGfx->CheckAPI();

  // use delayed mechanism for checking
  extern BOOL CheckDepthPoint( const CDrawPort *pdp, PIX pixI, PIX pixJ, FLOAT fOoK, INDEX iID, INDEX iMirrorLevel=0);
  return CheckDepthPoint( this, pixI, pixJ, fOoK, iID, iMirrorLevel);
}


void CDrawPort::RenderLensFlare( CTextureObject *pto, FLOAT fI, FLOAT fJ,
                                 FLOAT fSizeI, FLOAT fSizeJ, ANGLE aRotation, COLOR colLight) const
{
  // check API
  const GfxAPIType eAPI = _pGfx->GetCurrentAPI();
  _pGfx->CheckAPI();

  // setup rendering mode
  _pGfx->GetInterface()->EnableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_ONE, GFX_ONE);
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->ResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // find lens location and dimension
  const FLOAT fRI   = fSizeI*0.5f;
  const FLOAT fRJ   = fSizeJ*0.5f;
  const FLOAT fSinA = SinFast(aRotation);
  const FLOAT fCosA = CosFast(aRotation);
  const FLOAT fRICosA = fRI * +fCosA;
  const FLOAT fRJSinA = fRJ * -fSinA;
  const FLOAT fRISinA = fRI * +fSinA;
  const FLOAT fRJCosA = fRJ * +fCosA;

  // get texture parameters for current frame and needed mip factor and upload texture
  CTextureData *ptd = (CTextureData*)pto->GetData();
  ptd->SetAsCurrent(pto->GetFrame());
  // set lens color
  colLight = AdjustColor( colLight, _slShdHueShift, _slShdSaturation);
  const GFXColor glcol(colLight);

  // prepare coordinates of the rectangle
  pvtx[0].x = fI- fRICosA+fRJSinA;  pvtx[0].y = fJ- fRISinA+fRJCosA;  pvtx[0].z = 0.01f;
  pvtx[1].x = fI- fRICosA-fRJSinA;  pvtx[1].y = fJ- fRISinA-fRJCosA;  pvtx[1].z = 0.01f;
  pvtx[2].x = fI+ fRICosA-fRJSinA;  pvtx[2].y = fJ+ fRISinA-fRJCosA;  pvtx[2].z = 0.01f;
  pvtx[3].x = fI+ fRICosA+fRJSinA;  pvtx[3].y = fJ+ fRISinA+fRJCosA;  pvtx[3].z = 0.01f;
  ptex[0].s = 0;  ptex[0].t = 0;
  ptex[1].s = 0;  ptex[1].t = 1;
  ptex[2].s = 1;  ptex[2].t = 1;
  ptex[3].s = 1;  ptex[3].t = 0;
  pcol[0] = glcol;
  pcol[1] = glcol;
  pcol[2] = glcol;
  pcol[3] = glcol;
  // render it
  _pGfx->gl_ctWorldTriangles += 2; 
  _pGfx->GetInterface()->FlushQuads();
}



/*******************************************************
 * Routines for manipulating drawport's text capabilites
 */


// sets font to be used to printout some text on this drawport 
// WARNING: this resets text spacing, scaling and mode variables
void CDrawPort::SetFont( CFontData *pfd)
{
  // check if we're using font that's not even loaded yet
  ASSERT( pfd!=NULL); 
  dp_FontData = pfd;
  dp_pixTextCharSpacing = pfd->fd_pixCharSpacing; 
  dp_pixTextLineSpacing = pfd->fd_pixLineSpacing;
  dp_fTextScaling = 1.0f;                         
  dp_fTextAspect  = 1.0f;
  dp_iTextMode    = 1;
};


// returns width of the longest line in text string
ULONG CDrawPort::GetTextWidth( const CTString &strText) const
{
  // prepare scaling factors
  PIX   pixCellWidth    = dp_FontData->fd_pixCharWidth;
  SLONG fixTextScalingX = FloatToInt(dp_fTextScaling*dp_fTextAspect*65536.0f);

  // calculate width of entire text line
  PIX pixStringWidth=0, pixOldWidth=0;
  PIX pixCharStart=0, pixCharEnd=pixCellWidth;
  INDEX ctCharsPrinted=0;
  for (INDEX i = 0; i < strText.Length(); i++)
  { // get current letter
    unsigned char chrCurrent = strText[i];
    // next line situation?
    if( chrCurrent == '\n') {
      if( pixOldWidth < pixStringWidth) pixOldWidth = pixStringWidth;
      pixStringWidth=0;
      continue;
    }
    // special char encountered and allowed?
    else if( chrCurrent=='^' && dp_iTextMode!=-1) {
      // get next char
      chrCurrent = strText[++i];
      switch( chrCurrent) {
      // skip corresponding number of characters
      case 'c':  i += FindZero((UBYTE*)&strText[i],6);  continue;
      case 'a':  i += FindZero((UBYTE*)&strText[i],2);  continue;
      case 'f':  i += 1;  continue;
      case 'b':  case 'i':  case 'r':  case 'o':
      case 'C':  case 'A':  case 'F':  case 'B':  case 'I':  i+=0;  continue;
      default:   break; // if we get here this means that ^ or an unrecognized special code was specified
      }
    }
    // ignore tab
    else if( chrCurrent == '\t') continue;

    // add current letter's width to result width
    if( !dp_FontData->fd_bFixedWidth) {
      // proportional font case
      pixCharStart = dp_FontData->fd_fcdFontCharData[chrCurrent].fcd_pixStart;
      pixCharEnd   = dp_FontData->fd_fcdFontCharData[chrCurrent].fcd_pixEnd;
    }
    pixStringWidth += (((pixCharEnd-pixCharStart)*fixTextScalingX)>>16) +dp_pixTextCharSpacing;
    ctCharsPrinted++;
  }
  // determine largest width
  if( pixStringWidth < pixOldWidth) pixStringWidth = pixOldWidth;
  return pixStringWidth;
}


// writes text string on drawport (left aligned if not forced otherwise)
void CDrawPort::PutText( const CTString &strText, PIX pixX0, PIX pixY0, const COLOR colBlend) const
{
  // check API and adjust position for D3D by half pixel
  const GfxAPIType eAPI = _pGfx->GetCurrentAPI();
  _pGfx->CheckAPI();

  // skip drawing if text falls above or below draw port
  if( pixY0>dp_Height || pixX0>dp_Width) return;
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_PUTTEXT);
  char acTmp[7]; // needed for strtoul()
  char *pcDummy; 
  INDEX iRet;

  // cache char and texture dimensions
  FLOAT fTextScalingX   = dp_fTextScaling*dp_fTextAspect;
  SLONG fixTextScalingX = FloatToInt(fTextScalingX  *65536.0f);
  SLONG fixTextScalingY = FloatToInt(dp_fTextScaling*65536.0f);
  PIX pixCellWidth  = dp_FontData->fd_pixCharWidth;
  PIX pixCharHeight = dp_FontData->fd_pixCharHeight-1;
  PIX pixScaledWidth  = (pixCellWidth *fixTextScalingX)>>16;
  PIX pixScaledHeight = (pixCharHeight*fixTextScalingY)>>16;

  // prepare font texture
  _pGfx->GetInterface()->SetTextureWrapping(GFX_REPEAT, GFX_REPEAT);
  CTextureData &td = *dp_FontData->fd_ptdTextureData;
  td.SetAsCurrent();
  // setup rendering mode
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);

  // calculate and apply correction factor
  FLOAT fCorrectionU = 1.0f / td.GetPixWidth();
  FLOAT fCorrectionV = 1.0f / td.GetPixHeight();
  INDEX ctMaxChars = strText.Length();
  // determine text color
  GFXColor glcolDefault( AdjustColor( colBlend, _slTexHueShift, _slTexSaturation));
  GFXColor glcol = glcolDefault;
  ULONG ulAlphaDefault = (colBlend&CT_AMASK)>>CT_ASHIFT;;  // for flasher
  ASSERT( dp_iTextMode==-1 || dp_iTextMode==0 || dp_iTextMode==+1);

  // prepare some text control and output vars
  INDEX ctCharsPrinted=0, ctBoldsPrinted=0;
  BOOL bBold    = FALSE;
  BOOL bItalic  = FALSE;
  INDEX iFlash  = 0;
  ULONG ulAlpha = ulAlphaDefault;
  TIME tmFrame  = _pGfx->gl_tvFrameTime.GetSeconds();
  BOOL bParse = dp_iTextMode==1;

  // prepare arrays
  _pGfx->GetInterface()->ResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push( 2*ctMaxChars*4);  // 2* because of bold
  GFXTexCoord *ptex = _atexCommon.Push( 2*ctMaxChars*4);
  GFXColor    *pcol = _acolCommon.Push( 2*ctMaxChars*4);

  // loop thru chars
  PIX pixAdvancer = ((pixCellWidth*fixTextScalingX)>>16) +dp_pixTextCharSpacing;
  PIX pixStartX = pixX0;
  for( INDEX iChar=0; iChar<ctMaxChars; iChar++)
  {
    // get current char
    unsigned char chrCurrent = strText[iChar];
    // if at end of current line
    if( chrCurrent=='\n') {
      // advance to next line
      pixX0  = pixStartX;
      pixY0 += pixScaledHeight+dp_pixTextLineSpacing;
      if( pixY0>dp_Height) break;
      // skip to next char
      continue;
    }
    // special char encountered and allowed?
    else if( chrCurrent=='^' && dp_iTextMode!=-1) {
      // get next char
      chrCurrent = strText[++iChar];
      COLOR col;
      switch( chrCurrent)
      {
      // color change?
      case 'c':
        strncpy( acTmp, &strText[iChar+1], 6);
        iRet = FindZero( (UBYTE*)&strText[iChar+1], 6);
        iChar+=iRet;
        if( !bParse || iRet<6) continue;
        acTmp[6] = '\0'; // terminate string
        col = strtoul( acTmp, &pcDummy, 16) <<8;
        col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
        glcol.Set( col|glcol.a); // do color change but keep original alpha
        continue;
      // alpha change?
      case 'a':
        strncpy( acTmp, &strText[iChar+1], 2);
        iRet = FindZero( (UBYTE*)&strText[iChar+1], 2);
        iChar+=iRet;
        if( !bParse || iRet<2) continue;
        acTmp[2] = '\0'; // terminate string
        ulAlpha = strtoul( acTmp, &pcDummy, 16);
        continue;
      // flash?
      case 'f':
        chrCurrent = strText[++iChar];
        if( bParse) iFlash = 1+ 2* Clamp( (INDEX)(chrCurrent-'0'), 0L, 9L);
        continue;
      // reset all?
      case 'r':
        bBold   = FALSE;
        bItalic = FALSE;
        iFlash  = 0;
        glcol   = glcolDefault;
        ulAlpha = ulAlphaDefault;
        continue;
      // simple codes ...
      case 'o':  bParse = bParse && gfx_bDecoratedText;  continue;  // allow console override settings?
      case 'b':  if( bParse) bBold   = TRUE;  continue;  // bold?
      case 'i':  if( bParse) bItalic = TRUE;  continue;  // italic?
      case 'C':  glcol   = glcolDefault;      continue;  // color reset?
      case 'A':  ulAlpha = ulAlphaDefault;    continue;  // alpha reset?
      case 'B':  bBold   = FALSE;             continue;  // no bold?
      case 'I':  bItalic = FALSE;             continue;  // italic?
      case 'F':  iFlash  = 0;                 continue;  // no flash?
      default:   break;
      } // unrecognized special code or just plain ^
      if( chrCurrent!='^') { iChar--; break; }
    }
    // ignore tab
    else if( chrCurrent=='\t') continue;

    // get current location and dimensions
    CFontCharData &fcdCurrent = dp_FontData->fd_fcdFontCharData[chrCurrent];
    PIX pixCharX = fcdCurrent.fcd_pixXOffset;
    PIX pixCharY = fcdCurrent.fcd_pixYOffset;
    PIX pixCharStart = fcdCurrent.fcd_pixStart;
    PIX pixCharEnd   = fcdCurrent.fcd_pixEnd;
    PIX pixXA; // adjusted starting X location of printout

    // determine corresponding char width and position adjustments
    if( dp_FontData->fd_bFixedWidth) {
      // for fixed font
      pixXA = pixX0 - ((pixCharStart*fixTextScalingX)>>16)
            + (((pixScaledWidth<<16) - ((pixCharEnd-pixCharStart)*fixTextScalingX) +0x10000) >>17);
    } else {
      // for proportional font
      pixXA = pixX0 - ((pixCharStart*fixTextScalingX)>>16);
      pixAdvancer = (((pixCharEnd-pixCharStart)*fixTextScalingX)>>16) +dp_pixTextCharSpacing;
    }
    // out of screen (left) ?
    if( pixXA>dp_Width || (pixXA+pixCharEnd)<0) {
      // skip to next char
      pixX0 += pixAdvancer;
      continue; 
    }

    // adjust alpha for flashing
    if( iFlash>0) glcol.a = ulAlpha*(sin(iFlash*tmFrame)*0.5f+0.5f);
    else glcol.a = ulAlpha; 

    // prepare coordinates for screen and texture
    const FLOAT fX0 = pixXA;  const FLOAT fX1 = fX0 +pixScaledWidth;
    const FLOAT fY0 = pixY0;  const FLOAT fY1 = fY0 +pixScaledHeight;
    const FLOAT fU0 = pixCharX *fCorrectionU;  const FLOAT fU1 = (pixCharX+pixCellWidth)  *fCorrectionU;
    const FLOAT fV0 = pixCharY *fCorrectionV;  const FLOAT fV1 = (pixCharY+pixCharHeight) *fCorrectionV;
    pvtx[0].x = fX0;  pvtx[0].y = fY0;  pvtx[0].z = 0;
    pvtx[1].x = fX0;  pvtx[1].y = fY1;  pvtx[1].z = 0;
    pvtx[2].x = fX1;  pvtx[2].y = fY1;  pvtx[2].z = 0;
    pvtx[3].x = fX1;  pvtx[3].y = fY0;  pvtx[3].z = 0;
    ptex[0].s = fU0;  ptex[0].t = fV0;
    ptex[1].s = fU0;  ptex[1].t = fV1;
    ptex[2].s = fU1;  ptex[2].t = fV1;
    ptex[3].s = fU1;  ptex[3].t = fV0;
    pcol[0] = glcol;
    pcol[1] = glcol;
    pcol[2] = glcol;
    pcol[3] = glcol;

    // adjust for italic
    if( bItalic) {
      // [Cecil] Fixed larger slant with larger text scaling (fY1-fY0  ->  pixCharHeight)
      const FLOAT fAdjustX = fTextScalingX * FLOAT(pixCharHeight) * 0.2f;  // 20% slanted
      pvtx[0].x += fAdjustX;
      pvtx[3].x += fAdjustX;
    }
    // advance to next vetrices group
    pvtx += 4;
    ptex += 4;
    pcol += 4;
    // add bold char
    if( bBold) {
      const FLOAT fAdjustX = fTextScalingX * ((FLOAT)pixCellWidth)*0.1f;  // 10% fat (extra light mayonnaise:)
      pvtx[0].x = pvtx[0-4].x +fAdjustX;  pvtx[0].y = fY0;  pvtx[0].z = 0;
      pvtx[1].x = pvtx[1-4].x +fAdjustX;  pvtx[1].y = fY1;  pvtx[1].z = 0;
      pvtx[2].x = pvtx[2-4].x +fAdjustX;  pvtx[2].y = fY1;  pvtx[2].z = 0;
      pvtx[3].x = pvtx[3-4].x +fAdjustX;  pvtx[3].y = fY0;  pvtx[3].z = 0;
      ptex[0].s = fU0;    ptex[0].t = fV0;
      ptex[1].s = fU0;    ptex[1].t = fV1;
      ptex[2].s = fU1;    ptex[2].t = fV1;
      ptex[3].s = fU1;    ptex[3].t = fV0;
      pcol[0] = glcol;
      pcol[1] = glcol;
      pcol[2] = glcol;
      pcol[3] = glcol;
      pvtx += 4;
      ptex += 4;
      pcol += 4;
      ctBoldsPrinted++;
    }
    // advance to next char
    pixX0 += pixAdvancer;
    ctCharsPrinted++;
  }

  // adjust vertex arrays size according to chars that really got printed out
  ctCharsPrinted += ctBoldsPrinted;
  _avtxCommon.PopUntil( ctCharsPrinted*4-1);
  _atexCommon.PopUntil( ctCharsPrinted*4-1);
  _acolCommon.PopUntil( ctCharsPrinted*4-1);
  _pGfx->GetInterface()->FlushQuads();

  // all done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_PUTTEXT);
}



// writes text string on drawport (centered arround X)
void CDrawPort::PutTextC( const CTString &strText, PIX pixX0, PIX pixY0,
                          const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutText( strText, pixX0-GetTextWidth(strText)/2, pixY0, colBlend);
}

// writes text string on drawport (centered arround X and Y)
void CDrawPort::PutTextCXY( const CTString &strText, PIX pixX0, PIX pixY0,
                            const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PIX pixTextWidth  = GetTextWidth(strText);
  PIX pixTextHeight = dp_FontData->fd_pixCharHeight * dp_fTextScaling;
  PutText( strText, pixX0-pixTextWidth/2, pixY0-pixTextHeight/2, colBlend);
}

// writes text string on drawport (right-aligned)
void CDrawPort::PutTextR( const CTString &strText, PIX pixX0, PIX pixY0,
                          const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutText( strText, pixX0-GetTextWidth(strText), pixY0, colBlend);
}


/**********************************************************
 * Routines for putting and getting textures strictly in 2D
 */

void CDrawPort::PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                            const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutTexture( pTO, boxScreen, colBlend, colBlend, colBlend, colBlend);
}

void CDrawPort::PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                            const MEXaabbox2D &boxTexture, const COLOR colBlend/*=0xFFFFFFFF*/) const
{
  PutTexture( pTO, boxScreen, boxTexture, colBlend, colBlend, colBlend, colBlend);
}

void CDrawPort::PutTexture( class CTextureObject *pTO, const PIXaabbox2D &boxScreen,
                            const COLOR colUL, const COLOR colUR, const COLOR colDL, const COLOR colDR) const
{
  MEXaabbox2D boxTexture( MEX2D(0,0), MEX2D(pTO->GetWidth(), pTO->GetHeight()));
  PutTexture( pTO, boxScreen, boxTexture, colUL, colUR, colDL, colDR);
}

// complete put texture routine
void CDrawPort::PutTexture( class CTextureObject *pTO,
                            const PIXaabbox2D &boxScreen, const MEXaabbox2D &boxTexture,
                            const COLOR colUL, const COLOR colUR, const COLOR colDL, const COLOR colDR) const
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_PUTTEXTURE);

  // extract screen and texture coordinates
  const PIX pixI0 = boxScreen.Min()(1);  const PIX pixI1 = boxScreen.Max()(1);
  const PIX pixJ0 = boxScreen.Min()(2);  const PIX pixJ1 = boxScreen.Max()(2);

  // if whole texture is out of drawport
  if( pixI0>dp_Width || pixJ0>dp_Height || pixI1<0 || pixJ1<0) {
    // skip it (just to reduce OpenGL call overhead)
    _pfGfxProfile.StopTimer( CGfxProfile::PTI_PUTTEXTURE);
    return;
  }

  // check API and adjust position for D3D by half pixel
  const GfxAPIType eAPI = _pGfx->GetCurrentAPI();
  _pGfx->CheckAPI();

  FLOAT fI0 = pixI0;  FLOAT fI1 = pixI1;
  FLOAT fJ0 = pixJ0;  FLOAT fJ1 = pixJ1;

  // prepare texture
  _pGfx->GetInterface()->SetTextureWrapping(GFX_REPEAT, GFX_REPEAT);
  CTextureData *ptd = (CTextureData*)pTO->GetData();
  ptd->SetAsCurrent(pTO->GetFrame());

  // setup rendering mode
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  _pGfx->GetInterface()->ResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);

  // extract texture coordinates and apply correction factor
  const PIX pixWidth  = ptd->GetPixWidth();
  const PIX pixHeight = ptd->GetPixHeight();
  FLOAT fCorrectionU, fCorrectionV;
  (SLONG&)fCorrectionU = (127-FastLog2(pixWidth))  <<23; // fCorrectionU = 1.0f / ptd->GetPixWidth() 
  (SLONG&)fCorrectionV = (127-FastLog2(pixHeight)) <<23; // fCorrectionV = 1.0f / ptd->GetPixHeight() 
  FLOAT fU0 = (boxTexture.Min()(1)>>ptd->td_iFirstMipLevel) *fCorrectionU;
  FLOAT fU1 = (boxTexture.Max()(1)>>ptd->td_iFirstMipLevel) *fCorrectionU;
  FLOAT fV0 = (boxTexture.Min()(2)>>ptd->td_iFirstMipLevel) *fCorrectionV;
  FLOAT fV1 = (boxTexture.Max()(2)>>ptd->td_iFirstMipLevel) *fCorrectionV;

  // if not tiled
  const BOOL bTiled = Abs(fU0-fU1)>1 || Abs(fV0-fV1)>1;
  if( !bTiled) {
    // slight adjust for sub-pixel precision
    fU0 += +0.25f *fCorrectionU;
    fU1 += -0.25f *fCorrectionU;
    fV0 += +0.25f *fCorrectionV;
    fV1 += -0.25f *fCorrectionV;
  }
  // prepare colors
  const GFXColor glcolUL( AdjustColor( colUL, _slTexHueShift, _slTexSaturation));
  const GFXColor glcolUR( AdjustColor( colUR, _slTexHueShift, _slTexSaturation));
  const GFXColor glcolDL( AdjustColor( colDL, _slTexHueShift, _slTexSaturation));
  const GFXColor glcolDR( AdjustColor( colDR, _slTexHueShift, _slTexSaturation));

  // prepare coordinates of the rectangle
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;
  pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;
  ptex[0].s = fU0;  ptex[0].t = fV0;
  ptex[1].s = fU0;  ptex[1].t = fV1;
  ptex[2].s = fU1;  ptex[2].t = fV1;
  ptex[3].s = fU1;  ptex[3].t = fV0;
  pcol[0] = glcolUL;
  pcol[1] = glcolDL;
  pcol[2] = glcolDR;
  pcol[3] = glcolUR;
  _pGfx->GetInterface()->FlushQuads();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_PUTTEXTURE);
}



// prepares texture and rendering arrays
void CDrawPort::InitTexture( class CTextureObject *pTO, const BOOL bClamp/*=FALSE*/) const
{
  // prepare
  if( pTO!=NULL) {
    // has texture
    CTextureData *ptd = (CTextureData*)pTO->GetData();
    GfxWrap eWrap = GFX_REPEAT;
    if( bClamp) eWrap = GFX_CLAMP;
    _pGfx->GetInterface()->SetTextureWrapping(eWrap, eWrap);
    ptd->SetAsCurrent(pTO->GetFrame());
  } else {
    // no texture
    _pGfx->GetInterface()->DisableTexture();
  }
  // setup rendering mode
  _pGfx->GetInterface()->DisableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  // prepare arrays
  _pGfx->GetInterface()->ResetArrays();
}



// adds one full texture to rendering queue
void CDrawPort::AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1, const COLOR col) const
{
  const GFXColor glCol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  INDEX       *pelm = _aiCommonElements.Push(6);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;
  pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;
  ptex[0].s = 0;    ptex[0].t = 0;
  ptex[1].s = 0;    ptex[1].t = 1;
  ptex[2].s = 1;    ptex[2].t = 1;
  ptex[3].s = 1;    ptex[3].t = 0;
  pcol[0] = glCol;  pcol[1] = glCol;  pcol[2] = glCol;  pcol[3] = glCol;
  pelm[0] = iStart+0;  pelm[1] = iStart+1;  pelm[2] = iStart+2;
  pelm[3] = iStart+2;  pelm[4] = iStart+3;  pelm[5] = iStart+0;
}


// adds one part of texture to rendering queue
void CDrawPort::AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fI1, const FLOAT fJ1, 
                            const FLOAT fU0, const FLOAT fV0, const FLOAT fU1, const FLOAT fV1, const COLOR col) const
{
  const GFXColor glCol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  INDEX       *pelm = _aiCommonElements.Push(6);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI0;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI1;  pvtx[2].y = fJ1;  pvtx[2].z = 0;
  pvtx[3].x = fI1;  pvtx[3].y = fJ0;  pvtx[3].z = 0;
  ptex[0].s = fU0;  ptex[0].t = fV0;
  ptex[1].s = fU0;  ptex[1].t = fV1;
  ptex[2].s = fU1;  ptex[2].t = fV1;
  ptex[3].s = fU1;  ptex[3].t = fV0;
  pcol[0] = glCol;
  pcol[1] = glCol;
  pcol[2] = glCol;
  pcol[3] = glCol;
  pelm[0] = iStart+0;  pelm[1] = iStart+1;  pelm[2] = iStart+2;
  pelm[3] = iStart+2;  pelm[4] = iStart+3;  pelm[5] = iStart+0;
}


// adds one triangle to rendering queue
void CDrawPort::AddTriangle( const FLOAT fI0, const FLOAT fJ0,
                             const FLOAT fI1, const FLOAT fJ1,
                             const FLOAT fI2, const FLOAT fJ2, const COLOR col) const
{
  const GFXColor glCol( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(3);
  GFXTexCoord *ptex = _atexCommon.Push(3);
  GFXColor    *pcol = _acolCommon.Push(3);
  INDEX       *pelm = _aiCommonElements.Push(3);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI1;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI2;  pvtx[2].y = fJ2;  pvtx[2].z = 0;
  pcol[0] = glCol;
  pcol[1] = glCol;
  pcol[2] = glCol;
  pelm[0] = iStart+0;
  pelm[1] = iStart+1;
  pelm[2] = iStart+2;
}


// adds one textured quad (up-left start, counter-clockwise)
void CDrawPort::AddTexture( const FLOAT fI0, const FLOAT fJ0, const FLOAT fU0, const FLOAT fV0, const COLOR col0,
                            const FLOAT fI1, const FLOAT fJ1, const FLOAT fU1, const FLOAT fV1, const COLOR col1,
                            const FLOAT fI2, const FLOAT fJ2, const FLOAT fU2, const FLOAT fV2, const COLOR col2,
                            const FLOAT fI3, const FLOAT fJ3, const FLOAT fU3, const FLOAT fV3, const COLOR col3) const
{
  const GFXColor glCol0( AdjustColor( col0, _slTexHueShift, _slTexSaturation));
  const GFXColor glCol1( AdjustColor( col1, _slTexHueShift, _slTexSaturation));
  const GFXColor glCol2( AdjustColor( col2, _slTexHueShift, _slTexSaturation));
  const GFXColor glCol3( AdjustColor( col3, _slTexHueShift, _slTexSaturation));
  const INDEX iStart = _avtxCommon.Count();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  INDEX       *pelm = _aiCommonElements.Push(6);
  pvtx[0].x = fI0;  pvtx[0].y = fJ0;  pvtx[0].z = 0;
  pvtx[1].x = fI1;  pvtx[1].y = fJ1;  pvtx[1].z = 0;
  pvtx[2].x = fI2;  pvtx[2].y = fJ2;  pvtx[2].z = 0;
  pvtx[3].x = fI3;  pvtx[3].y = fJ3;  pvtx[3].z = 0;
  ptex[0].s = fU0;  ptex[0].t = fV0;
  ptex[1].s = fU1;  ptex[1].t = fV1;
  ptex[2].s = fU2;  ptex[2].t = fV2;
  ptex[3].s = fU3;  ptex[3].t = fV3;
  pcol[0] = glCol0;
  pcol[1] = glCol1;
  pcol[2] = glCol2;
  pcol[3] = glCol3;
  pelm[0] = iStart+0;  pelm[1] = iStart+1;  pelm[2] = iStart+2;
  pelm[3] = iStart+2;  pelm[4] = iStart+3;  pelm[5] = iStart+0;
}


// renders all textures from rendering queue and flushed rendering arrays
void CDrawPort::FlushRenderingQueue(void) const
{ 
  _pGfx->GetInterface()->FlushElements(); 
  _pGfx->GetInterface()->ResetArrays(); 
}



// blends screen with accumulation color
void CDrawPort::BlendScreen(void)
{
  if( dp_ulBlendingA==0) return;

  ULONG fix1oA = 65536 / dp_ulBlendingA;
  ULONG ulRA = (dp_ulBlendingRA*fix1oA)>>16;
  ULONG ulGA = (dp_ulBlendingGA*fix1oA)>>16;
  ULONG ulBA = (dp_ulBlendingBA*fix1oA)>>16;
  ULONG ulA  = ClampUp( dp_ulBlendingA, 255UL);
  COLOR colBlending = RGBAToColor( ulRA, ulGA, ulBA, ulA);
                                    
  // blend drawport (thru z-buffer because of elimination of pixel artefacts)
  _pGfx->GetInterface()->EnableDepthTest();
  _pGfx->GetInterface()->DisableDepthWrite();
  _pGfx->GetInterface()->EnableBlend();
  _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  _pGfx->GetInterface()->DisableAlphaTest();
  _pGfx->GetInterface()->DisableTexture();
  // prepare color
  colBlending = AdjustColor( colBlending, _slTexHueShift, _slTexSaturation);
  GFXColor glcol(colBlending);

  // set arrays
  _pGfx->GetInterface()->ResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  const INDEX iW = dp_Width;
  const INDEX iH = dp_Height;
  pvtx[0].x =  0;  pvtx[0].y =  0;  pvtx[0].z = 0.01f;
  pvtx[1].x =  0;  pvtx[1].y = iH;  pvtx[1].z = 0.01f;
  pvtx[2].x = iW;  pvtx[2].y = iH;  pvtx[2].z = 0.01f;
  pvtx[3].x = iW;  pvtx[3].y =  0;  pvtx[3].z = 0.01f;
  pcol[0] = glcol;
  pcol[1] = glcol;
  pcol[2] = glcol;
  pcol[3] = glcol;
  _pGfx->GetInterface()->FlushQuads();
  // reset accumulation color
  dp_ulBlendingRA = 0;
  dp_ulBlendingGA = 0;
  dp_ulBlendingBA = 0;
  dp_ulBlendingA  = 0;
}

