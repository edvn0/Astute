param (
    [string]$buildType,
    [int]$parallel = -1
)

# List of valid CMake build types
$validBuildTypes = 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo'

# Run the application with validation of build type
if ($buildType -and $validBuildTypes -contains $buildType) {
    Write-Output "Running the application... Mode: $buildType"

    $cmakeArgs = @('--build', 'build', '--config', $buildType)
    if ($parallel -ne -1) {
        $cmakeArgs += '--parallel', $parallel
    }

    cmake @cmakeArgs

    # Check if cmake build was successful
    if ($?) {
        & ".\build\Application\$buildType\App" -s 4096
    }
    else {
        Write-Error "cmake build failed. Application will not be run."
        exit 1
    }
}
else {
    Write-Error "Invalid or missing build type. Valid types are: $($validBuildTypes -join ', ')"
    exit 1
}
