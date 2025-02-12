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
#include <Engine/Base/Stream.h>
#include <Engine/Math/Projection.h>
#include <Engine/Ska/Render.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Fog_internal.h>

static INDEX _ctVertices =-1;
static INDEX _ctIndices  =-1;
static INDEX _ctTextures =-1;
static INDEX _ctUVMaps   =-1;
static INDEX _ctColors   =-1;
static INDEX _ctFloats   =-1;
static INDEX _ctLights   =-1;

static CAnyProjection3D *_paprProjection;  // current projection
static Matrix12 *_pmObjToView = NULL;
static Matrix12 *_pmObjToAbs  = NULL;

static CShader     *_pShader     = NULL;   // current shader
static GFXTexCoord *_pCurrentUVMap = NULL; // current UVMap

static GFXVertex4  *_paVertices  = NULL;   // array of vertices
static GFXNormal   *_paNormals   = NULL;   // array of normals
static GFXTexCoord **_paUVMaps    = NULL;   // array of uvmaps to chose from

static GFXTexCoord *_paFogUVMap  = NULL;   // UVMap for fog pass
static GFXTexCoord *_paHazeUVMap = NULL;   // UVMap for haze pass
static GFXColor    *_pacolVtxHaze = NULL;  // array of vertex colors for haze

static CTextureObject **_paTextures = NULL;// array of textures to chose from
static INDEX       *_paIndices   = NULL;   // current array of triangle indices

static GFXColor    _colAmbient = 0x000000FF;     // Ambient color
static COLOR       _colModel   = 0x000000FF;     // Model color
static GFXColor    _colLight   = 0x000000FF;     // Light color
static FLOAT3D     _vLightDir  = FLOAT3D(0,0,0); // Light direction

static COLOR       _colConstant  = 0;     // current set color
static COLOR       *_paColors    = NULL;  // array of colors to chose from
static FLOAT       *_paFloats    = NULL;  // array of floats to chose from
static ULONG       _ulFlags      = 0;

// Vertex colors
static CStaticStackArray<GFXColor> _acolVtxColors;        // array of color values for each vertex
static CStaticStackArray<GFXColor> _acolVtxModifyColors;  // array of color modified values for each vertex
GFXColor *_pcolVtxColors = NULL;    // pointer to vertex color array (points to current array of vertex colors)

// vertex array that is returned if shader request vertices for modify
static CStaticStackArray<GFXVertex4>  _vModifyVertices;
static CStaticStackArray<GFXTexCoord> _uvUVMapForModify;

// [Cecil] Texture wrapping set by the shader
static GfxWrap _aTexWrapping[2] = { GFX_REPEAT, GFX_REPEAT };

// Begin shader using
void shaBegin(CAnyProjection3D &aprProjection,CShader *pShader)
{
  // Chech that last shading ended with shaEnd
  ASSERT(_pShader==NULL);
  // Chech if shader exists
  ASSERT(pShader!=NULL);
  // Set current projection
  _paprProjection = &aprProjection;
  // Set pointer to shader
  _pShader = pShader;
}

// End shader using
void shaEnd(void)
{
  // Chech if shader exists
  ASSERT(_pShader!=NULL);
  // Call shader function
  _pShader->pShaderFunc();
  // Clean used values
  shaClean();

  _pShader = NULL;
}

// Render given model
void shaRender(void)
{
  ASSERT(_ctVertices>0);
  ASSERT(_ctIndices>0);
  ASSERT(_paVertices!=NULL);
  ASSERT(_paIndices!=NULL);

  // Set vertices
  _pGfx->GetInterface()->SetVertexArray(_paVertices,_ctVertices);
  _pGfx->GetInterface()->LockArrays();

  // if there is valid UVMap
  if(_pCurrentUVMap!=NULL) {
    _pGfx->GetInterface()->SetTexCoordArray(_pCurrentUVMap, FALSE);
  }

  // if there is valid vertex color array
  if(_pcolVtxColors!=NULL) {
    _pGfx->GetInterface()->SetColorArray(_pcolVtxColors);
  }

  // draw model with set params
  _pGfx->GetInterface()->DrawElements(_ctIndices, _paIndices);
  _pGfx->GetInterface()->UnlockArrays();
}

