param(
[string[]]$jdkVersions,
[string]$jarPath,
+[string]$dllPath,
[bool]$download=$false
)

$jarPath = resolve-path $jarPath
$dllPath = [System.IO.Path]::GetDirectoryName($(Resolve-Path $dllPath))

function Get-Address-ThreadDump{
    param(
    [string]$debugSymbolsPath
    )

    $jvmDllMap = "jvm.dll.map"

    $foundFiles = Get-ChildItem -Path $debugSymbolsPath -Recurse -Filter $jvmDllMap

    if ($null -eq $foundFiles) {
        Write-Host "file not found: $jvmDllMap in $debugSymbolsPath"
        exit 1
    }

    $jvmDllMapPath = $foundFiles[0]
    Write-Host "jvmDllMap found: "$jvmDllMapPath.FullName

    $threadDumpAddressLine = $(Select-String -SimpleMatch "?thread_dump@@YAJPEAVAttachOperation@@PEAVoutputStream@@@Z" -Path $jvmDllMapPath | Where-Object { $_ -notmatch "unwind" }).Line

    return $(-split $threadDumpAddressLine)[2]
}

foreach ($i in $jdkVersions)  
{ 
    $jdkUrl = "https://aka.ms/download-jdk/microsoft-jdk-$i-windows-x64.zip"
    $debugSymbolsUrl = "https://aka.ms/download-jdk/microsoft-jdk-debugsymbols-$i-windows-x64.zip"

    Write-Host "version: $i"

    mkdir -Force $i
    cd $i
    if ($download) {
        Write-Host $jdkUrl
        Invoke-WebRequest $jdkUrl -OutFile $i\jdk.zip
        7z x $i\jdk.zip -ojdk
    }
    
    $foundJavaExes = Get-ChildItem -Path jdk -Recurse -Filter java.exe

    if ($null -eq $foundJavaExes) {
        Write-Host "file not found: java.exe in jdk"
        exit 1
    }
    $javaExe = $foundJavaExes[0]
    Write-Host "java found: "$javaExe.FullName

    if ($download) {
        Write-Host $debugSymbolsUrl
        Invoke-WebRequest $debugSymbolsUrl -OutFile debugsymbols.zip
        7z x debugsymbols.zip -odebugsymbols
    }
    $threadDumpAddress = Get-Address-ThreadDump -debugSymbolsPath debugsymbols
    Write-Host "threadDumpAddress found: $threadDumpAddress"
    Write-Host $javaExe.FullName" -Dlocaljstack.threadDumpOffset=$threadDumpAddress -cp $jarPath app.PrintStack"

    $command = "-Dlocaljstack.threadDumpOffset=$threadDumpAddress -Djava.library.path=$dllPath -cp $jarPath app.PrintStack"

    Write-Host $javaExe.FullName $command
    if ($proc.ExitCode -ne 0) {
        Write-Warning "$i exit with status code $($proc.ExitCode)"
    }
    cd ..
}

