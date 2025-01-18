# Check if required arguments are provided
if ($args.Count -ne 3) {
  Write-Host 'Please provide 3 arguments for the script:
1. Output source file, e.g. "EntityClasses.inl"
2. Project name the entities belong to, e.g. "EntitiesMP"
3. Directory with *_tables.h files, e.g. "Sources/EntitiesMP"'

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
"// This is an automatically generated list of entity classes from the '$projectName' module.
// This file can be included inline to generate functions, arrays or any other structures
// with all classes in the alphabetical order by defining a macro function below.
" | Out-File -FilePath $outputFile

'#ifndef ES_CLASS_INLINE
  #error Please define ES_CLASS_INLINE(class) macro
#endif

// [Cecil] NOTE: If you are getting errors about specific classes, it means that they haven''t been compiled in
// your module. Delete "*_tables.h" files left by those entity sources (or all of them) and rebuild the module.
' | Out-File -FilePath $outputFile -Append

# Array of entity class names
$entityClasses = [System.Collections.ArrayList]::new()

# Go through all entity tables in the specified directory
Get-ChildItem -Path $dirWithSources -Filter *_tables.h | ForEach-Object {
  # Match lines in the file that define the entity class
  $classMatches = Select-String -Path $_.FullName -Pattern "#define ENTITYCLASS " -AllMatches

  foreach ($match in $classMatches) {
    # Extract the class name from the match
    $className = $match.Line.Split(" ", [System.StringSplitOptions]::RemoveEmptyEntries)[2]

    # And add it to the array (only the first definition is relevant)
    $entityClasses.Add("$className") | Out-Null
    break
  }
}

# Sort entity classes alphabetically and print them out
$sortedClasses = $entityClasses | Sort-Object

foreach ($className in $sortedClasses) {
  "ES_CLASS_INLINE($className)" | Out-File -FilePath $outputFile -Append
}

$count = $sortedClasses.Length
Write-Host "Done generating $outputFile ($count entity classes)"