// Render aditional pass for fog and haze
void shaDoFogPass(void)
{
  // if full bright 
  if(shaGetFlags()&BASE_FULL_BRIGHT) {
    // no fog pass 
    return;
  }

  ASSERT(_paFogUVMap==NULL);
  ASSERT(_paHazeUVMap==NULL);

  // Calculate fog and haze uvmap for this opaque surface
  RM_DoFogAndHaze(TRUE);
  // if fog uvmap has been given
  if(_paFogUVMap!=NULL) {
    // setup texture/color arrays and rendering mode
    _pGfx->GetInterface()->SetTextureWrapping(GFX_CLAMP, GFX_CLAMP);
    _pGfx->GetInterface()->SetTexture(_fog_ulTexture, _fog_tpLocal);
    _pGfx->GetInterface()->SetTexCoordArray(_paFogUVMap, FALSE);
    _pGfx->GetInterface()->SetConstantColor(_fog_fp.fp_colColor);
    _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    _pGfx->GetInterface()->EnableBlend();
    // render fog pass
    _pGfx->GetInterface()->DrawElements( _ctIndices, _paIndices);
  }
  // if haze uvmap has been given
  if(_paHazeUVMap!=NULL) {
    _pGfx->GetInterface()->SetTextureWrapping(GFX_CLAMP, GFX_CLAMP);
    _pGfx->GetInterface()->SetTexture(_haze_ulTexture, _haze_tpLocal);
    _pGfx->GetInterface()->SetTexCoordArray(_paHazeUVMap, FALSE);
    _pGfx->GetInterface()->BlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    _pGfx->GetInterface()->EnableBlend();
    // set vertex color array for haze
    if(_pacolVtxHaze !=NULL ) {
      _pGfx->GetInterface()->SetColorArray(_pacolVtxHaze);
    }

    // render fog pass
    _pGfx->GetInterface()->DrawElements(_ctIndices, _paIndices);
  }

  // [Cecil] Restore texture wrapping set by the shader
  _pGfx->GetInterface()->SetTextureWrapping(_aTexWrapping[0], _aTexWrapping[1]);
}

// Modify color for fog
void shaModifyColorForFog(void)
{
  // if full bright
  if(shaGetFlags()&BASE_FULL_BRIGHT) {
    // no fog colors
    return;
  }

  // Update this surface color array if fog or haze exists
  RM_DoFogAndHaze(FALSE);
}

// Calculate lightning for given model
void shaCalculateLight(void)
{
  // if full bright
  if(shaGetFlags()&BASE_FULL_BRIGHT) {
    GFXColor colLight = _colConstant;
    GFXColor colAmbient;
    GFXColor colConstant;
    // is over brightning enabled
    if(shaOverBrightningEnabled()) {
      colAmbient = 0x7F7F7FFF;
    } else {
      colAmbient = 0xFFFFFFFF;
    }
    colConstant.MultiplyRGBA(colLight,colAmbient);
    shaSetConstantColor(ByteSwap32(colConstant.abgr));
    // no vertex colors
    return;
  }

  ASSERT(_paNormals!=NULL);
  _acolVtxColors.PopAll();
  _acolVtxColors.Push(_ctVertices);

  GFXColor colModel    = (GFXColor)_colModel;   // Model color
  GFXColor &colAmbient = (GFXColor &)_colAmbient; // Ambient color
  GFXColor &colLight   = (GFXColor &)_colLight;   // Light color
  GFXColor &colSurface = (GFXColor &)_colConstant; // shader color

  colModel.MultiplyRGBA(colModel,colSurface);

  UBYTE ubColShift = 8;
  SLONG slar = colAmbient.r;
  SLONG slag = colAmbient.g;
  SLONG slab = colAmbient.b;

  if(shaOverBrightningEnabled()) {
    slar = ClampUp(slar,127L);
    slag = ClampUp(slag,127L);
    slab = ClampUp(slab,127L);
    ubColShift = 8;
  } else {
    slar*=2;
    slag*=2;
    slab*=2;
    ubColShift = 7;
  }

  // for each vertex color
  for(INDEX ivx=0;ivx<_ctVertices;ivx++) {
    // calculate vertex light
    FLOAT3D vNorm = FLOAT3D(_paNormals[ivx].nx, _paNormals[ivx].ny, _paNormals[ivx].nz);
    FLOAT fDot = vNorm % _vLightDir;
    fDot = Clamp(fDot,0.0f,1.0f);
    SLONG slDot = NormFloatToByte(fDot);

    _acolVtxColors[ivx].r = ClampUp(colModel.r * (slar + ((colLight.r * slDot)>>ubColShift))>>8,255L);
    _acolVtxColors[ivx].g = ClampUp(colModel.g * (slag + ((colLight.g * slDot)>>ubColShift))>>8,255L);
    _acolVtxColors[ivx].b = ClampUp(colModel.b * (slab + ((colLight.b * slDot)>>ubColShift))>>8,255L);
    _acolVtxColors[ivx].a = colModel.a;//slDot;
  }
  // Set current vertex color array 
  _pcolVtxColors = &_acolVtxColors[0];
}

