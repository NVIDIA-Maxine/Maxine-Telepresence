# Model download script for 3DVC
# Downloads models from GitHub
# Usage: download_models.ps1 [OPTIONS]

[CmdletBinding()]
param(
  [Parameter(Mandatory = $false)]
  [Alias("g")]
  [string[]]$gpu,
    
  [Parameter(Mandatory = $false)]
  [Alias("h")]
  [switch]$help
)

# List of supported compute capabilities
$SUPPORTED_GPUS = @("75", "86", "89", "120")

# List of SDKs to get
$SDKS = @("ar", "vfx", "afx")

$SCRIPT_PATH = $PSCommandPath
$SCRIPT_FILENAME = Split-Path $SCRIPT_PATH -Leaf
$SCRIPT_DIRECTORY = Split-Path $SCRIPT_PATH -Parent
$REPO_ROOT = Split-Path $SCRIPT_DIRECTORY -Parent

# ============================================================================
# Helper Functions
# ============================================================================

function Show-Usage {
  Write-Host @"

Usage: .\$SCRIPT_FILENAME [OPTIONS]

Options:
-gpu <gpu1,gpu2,...>  : One or more supported GPU compute capabilities
                        Use 'all' to download models for all supported GPUs
                        Optional - will auto-detect if not specified
-help                 : Show this help message

Examples:
# Download models (auto-detect GPU):
.\$SCRIPT_FILENAME

# Download models with specific GPU:
.\$SCRIPT_FILENAME -gpu 89

# Download models for multiple GPUs:
.\$SCRIPT_FILENAME -gpu 89,120

# Download models for all supported GPUs:
.\$SCRIPT_FILENAME -gpu all

Supported GPU compute capabilities: $($SUPPORTED_GPUS -join ', ')

"@
  exit 0
}


function Get-FileFromGitHub {
  param(
    [string]$file
  )

  # Construct the GitHub release asset URL
  $repo = "NVIDIA-Maxine/Maxine-Telepresence"
  $repo_url = "https://github.com/$repo/releases/latest/download"

  Write-Debug "Downloading $file from GitHub..."
  Write-Debug "URL: $repo_url/$file"
  Write-Debug ""

  try {
    # Suppress progress output
    $ProgressPreference = "SilentlyContinue"

    # Download the asset
    Invoke-WebRequest -Uri "$repo_url/$file" -OutFile $file -ErrorAction Stop
      
    # Get file size after download
    $file_info = Get-Item $file
    $file_size_mb = [math]::Round($file_info.Length / 1MB, 1)
      
    Write-Host "Downloaded: $(Split-Path $file -Leaf) ($file_size_mb MB)" -ForegroundColor Green
    return $true
  }
  catch {
    if ($_.Exception.Response.StatusCode -eq 404) {
      Write-Host "WARNING: File not found (HTTP 404)" -ForegroundColor Yellow
    }
    else {
      Write-Host "ERROR: Download failed: $_" -ForegroundColor Red
    }
    return $false
  }
}

# Function to auto-detect GPU compute capability
function Detect-GPU {
  Write-Host "GPU is not set, trying to auto-detect GPU 0" -ForegroundColor Yellow
  Write-Host "Note: If using a multi-GPU device, use CUDA_VISIBLE_DEVICES to expose the device for which to download models" -ForegroundColor Yellow
  
  # Try compute_capability.exe in features directory first
  $compute_cap_exe = Join-Path $SCRIPT_DIRECTORY "compute_capability.exe"
  if (Test-Path $compute_cap_exe) {
    try {
      $cc = & "$compute_cap_exe" 2>$null
      if ($cc) {
        Write-Host "Auto-detected GPU Compute capability = $cc (using compute_capability.exe)" -ForegroundColor Green
        return $cc
      }
    }
    catch {
      # Unable to run compute_capability.exe
    }
  }
  
  return $null
}

# ============================================================================
# Main Script
# ============================================================================

if ($help) {
  Show-Usage
}

# Process GPU parameter
$gpus_to_process = @()

if (-not $gpu) {
  # Try to auto-detect GPU
  $detected_gpu = Detect-GPU
  if ($detected_gpu) {
    $gpus_to_process = @($detected_gpu)
  }
  else {
    Write-Host "ERROR: Could not auto-detect GPU architecture" -ForegroundColor Red
    Write-Host "       Please specify your GPU using the -gpu flag" -ForegroundColor Yellow
    Write-Host "       Supported GPU compute capabilities: $($SUPPORTED_GPUS -join ', ')" -ForegroundColor Yellow
    Write-Host "       Example: .\download_models.ps1 -gpu 89" -ForegroundColor Yellow
    Write-Host "       Example: .\download_models.ps1 -gpu 89,120" -ForegroundColor Yellow
    Write-Host "       Example: .\download_models.ps1 -gpu all" -ForegroundColor Yellow
    exit 1
  }
}
else {
  # Check if "all" was specified
  if ($gpu.Count -eq 1 -and $gpu[0].ToLower() -eq "all") {
    Write-Host "Downloading models for all supported GPUs" -ForegroundColor Cyan
    $gpus_to_process = $SUPPORTED_GPUS
  }
  else {
    # Process the provided GPU values
    $gpus_to_process = $gpu
  }
}

# Validate all GPU compute capabilities
$invalid_gpus = @()
foreach ($gpu_value in $gpus_to_process) {
  if ($gpu_value -notin $SUPPORTED_GPUS) {
    $invalid_gpus += $gpu_value
  }
}

if ($invalid_gpus.Count -gt 0) {
  Write-Host "ERROR: The following GPU compute capabilities are not supported: $($invalid_gpus -join ', ')" -ForegroundColor Red
  Write-Host "Supported GPU compute capabilities: $($SUPPORTED_GPUS -join ', ')" -ForegroundColor Yellow
  exit 1
}

Write-Host "`n========================================"
Write-Host "Model Download Script"
Write-Host "========================================`n"
Write-Host "Will download models for GPU(s): $($gpus_to_process -join ', ')" -ForegroundColor Cyan

# Process each GPU
foreach ($sm_arch in $gpus_to_process) {
  # Download model zip files for this GPU
  foreach ($sdk in $SDKS) {
    Write-Host "Downloading $($sdk.ToUpper()) models for GPU $sm_arch" -ForegroundColor Cyan
    $filename = "${sdk}sdk-models-$sm_arch.zip"
    if (Test-Path $filename) {
      Write-Host "File $filename already exists, skipping download" -ForegroundColor Yellow
      $result = $true
    }
    else {
      $result = Get-FileFromGitHub $filename
    }
    
    if ($result) {
      $destination_path = "$REPO_ROOT/Internal/nv$sdk/bin/models"
      Write-Host "Extracting models to $destination_path" -ForegroundColor Green
      try {
        Expand-Archive -Force -Path $filename -DestinationPath $destination_path
      }
      catch {
        Write-Host "ERROR: Failed to extract models file $filename" -ForegroundColor Red
        Write-Host $_ -ForegroundColor Red
      }
    }
    else {
      Write-Host "ERROR: Failed to download models file $filename" -ForegroundColor Red
    }
  }
}

# Summary
Write-Host "`n========================================"
Write-Host "Downloads Complete"
Write-Host "========================================`n"
