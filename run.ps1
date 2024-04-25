# List of valid CMake build types
$validBuildTypes = 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo'

# Run the application with validation of build type
if ($args[0] -and $validBuildTypes -contains $args[0]) {
    Write-Output "Running the application... Mode: $($args[0])"
    cmake --build build --config $args[0]

    # Check if cmake build was successful
    if ($?) {
        & ".\build\Application\$($args[0])\App"
    } else {
        Write-Error "cmake build failed. Application will not be run."
        exit 1
    }
} else {
    Write-Error "Invalid or missing build type. Valid types are: $($validBuildTypes -join ', ')"
    exit 1
}