// Calculate lightning for given model
void shaCalculateLightForSpecular(void)
{
  ASSERT(_paNormals!=NULL);
  _acolVtxColors.PopAll();
  _acolVtxColors.Push(_ctVertices);

  GFXColor colModel    = (GFXColor)_colModel;   // Model color
  GFXColor &colAmbient = (GFXColor &)_colAmbient; // Ambient color
  GFXColor &colLight   = (GFXColor &)_colLight;   // Light color
  GFXColor &colSurface = (GFXColor &)_colConstant; // shader color

  // colModel = MulColors(colModel.r,colSurface.abgr);
  colModel.MultiplyRGBA(colModel,colSurface);

  UBYTE ubColShift = 8;
  SLONG slar = colAmbient.r;
  SLONG slag = colAmbient.g;
  SLONG slab = colAmbient.b;

  if(shaOverBrightningEnabled()) {
    slar = ClampUp(slar, (INDEX)127);
    slag = ClampUp(slag, (INDEX)127);
    slab = ClampUp(slab, (INDEX)127);
    ubColShift = 8;
  } else {
    slar*=2;
    slag*=2;
    slab*=2;
    ubColShift = 7;
  }

  // for each vertex color
  for(INDEX ivx=0;ivx<_ctVertices;ivx++) {
    // calculate vertex light
    FLOAT3D vNorm = FLOAT3D(_paNormals[ivx].nx, _paNormals[ivx].ny, _paNormals[ivx].nz);
    FLOAT fDot = vNorm % _vLightDir;
    fDot = Clamp(fDot,0.0f,1.0f);
    SLONG slDot = NormFloatToByte(fDot);

    _acolVtxColors[ivx].r = ClampUp(colModel.r * (slar + ((colLight.r * slDot)>>ubColShift))>>8,255L);
    _acolVtxColors[ivx].g = ClampUp(colModel.g * (slag + ((colLight.g * slDot)>>ubColShift))>>8,255L);
    _acolVtxColors[ivx].b = ClampUp(colModel.b * (slab + ((colLight.b * slDot)>>ubColShift))>>8,255L);
    _acolVtxColors[ivx].a = slDot;//colModel.a;//slDot;
  }
  // Set current wertex array 
  _pcolVtxColors = &_acolVtxColors[0];
}

// Clean all values
void shaClean(void)
{
  _ctVertices   = -1;
  _ctIndices    = -1;
  _ctColors     = -1;
  _ctTextures   = -1;
  _ctUVMaps     = -1;
  _ctLights     = -1;

  _colConstant   = 0;
  _ulFlags       = 0;

  _pShader        = NULL;
  _paVertices     = NULL;
  _paNormals      = NULL;
  _paIndices      = NULL;
  _paUVMaps       = NULL;
  _paTextures     = NULL;
  _paColors       = NULL;
  _paFloats       = NULL;
  _pCurrentUVMap  = NULL;
  _pcolVtxColors  = NULL;

  _paFogUVMap     = NULL;
  _paHazeUVMap    = NULL;
  _pacolVtxHaze   = NULL;
  _pmObjToView    = NULL;
  _pmObjToAbs     = NULL;
  _paprProjection = NULL;

  _acolVtxColors.PopAll();
  _acolVtxModifyColors.PopAll();
  _vModifyVertices.PopAll();
  _uvUVMapForModify.PopAll();
  shaCullFace(GFX_BACK);
}


/*
 * Shader value setting
 */

// Set array of vertices
void shaSetVertexArray(GFXVertex4 *paVertices,INDEX ctVertices)
{
  ASSERT(paVertices!=NULL);
  ASSERT(ctVertices>0);
  // set pointer to new vertex array
  _paVertices = paVertices;
  _ctVertices = ctVertices;
}

// Set array of normals
void shaSetNormalArray(GFXNormal *paNormals)
{
  ASSERT(paNormals!=NULL);

  _paNormals = paNormals;
}

// Set array of indices
void shaSetIndices(INDEX *paIndices,INDEX ctIndices)
{
  ASSERT(paIndices!=NULL);
  ASSERT(ctIndices>0);

  _paIndices = paIndices;
  _ctIndices = ctIndices;
}

