/* Copyright (c) 2023 Dreamy Cecil
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

#ifndef SE_INCL_GFX_INTERFACE_H
#define SE_INCL_GFX_INTERFACE_H

#include <Engine/Graphics/Gfx_wrapper.h>

class IGfxInterface
{
  public:
    // Info of one point for delayed depth buffer lookup
    struct DepthInfo {
      INDEX di_iID;               // Unique identifier
      PIX   di_pixI, di_pixJ;     // Last requested coordinates
      FLOAT di_fOoK;              // Last requested depth
      INDEX di_iSwapLastRequest;  // Index of swap when last requested
      INDEX di_iMirrorLevel;      // Level of mirror recursion in which the flare is
      BOOL  di_bVisible;          // Whether the point was visible
    };

  protected:
    ULONG _ulCurrentColorMask; // Current color mask

  public:
    // Constructor
    IGfxInterface()
    {
      _ulCurrentColorMask = (CT_RMASK | CT_GMASK | CT_BMASK | CT_AMASK);
    };

    // Destructor
    virtual ~IGfxInterface()
    {
    };

    // Unique graphical API type
    virtual GfxAPIType GetType(void) const = 0;

  // Graphical API methods
  public:

    // Enable operations
    virtual void EnableDepthWrite(void) = 0;
    virtual void EnableDepthBias(void) = 0;
    virtual void EnableDepthTest(void) = 0;
    virtual void EnableAlphaTest(void) = 0;
    virtual void EnableBlend(void) = 0;
    virtual void EnableDither(void) = 0;
    virtual void EnableTexture(void) = 0;
    virtual void EnableClipping(void) = 0;
    virtual void EnableClipPlane(void) = 0;

    // Disable operations
    virtual void DisableDepthWrite(void) = 0;
    virtual void DisableDepthBias(void) = 0;
    virtual void DisableDepthTest(void) = 0;
    virtual void DisableAlphaTest(void) = 0;
    virtual void DisableBlend(void) = 0;
    virtual void DisableDither(void) = 0;
    virtual void DisableTexture(void) = 0;
    virtual void DisableClipping(void) = 0;
    virtual void DisableClipPlane(void) = 0;

    // Set blending operations
    virtual void BlendFunc(GfxBlend eSrc, GfxBlend eDst) = 0;

    // Set depth buffer compare mode
    virtual void DepthFunc(GfxComp eFunc) = 0;

    // Set depth buffer range
    virtual void DepthRange(FLOAT fMin, FLOAT fMax) = 0;

    // Color mask control (use CT_RMASK, CT_GMASK, CT_BMASK, CT_AMASK to enable specific channels)
    virtual void SetColorMask(ULONG ulColorMask) = 0;

    // Read current color mask
    virtual ULONG GetColorMask(void)
    {
      return _ulCurrentColorMask;
    };

  // Projections
  public:

    // Set face culling
    virtual void CullFace(GfxFace eFace) = 0;
    virtual void FrontFace(GfxFace eFace) = 0;

    // Set custom clip plane (if NULL, disable it)
    virtual void ClipPlane(const DOUBLE *pdPlane) = 0;

    // Set orthographic matrix
    virtual void SetOrtho(const FLOAT fLeft, const FLOAT fRight, const FLOAT fTop,  const FLOAT fBottom, const FLOAT fNear, const FLOAT fFar, const BOOL bSubPixelAdjust) = 0;

    // Set frustrum matrix
    virtual void SetFrustum(const FLOAT fLeft, const FLOAT fRight, const FLOAT fTop,  const FLOAT fBottom, const FLOAT fNear, const FLOAT fFar) = 0;

    // Set view matrix
    virtual void SetViewMatrix(const FLOAT *pfMatrix) = 0;

    // Set texture matrix
    virtual void SetTextureMatrix(const FLOAT *pfMatrix) = 0;

    // Polygon mode (point, line or fill)
    virtual void PolygonMode(GfxPolyMode ePolyMode) = 0;

  // Textures
  public:

    // Get current texture filtering mode
    void GetTextureFiltering(INDEX &iFilterType, INDEX &iAnisotropyDegree);

    // Set texture filtering
    void SetTextureFiltering(INDEX &iFilterType, INDEX &iAnisotropyDegree);

    // Set texture LOD biasing
    void SetTextureBiasing(FLOAT &fLODBias);

    // Set texture wrapping mode
    virtual void SetTextureWrapping(enum GfxWrap eWrapU, enum GfxWrap eWrapV) = 0;

    // Set texture modulation mode (1X or 2X)
    virtual void SetTextureModulation(INDEX iScale) = 0;

    // Set texture unit as active
    void SetTextureUnit(INDEX iUnit);

    // Generate texture for API
    virtual void GenerateTexture(ULONG &ulTexObject) = 0;

    // Unbind texture from API
    virtual void DeleteTexture(ULONG &ulTexObject) = 0;

    // Set texture as current
    // - ulTexture = bind number for OGL, or *LPDIRECT3DTEXTURE8 for D3D (pointer to pointer!)
    void SetTexture(ULONG &ulTexObject, CTexParams &tpLocal);

    // Upload texture
    /*
      - ulTexture  = bind number for OGL, or LPDIRECT3DTEXTURE8 for D3D
      - pulTexture = pointer to texture in 32-bit R,G,B,A format (in that byte order)
      - ulFormat   = format in which the texture will be stored in accelerator's (or driver's) memory
      - bNoDiscard = no need to discard old texture (for OGL, this is like "use SubImage")
     */
    void UploadTexture(ULONG *pulTexture, PIX pixWidth, PIX pixHeight, ULONG ulFormat, BOOL bNoDiscard);

    // Returns size of uploaded texture
    SLONG GetTextureSize(ULONG ulTexObject, BOOL bHasMipmaps = TRUE);

    // Returns bytes/pixels ratio for uploaded texture
    INDEX GetTexturePixRatio(ULONG ulTextureObject);

    // Returns bytes/pixels ratio for uploaded texture format
    INDEX GetFormatPixRatio(ULONG ulTextureFormat);

  // Vertex arrays
  public:

    // Prepare vertex array for API
    virtual void SetVertexArray(void *pvtx, INDEX ctVtx) = 0;

  #if SE1_TRUFORM
    // Prepare normal array for API
    virtual void SetNormalArray(GFXNormal *pnor) = 0;
  #endif

    // Prepare UV array for API
    virtual void SetTexCoordArray(GFXTexCoord *ptex, BOOL b4) = 0; // b4 = projective mapping (4 FLOATs)

    // Prepare color array for API
    virtual void SetColorArray(GFXColor *pcol) = 0;

    // Draw prepared arrays
    virtual void DrawElements(INDEX ctElem, INDEX *pidx) = 0;

    // Set constant color for subsequent rendering (until first SetColorArray() call!)
    virtual void SetConstantColor(COLOR col) = 0;

    // Color array usage control
    virtual void EnableColorArray(void) = 0;
    virtual void DisableColorArray(void) = 0;

  // Miscellaneous
  public:

    // Set viewport limits from a drawport
    virtual void SetViewport(const CDrawPort *pdp) = 0;

    // Read depth buffer and update visibility flag of depth points
    virtual void UpdateDepthPointsVisibility(const CDrawPort *pdp, INDEX iMirrorLevel, DepthInfo *pdi, INDEX ctCount) = 0;

    // Force finish of rendering queue
    virtual void Finish(void) = 0;

    // Compiled vertex array control
    virtual void LockArrays(void) = 0;

    // Compiled vertex array control
    void UnlockArrays(void);

    // Reset all arrays
    inline void ResetArrays(void)
    {
      _avtxCommon.PopAll();
      _atexCommon.PopAll();
      _acolCommon.PopAll();
      _aiCommonElements.PopAll();
    };

    // Render elements to screen buffer
    void FlushElements(void);
    void FlushQuads(void);

  #if SE1_TRUFORM
    // Toggle truform
    virtual void EnableTruform(void) = 0;
    virtual void DisableTruform(void) = 0;

    // Set truform parameters
    virtual void SetTruform(const INDEX iLevel, BOOL bLinearNormals);
  #endif

    // Set D3D vertex shader only if it has changed since last time
    void SetVertexShader(DWORD dwHandle);

  // Drawing methods
  public:

    // Draw a point on screen
    virtual void DrawPoint(PIX pixX, PIX pixY, COLOR col, FLOAT fRadius) = 0;

    // Draw a point during 3D rendering
    virtual void DrawPoint3D(FLOAT3D v, COLOR col, FLOAT fRadius) = 0;

    // Draw a line on screen
    virtual void DrawLine(PIX pixX0, PIX pixY0, PIX pixX1, PIX pixY1, COLOR col, FLOAT fD = 0.0f) = 0;

    // Draw a line during 3D rendering
    virtual void DrawLine3D(FLOAT3D v0, FLOAT3D v1, COLOR col) = 0;

    // Draw a square border
    virtual void DrawBorder(FLOAT fX0, FLOAT fY0, FLOAT fX1, FLOAT fY1, COLOR col, FLOAT fD = 0.0f) = 0;

    // Fill a part of the drawport with a given color with blending
    virtual void Fill(PIX pixX0, PIX pixY0, PIX pixX1, PIX pixY1, COLOR col, const CDrawPort *pdpReference) = 0;

    // Fill a part of the drawport with four corner colors with blending
    virtual void Fill(FLOAT fX0, FLOAT fY0, FLOAT fX1, FLOAT fY1, COLOR colUL, COLOR colUR, COLOR colDL, COLOR colDR) = 0;

    // Fill the entire drawport with a given color without blending
    virtual void Fill(COLOR col) = 0;

    // Fill a part of the z-buffer with a given value
    virtual void FillZBuffer(PIX pixX0, PIX pixY0, PIX pixX1, PIX pixY1, FLOAT fZ, const CDrawPort *pdpReference) = 0;

    // Fill the entire z-buffer with a given value
    virtual void FillZBuffer(FLOAT fZ) = 0;

    // Save drawport pixels as an image
    virtual void GrabScreen(class CImageInfo &iiOutput, const CDrawPort *pdpInput, BOOL bGrabDepth) = 0;
};

