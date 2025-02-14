%option prefix="SKAyy"
%{
#include "StdAfx.h"
#include "ParsingSymbols.h"
#include "Parser.h"

#include <Engine/Base/CTString.h>
#include <Engine/Base/CTString.inl>

#include <Engine/Templates/DynamicStackArray.cpp>

extern "C" {
  int SKAyywrap(void)
  {
    // no more buffers
    return 1;
  };
}
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
CTFileName include_fnStack[MAX_INCLUDE_DEPTH];
INDEX include_lineStack[MAX_INCLUDE_DEPTH];
FILE *include_FileStack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;

CTFileName strCurentFileName;
CDynamicStackArray<CTString> astrText;

%}

%x COMMENT
%x INCLUDE

DIGIT		[0-9]
HEXDIGIT [0-9A-Fa-f]
DOUBLEQUOTE	\"
STRINGCONTENT	([^\"]|(\\\"))
NONEXP_FLT  ({DIGIT}+"."{DIGIT}*)
EXP_FLT (({DIGIT}+("."({DIGIT}*)?)?)("E"|"e")("+"|"-")?{DIGIT}+)

%%

 // keywords
"#INCLUDE"             BEGIN(INCLUDE);
"SE_MESH"              { return(k_SE_MESH); }
"VERTICES"             { return(k_VERTICES); }
"NORMALS"              { return(k_NORMALS); }
"UVMAPS"               { return(k_UVMAPS); }
"NAME"                 { return(k_NAME); }
"TEXCOORDS"            { return(k_TEXCOORDS); }
"SURFACES"             { return(k_SURFACES); }
"TRIANGLE_SET"         { return(k_TRIANGLE_SET); }
"WEIGHTS"              { return(k_WEIGHTS); }
"WEIGHT_SET"           { return(k_WEIGHT_SET); }
"MORPHS"               { return(k_MORPHS); }
"RELATIVE"             { return(k_RELATIVE); }
"TRUE"                 { return(k_TRUE); }
"FALSE"                { return(k_FALSE); }
"MORPH_SET"            { return(k_MORPH_SET); }
"SE_MESH_END"          { return(k_SE_MESH_END); }
"SE_SKELETON"          { return(k_SE_SKELETON); }
"PARENT"               { return(k_PARENT);}
"BONES"                { return(k_BONES);}
"SE_SKELETON_END"      { return(k_SE_SKELETON_END);}
"SE_ANIM"              { return(k_SE_ANIM);}
"SEC_PER_FRAME"        { return(k_SEC_PER_FRAME);}
"FRAMES"               { return(k_FRAMES);} 
"DEFAULT_POSE"         { return(k_DEFAULT_POSE);}
"SE_ANIM_END"          { return(k_SE_ANIM_END);}
"BONEENVELOPES"        { return(k_BONEENVELOPES);}
"MORPHENVELOPES"       { return(k_MORPHENVELOPES);}
"ANIMSETLIST"          { return(k_ANIM_SET_LIST);}
"ANIM_ID"              { return(k_ANIM_ID);}
"MAX_DISTANCE"         { return(k_MAX_DISTANCE);}
"MESHLODLIST"          { return(k_MESHLODLIST);}
"SKELETONLODLIST"      { return(k_SKELETONLODLIST);}
"TRESHOLD"             { return(k_TRESHOLD);}
"COMPRESION"           { return(k_COMPRESION);}
"LENGTH"               { return(k_LENGTH);}
"ANIMSPEED"            { return(k_ANIMSPEED);}
"SHADER_PARAMS"        { return(k_SHADER_PARAMS);}
"SHADER_PARAMS_END"    { return(k_SHADER_PARAMS_END);}
"SHADER_NAME"          { return(k_SHADER_NAME);}
"SHADER_SURFACES"      { return(k_SHADER_SURFACES);}
"SHADER_SURFACE"       { return(k_SHADER_SURFACE);}
"SHADER_TEXTURES"      { return(k_SHADER_TEXTURES);}
"SHADER_UVMAPS"        { return(k_SHADER_UVMAPS);}
"SHADER_COLORS"        { return(k_SHADER_COLORS);}
"SHADER_FLOATS"        { return(k_SHADER_FLOATS);}
"SHADER_FLAGS"         { return(k_SHADER_FLAGS);}
"FULL_FACE_FORWARD"    { return(k_FULL_FACE_FORWARD);}
"HALF_FACE_FORWARD"    { return(k_HALF_FACE_FORWARD);}



<INCLUDE>[ \t]*"\""      /* eat the whitespace */
<INCLUDE>[^"\""]*"\""   { /* got the include file name */
  if(include_stack_ptr >= MAX_INCLUDE_DEPTH)
  {
    BEGIN(INITIAL);
    ThrowF_t("Includes nested too deeply '%c' in line %d)", yytext[0], _yy_iLine );
  }

  const CTString strFileName = ExpandPath::OnDisk(yytext);

  try
  {
    include_stack[include_stack_ptr] = YY_CURRENT_BUFFER;
    include_fnStack[include_stack_ptr] = strCurentFileName;
    include_lineStack[include_stack_ptr] = _yy_iLine;
    include_FileStack[include_stack_ptr] = yyin;
    yyin = FileSystem::Open(strFileName, "r");
    
    if(yyin == NULL) 
      ThrowF_t("File '%s'\n Could not open '%s' (line %d)", strCurentFileName.ConstData(), strFileName.ConstData(), _yy_iLine);

    include_stack_ptr++;
    yy_switch_to_buffer(yy_create_buffer( yyin, YY_BUF_SIZE ));
    strCurentFileName = strFileName;
    // try to remove app path from source file name
    try { strCurentFileName.RemoveApplicationPath_t(); }
    catch(char *){}
    _yy_iLine = 1;
  }
  catch(char *strError)
  {
    BEGIN(INITIAL);
    ThrowF_t(strError);
  }
  BEGIN(INITIAL);
}
<INCLUDE>.    {  /* something unrecognized inside include statement */
  ThrowF_t("Wrong syntax for include statement");
  BEGIN(INITIAL);
}
<<EOF>> {
  if ( --include_stack_ptr < 0 )
  {
    yyterminate();
    exit(1);
  }
  else
  {
    fclose(yyin);
    SKAyy_delete_buffer( YY_CURRENT_BUFFER );
    SKAyy_switch_to_buffer( include_stack[include_stack_ptr] );
    strCurentFileName = include_fnStack[include_stack_ptr];
    yyin = include_FileStack[include_stack_ptr];
    _yy_iLine = include_lineStack[include_stack_ptr];
  }
}

 /* single character operators and punctuations */
";"|","|"{"|"}" {
  return(yytext[0]);}

 /* constants */
"-"?{DIGIT}+                  { SKAyylval.i = atoi(yytext); return(c_int); }
"0x"{HEXDIGIT}+               { SKAyylval.i = strtoul(yytext+2, NULL, 16); return(c_int);}
"-"?{NONEXP_FLT}("f"|"F")?    { SKAyylval.f = (float) atof(yytext); return(c_float); }
"-"?{EXP_FLT}("f"|"F")?       { SKAyylval.f = (float) atof(yytext); return(c_float); }
"\""{STRINGCONTENT}*"\""  { 
  CTString &strNew = astrText.Push();
  char *pchNew;
  // remove double-quotes
  yytext[strlen(yytext)-1] = 0;
  pchNew = yytext+1;
  //SKAyylval.str = strNew.ConstData();
  strNew = pchNew;
  SKAyylval.str = strNew.ConstData();
  return(c_string); 
}

 /* eat up comments */
"/*"          { BEGIN(COMMENT); }
<COMMENT>"* /" { BEGIN(INITIAL); }
<COMMENT>.    {}
"//"[^\n]*\n { _yy_iLine++; }

 /* eat up whitespace */
[ \t]+	 {
}
 /* eat up linefeeds and count lines in all conditions */
<*>\n	{
  _yy_iLine++;
}

 /* for all unrecognized characters */
. {
  // report an error
  ThrowF_t("File '%s'\n Unrecognized character '%c' (line %d)", strCurentFileName.ConstData(), yytext, _yy_iLine);
}

%%