// Set array of texture objects for shader
void shaSetTextureArray(CTextureObject **paTextureObject, INDEX ctTextures)
{
  _paTextures = paTextureObject;
  _ctTextures = ctTextures;
}

// Set array of uv maps
void shaSetUVMapsArray(GFXTexCoord **paUVMaps, INDEX ctUVMaps)
{
  ASSERT(paUVMaps!=NULL);
  ASSERT(ctUVMaps>0);

  _paUVMaps = paUVMaps;
  _ctUVMaps = ctUVMaps;
}

// Set array of shader colors
void shaSetColorArray(COLOR *paColors, INDEX ctColors)
{
  ASSERT(paColors!=NULL);
  ASSERT(ctColors>0);

  _paColors = paColors;
  _ctColors = ctColors;
}

// Set array of floats for shader
void shaSetFloatArray(FLOAT *paFloats, INDEX ctFloats)
{
  ASSERT(paFloats!=NULL);
  _paFloats = paFloats;
  _ctFloats = ctFloats;
}

// Set shading flags
void shaSetFlags(ULONG ulFlags)
{
  _ulFlags = ulFlags;
}

// Set base color of model 
void shaSetModelColor(COLOR &colModel)
{
  _colModel = colModel;
}

// Set light direction
void shaSetLightDirection(const FLOAT3D &vLightDir)
{
  _vLightDir = vLightDir;
}

// Set light color
void shaSetLightColor(COLOR colAmbient, COLOR colLight)
{
  _colAmbient = colAmbient;
  _colLight = colLight;
}

// Set object to view matrix
void shaSetObjToViewMatrix(Matrix12 &mat)
{
  _pmObjToView = &mat;
}

// Set object to abs matrix
void shaSetObjToAbsMatrix(Matrix12 &mat)
{
  _pmObjToAbs = &mat;
}



// Set current texture index
void shaSetTexture(INDEX iTextureIndex)
{
  if(_paTextures==NULL || iTextureIndex<0 || iTextureIndex>=_ctTextures ||  _paTextures[iTextureIndex] == NULL) {
    _pGfx->GetInterface()->DisableTexture();
    return;
  }
  ASSERT(iTextureIndex<_ctTextures);

  CTextureObject *pto = _paTextures[iTextureIndex];
  ASSERT(pto!=NULL);

  CTextureData *pTextureData = (CTextureData*)pto->GetData();
  const INDEX iFrameNo = pto->GetFrame();
  pTextureData->SetAsCurrent(iFrameNo);
}

// Set current uvmap index
void shaSetUVMap(INDEX iUVMapIndex)
{
  ASSERT(iUVMapIndex>=0);
  if(iUVMapIndex<=_ctUVMaps) {
    _pCurrentUVMap = _paUVMaps[iUVMapIndex];
  }
}

// Set current color index
void shaSetColor(INDEX icolIndex)
{
  ASSERT(icolIndex>=0);

  if(icolIndex>=_ctColors) {
    _colConstant = C_WHITE|CT_OPAQUE;
  } else {
    _colConstant = _paColors[icolIndex];
  }
  // Set this color as constant color
  _pGfx->GetInterface()->SetConstantColor(_colConstant);
}

// Set array of texcoords index
void shaSetTexCoords(GFXTexCoord *uvNewMap)
{
  _pCurrentUVMap = uvNewMap;
}

// Set array of vertex colors
void shaSetVertexColors(GFXColor *paColors)
{
  _pcolVtxColors = paColors;
}

// Set constant color
void shaSetConstantColor(const COLOR colConstant)
{
  _pGfx->GetInterface()->SetConstantColor(colConstant);
}


/*
 * Shader value getting
 */ 

// Get vertex count
INDEX shaGetVertexCount(void)
{
  return _ctVertices;
}

// Get index count
INDEX shaGetIndexCount(void)
{
  return _ctIndices;
}

// Get float from array of floats
FLOAT shaGetFloat(INDEX iFloatIndex)
{
  ASSERT(iFloatIndex>=0);
  ASSERT(iFloatIndex<_ctFloats);
  return _paFloats[iFloatIndex];
}

// Get texture from array of textures
CTextureObject *shaGetTexture( INDEX iTextureIndex)
{
  ASSERT( iTextureIndex>=0);
  if( _paTextures==NULL || iTextureIndex>=_ctTextures || _paTextures[iTextureIndex]==NULL) return NULL;
  else return _paTextures[iTextureIndex];
}

