# Check if required arguments are provided
if ($args.Count -ne 3) {
  Write-Host 'Please provide 3 arguments for the script:
1. Output source file, e.g. "ShaderFunctions.inl"
2. Project name the entities belong to, e.g. "Shaders"
3. Directory with *_tables.h files, e.g. "Sources/Shaders"'

  exit 1
}

$outputFile = $args[0]
$projectName = $args[1]
$dirWithSources = $args[2]

# FIXME: There shouldn't be any double quotes in directories to begin with but I can't seem to figure out how to pass
# directories as arguments without double quotes if the path contains spaces, so just strip them manually for now
$dirWithSources = $dirWithSources.Trim('"')

# Create the file with all the directories in-between
New-Item -Path $outputFile -ItemType File -Force | Out-Null

# Add source file header
"// This is an automatically generated list of shader names from the '$projectName' module.
// This file can be included inline to generate functions, arrays or any other structures
// with all shaders in the alphabetical order by defining macro functions below.
" | Out-File -FilePath $outputFile

'#ifndef SHADER_FUNC_INLINE
  #error Please define SHADER_FUNC_INLINE(name) macro
#endif

#ifndef SHADER_DESC_INLINE
  #error Please define SHADER_DESC_INLINE(name) macro
#endif

// [Cecil] NOTE: If you are getting errors about specific shaders, it means that they haven''t been compiled in
// your module. Delete "*.cpp" files with shaders that aren''t being used by the module and then rebuild it.
' | Out-File -FilePath $outputFile -Append

# Array of shader names
$shaderFuncNames = [System.Collections.ArrayList]::new()
$shaderDescNames = [System.Collections.ArrayList]::new()

# Go through all entity tables in the specified directory
Get-ChildItem -Path $dirWithSources -Filter *.cpp | ForEach-Object {
  # Read the file content
  $content = Get-Content -Path $_.FullName

  # Regular expression pattern to capture the content inside parentheses
  # Use "(?:SHADER_MAIN|SHADER_DESC)" to match either
  $funcPattern = 'SHADER_MAIN\s*\(\s*([^,)]+)\s*(?:,|\))'
  $descPattern = 'SHADER_DESC\s*\(\s*([^,)]+)\s*(?:,|\))'

  foreach ($line in $content) {
    if ($line -match $funcPattern) {
      # Extract the render function name from the match
      $shaderName = $matches[1]
      $shaderFuncNames.Add("$shaderName") | Out-Null
    }

    if ($line -match $descPattern) {
      # Extract the description function name from the match
      $shaderName = $matches[1]
      $shaderDescNames.Add("$shaderName") | Out-Null
    }
  }
}

# Sort render function names alphabetically and print them out
$sortedNames = $shaderFuncNames | Sort-Object
$countFunc = $sortedNames.Length

foreach ($shaderName in $sortedNames) {
  "SHADER_FUNC_INLINE($shaderName)" | Out-File -FilePath $outputFile -Append
}

"" | Out-File -FilePath $outputFile -Append

# Sort description function names alphabetically and print them out
$sortedNames = $shaderDescNames | Sort-Object
$countDesc = $sortedNames.Length

foreach ($shaderName in $sortedNames) {
  "SHADER_DESC_INLINE($shaderName)" | Out-File -FilePath $outputFile -Append
}

Write-Host "Done generating $outputFile ($countFunc render funcs; $countDesc desc funcs)"
