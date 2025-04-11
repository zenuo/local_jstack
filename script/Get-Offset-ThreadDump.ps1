function Get-Offset-ThreadDump{
    param(
    [Parameter(Mandatory=$true)][string]$debugSymbolsPath,
	[Parameter(Mandatory=$true)][string]$jdkVersion
    )

    $jvmDllMap = If ($jdkVersion.StartsWith("21")) {"jvm.dll.map"} Else {"jvm.map"}

    Write-Host $jvmDllMap

    $foundFiles = Get-ChildItem -Path $debugSymbolsPath -Recurse -Filter $jvmDllMap

    if ($null -eq $foundFiles) {
        Write-Host "file not found: $jvmDllMap in $debugSymbolsPath"
        exit 1
    }

    $jvmDllMapPath = $foundFiles[0]
    Write-Host "jvmDllMap found: "$jvmDllMapPath.FullName
	
	# __ImageBase
	
	$imageBaseLine = $(Select-String -SimpleMatch "__ImageBase" -Path $jvmDllMapPath).Line
	
	$imageBase = $(-split $imageBaseLine)[2]

    $threadDumpAddressLine = $(Select-String -SimpleMatch "?thread_dump@@YAJPEAVAttachOperation@@PEAVoutputStream@@@Z" -Path $jvmDllMapPath | Where-Object { $_ -notmatch "unwind" }).Line
	
	$threadDumpAddress = $(-split $threadDumpAddressLine)[2]
	
	$offsetInt = [System.Convert]::ToInt64($threadDumpAddress, 16) - [System.Convert]::ToInt64($imageBase, 16)

    return [System.Convert]::ToString($offsetInt, 16)
}