// Get color from color array
COLOR &shaGetColor(INDEX iColorIndex)
{
  ASSERT(iColorIndex<_ctColors);
  return _paColors[iColorIndex];
}

// Get shading flags
ULONG &shaGetFlags()
{
  return _ulFlags;
}

// Get base color of model
COLOR &shaGetModelColor(void)
{
  return _colModel;
}

// Get light direction
FLOAT3D &shaGetLightDirection(void)
{
  return _vLightDir;
}

// Get current light color
COLOR &shaGetLightColor(void)
{
  return _colLight.abgr;
}

// Get current ambient volor
COLOR &shaGetAmbientColor(void)
{
  return _colAmbient.abgr;
}

// Get current set color
COLOR &shaGetCurrentColor(void)
{
  return _colConstant;
}

// Get vertex array
GFXVertex4 *shaGetVertexArray(void)
{
  return _paVertices;
}

// Get index array
INDEX *shaGetIndexArray(void)
{
  return _paIndices;
}

// Get normal array
GFXNormal *shaGetNormalArray(void)
{
  return _paNormals;
}

// Get uvmap array from array of uvmaps
GFXTexCoord *shaGetUVMap(INDEX iUVMapIndex)
{
  ASSERT( iUVMapIndex>=0);
  if( iUVMapIndex>=_ctUVMaps) return NULL;
  else return _paUVMaps[iUVMapIndex];
}

// Get color array
GFXColor *shaGetColorArray(void)
{
  return &_acolVtxColors[0];
}


// Get empty color array for modifying
GFXColor *shaGetNewColorArray(void)
{
  ASSERT(_ctVertices!=0);
  _acolVtxModifyColors.PopAll();
  _acolVtxModifyColors.Push(_ctVertices);
  return &_acolVtxModifyColors[0];
}

// Get empty texcoords array for modifying
GFXTexCoord *shaGetNewTexCoordArray(void)
{
  ASSERT(_ctVertices!=0);
  _uvUVMapForModify.PopAll();
  _uvUVMapForModify.Push(_ctVertices);
  return &_uvUVMapForModify[0];
}

// Get empty vertex array for modifying
GFXVertex *shaGetNewVertexArray(void)
{
  ASSERT(_ctVertices!=0);
  _vModifyVertices.PopAll();
  _vModifyVertices.Push(_ctVertices);
  return &_vModifyVertices[0];
}

// Get current projection
CAnyProjection3D *shaGetProjection()
{
  return _paprProjection;
}

// Get object to view matrix
Matrix12 *shaGetObjToViewMatrix(void)
{
  ASSERT(_pmObjToView!=NULL);
  return _pmObjToView;
}

// Get object to abs matrix
Matrix12 *shaGetObjToAbsMatrix(void)
{
  ASSERT(_pmObjToAbs!=NULL);
  return _pmObjToAbs;
}



/*
 * Shader states
 */

// Set face culling
void shaCullFace(GfxFace eFace)
{
  if(_paprProjection !=NULL && (*_paprProjection)->pr_bMirror) {
    _pGfx->GetInterface()->FrontFace(GFX_CW);
  } else {
    _pGfx->GetInterface()->FrontFace(GFX_CCW);
  }
  _pGfx->GetInterface()->CullFace(eFace);
}

// Set blending operations
void shaBlendFunc(GfxBlend eSrc, GfxBlend eDst)
{
  _pGfx->GetInterface()->BlendFunc(eSrc, eDst);
}

// Set texture modulation mode
void shaSetTextureModulation(INDEX iScale) 
{
  _pGfx->GetInterface()->SetTextureModulation(iScale);
}

// Enable/Disable blening
void shaEnableBlend(void)
{
  _pGfx->GetInterface()->EnableBlend();
}
void shaDisableBlend(void)
{
  _pGfx->GetInterface()->DisableBlend();
}

// Enable/Disable alpha test
void shaEnableAlphaTest(void)
{
  _pGfx->GetInterface()->EnableAlphaTest();
}
void shaDisableAlphaTest(void)
{
  _pGfx->GetInterface()->DisableAlphaTest();
}

// Enable/Disable depth test
void shaEnableDepthTest(void)
{
  _pGfx->GetInterface()->EnableDepthTest();
}
void shaDisableDepthTest(void)
{
  _pGfx->GetInterface()->DisableDepthTest();
}

