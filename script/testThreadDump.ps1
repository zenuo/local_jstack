#.\testThreadDump.ps1 -jdkPath C:\download -jdkVersions 11.0.26, 21.0.3 -jarPath C:\download\localjstack-0.0.1.jar -dllPath C:\download\local_jstack\build\x86_windows_msvc2022_pe_32bit-Debug\src\main\cpp\localjstack.dll

param(
[Parameter(Mandatory=$true)][string[]]$jdkVersions,
[Parameter(Mandatory=$true)][string]$jarPath,
[Parameter(Mandatory=$true)][string]$dllPath,
[switch]$download,
[Parameter(Mandatory=$true)][string]$jdkPath
)

$jarPath = resolve-path $jarPath
$dllPath = [System.IO.Path]::GetDirectoryName($(Resolve-Path $dllPath))

Write-Host $dllPath

. ($PSScriptRoot + "\Get-Offset-ThreadDump.ps1")

cd $jdkPath

foreach ($i in $jdkVersions)  
{ 
    $jdkUrl = "https://aka.ms/download-jdk/microsoft-jdk-$i-windows-x64.zip"
    $debugSymbolsUrl = "https://aka.ms/download-jdk/microsoft-jdk-debugsymbols-$i-windows-x64.zip"

    Write-Host "jdkVersion: $i"

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
    $threadDumpOffset = Get-Offset-ThreadDump -debugSymbolsPath debugsymbols -jdkVersion $i
    Write-Host "threadDumpOffset found: $threadDumpOffset"

    $command = "`"-Dlocaljstack.threadDumpOffset=$threadDumpOffset`" `"-Djava.library.path=$dllPath`" -cp $jarPath app.PrintStack"
    Write-Host $javaExe.FullName $command
	
	$proc = Start-Process -FilePath $javaExe.FullName -ArgumentList $command -PassThru -Wait

    if ($proc.ExitCode -ne 0) {
        Write-Warning "$i exit with status code $($proc.ExitCode)"
		exit $proc.ExitCode
    }
	
	Write-Host "jdkVersion passed: $i"

    cd ..
}