// Method compatibility
#define gfxEnableDepthWrite     _pGfx->GetInterface()->EnableDepthWrite
#define gfxEnableDepthBias      _pGfx->GetInterface()->EnableDepthBias
#define gfxEnableDepthTest      _pGfx->GetInterface()->EnableDepthTest
#define gfxEnableAlphaTest      _pGfx->GetInterface()->EnableAlphaTest
#define gfxEnableBlend          _pGfx->GetInterface()->EnableBlend
#define gfxEnableDither         _pGfx->GetInterface()->EnableDither
#define gfxEnableTexture        _pGfx->GetInterface()->EnableTexture
#define gfxEnableClipping       _pGfx->GetInterface()->EnableClipping
#define gfxEnableClipPlane      _pGfx->GetInterface()->EnableClipPlane
#define gfxDisableDepthWrite    _pGfx->GetInterface()->DisableDepthWrite
#define gfxDisableDepthBias     _pGfx->GetInterface()->DisableDepthBias
#define gfxDisableDepthTest     _pGfx->GetInterface()->DisableDepthTest
#define gfxDisableAlphaTest     _pGfx->GetInterface()->DisableAlphaTest
#define gfxDisableBlend         _pGfx->GetInterface()->DisableBlend
#define gfxDisableDither        _pGfx->GetInterface()->DisableDither
#define gfxDisableTexture       _pGfx->GetInterface()->DisableTexture
#define gfxDisableClipping      _pGfx->GetInterface()->DisableClipping
#define gfxDisableClipPlane     _pGfx->GetInterface()->DisableClipPlane
#define gfxBlendFunc            _pGfx->GetInterface()->BlendFunc
#define gfxDepthFunc            _pGfx->GetInterface()->DepthFunc
#define gfxDepthRange           _pGfx->GetInterface()->DepthRange
#define gfxSetColorMask         _pGfx->GetInterface()->SetColorMask
#define gfxGetColorMask         _pGfx->GetInterface()->GetColorMask
#define gfxCullFace             _pGfx->GetInterface()->CullFace
#define gfxFrontFace            _pGfx->GetInterface()->FrontFace
#define gfxClipPlane            _pGfx->GetInterface()->ClipPlane
#define gfxSetOrtho             _pGfx->GetInterface()->SetOrtho
#define gfxSetFrustum           _pGfx->GetInterface()->SetFrustum
#define gfxSetViewMatrix        _pGfx->GetInterface()->SetViewMatrix
#define gfxSetTextureMatrix     _pGfx->GetInterface()->SetTextureMatrix
#define gfxPolygonMode          _pGfx->GetInterface()->PolygonMode
#define gfxGetTextureFiltering  _pGfx->GetInterface()->GetTextureFiltering
#define gfxSetTextureFiltering  _pGfx->GetInterface()->SetTextureFiltering
#define gfxSetTextureBiasing    _pGfx->GetInterface()->SetTextureBiasing
#define gfxSetTextureWrapping   _pGfx->GetInterface()->SetTextureWrapping
#define gfxSetTextureModulation _pGfx->GetInterface()->SetTextureModulation
#define gfxSetTextureUnit       _pGfx->GetInterface()->SetTextureUnit
#define gfxGenerateTexture      _pGfx->GetInterface()->GenerateTexture
#define gfxDeleteTexture        _pGfx->GetInterface()->DeleteTexture
#define gfxSetTexture           _pGfx->GetInterface()->SetTexture
#define gfxUploadTexture        _pGfx->GetInterface()->UploadTexture
#define gfxGetTextureSize       _pGfx->GetInterface()->GetTextureSize
#define gfxGetTexturePixRatio   _pGfx->GetInterface()->GetTexturePixRatio
#define gfxGetFormatPixRatio    _pGfx->GetInterface()->GetFormatPixRatio
#define gfxSetVertexArray       _pGfx->GetInterface()->SetVertexArray
#define gfxSetTexCoordArray     _pGfx->GetInterface()->SetTexCoordArray
#define gfxSetColorArray        _pGfx->GetInterface()->SetColorArray
#define gfxDrawElements         _pGfx->GetInterface()->DrawElements
#define gfxSetConstantColor     _pGfx->GetInterface()->SetConstantColor
#define gfxEnableColorArray     _pGfx->GetInterface()->EnableColorArray
#define gfxDisableColorArray    _pGfx->GetInterface()->DisableColorArray
#define gfxFinish               _pGfx->GetInterface()->Finish
#define gfxLockArrays           _pGfx->GetInterface()->LockArrays
#define gfxUnlockArrays         _pGfx->GetInterface()->UnlockArrays
#define gfxResetArrays          _pGfx->GetInterface()->ResetArrays
#define gfxFlushElements        _pGfx->GetInterface()->FlushElements
#define gfxFlushQuads           _pGfx->GetInterface()->FlushQuads

// Truform
#if SE1_TRUFORM
#define gfxEnableTruform        _pGfx->GetInterface()->EnableTruform
#define gfxDisableTruform       _pGfx->GetInterface()->DisableTruform
#define gfxSetTruform           _pGfx->GetInterface()->SetTruform
#define gfxSetNormalArray       _pGfx->GetInterface()->SetNormalArray
#endif

#define d3dSetVertexShader      _pGfx->GetInterface()->SetVertexShader

#endif // include-once check