// Enable/Disable depth write
void shaEnableDepthWrite(void)
{
  _pGfx->GetInterface()->EnableDepthWrite();
}
void shaDisableDepthWrite(void)
{
  _pGfx->GetInterface()->DisableDepthWrite();
}

// Set depth buffer compare mode
void shaDepthFunc(GfxComp eComp)
{
  _pGfx->GetInterface()->DepthFunc(eComp);
}

// Set texture wrapping 
void shaSetTextureWrapping( enum GfxWrap eWrapU, enum GfxWrap eWrapV)
{
  // [Cecil] Remember shader's texture wrapping
  _aTexWrapping[0] = eWrapU;
  _aTexWrapping[1] = eWrapV;

  _pGfx->GetInterface()->SetTextureWrapping(eWrapU, eWrapV);
}


// Set uvmap for fog
void shaSetFogUVMap(GFXTexCoord *paFogUVMap)
{
  _paFogUVMap = paFogUVMap;
}

// Set uvmap for haze
void shaSetHazeUVMap(GFXTexCoord *paHazeUVMap)
{
  _paHazeUVMap = paHazeUVMap;
}

// Set array of vertex colors used in haze
void shaSetHazeColorArray(GFXColor *paHazeColors)
{
  _pacolVtxHaze = paHazeColors;
}

BOOL shaOverBrightningEnabled(void)
{
  // determine multitexturing capability for overbrighting purposes
  extern INDEX mdl_bAllowOverbright;
  return mdl_bAllowOverbright && _pGfx->gl_ctTextureUnits>1;
}


/*
 * Shader handling
 */

// Constructor
CShader::CShader()
{
  pShaderFunc = NULL;
}

// Destructor
CShader::~CShader()
{
  // Release shader dll
  Clear();
}

// Clear shader 
void CShader::Clear(void)
{
  pShaderFunc = NULL;
  shDesc.Clear(); // [Cecil]
  // release dll
  mdLibrary.Free();
}

// Count used memory
SLONG CShader::GetUsedMemory(void)
{
  return sizeof(CShader);
}

// Write to stream
void CShader::Write_t(CTStream *ostrFile)
{
}

// Read from stream
void CShader::Read_t(CTStream *istrFile)
{
  // read the dll filename and class name from the stream
  CTFileName fnmDLL;
  CTString strShaderFunc;
  CTString strShaderInfo;

  fnmDLL.ReadFromText_t(*istrFile, "Package: ", TRUE);
  strShaderFunc.ReadFromText_t(*istrFile, "Name: ");
  strShaderInfo.ReadFromText_t(*istrFile, "Info: ");

  // create name of dll
  #ifndef NDEBUG
    fnmDLL = _fnmApplicationExe.FileDir() + fnmDLL.FileName() + "D" + fnmDLL.FileExt();
  #else
    fnmDLL = _fnmApplicationExe.FileDir() + fnmDLL.FileName() + fnmDLL.FileExt();
  #endif

  ExpandPath expath;
  expath.ForReading(fnmDLL, DLI_IGNOREGRO);

#if SE1_WIN
  // set new error mode
  const UINT iOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
#endif

  // load dll
  mdLibrary.Load(expath.fnmExpanded.ConstData());

#if SE1_WIN
  // return last error mode
  SetErrorMode(iOldErrorMode);
#endif

  // check if library has loaded
  if (!mdLibrary.IsLoaded())
  {
    // report error
    istrFile->Throw_t("Error loading '%s' library", expath.fnmExpanded.ConstData());
    return;
  }

  ASSERT(_pShaderRenderRegistry != NULL && _pShaderDescRegistry != NULL);

  // [Cecil] Try to find the function in the global registry, assuming it's been added
  ShaderRenderRegistry_t::const_iterator itRender = _pShaderRenderRegistry->find(strShaderFunc);

  // if error accured
  if (itRender == _pShaderRenderRegistry->end()) {
    // report error
    istrFile->Throw_t("GetProcAddress 'ShaderFunc' Error");
  }

  // [Cecil] Try to find the function in the global registry, assuming it's been added
  ShaderDescRegistry_t::const_iterator itDesc = _pShaderDescRegistry->find(strShaderInfo);

  // if error accured
  if (itDesc == _pShaderDescRegistry->end()) {
    // report error
    istrFile->Throw_t("GetProcAddress 'ShaderDesc' Error");
  }

  // [Cecil] Get render function from the registry and read shader description immediately
  pShaderFunc = itRender->second;
  itDesc->second(shDesc);
}

