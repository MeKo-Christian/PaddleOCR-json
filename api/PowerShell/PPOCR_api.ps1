# PowerShell API for calling PaddleOCR-json.exe
# Project homepage:
# https://github.com/hiroi-sora/PaddleOCR-json

###########################################################################
function asc($param){
## Function to convert Chinese to \uxxxx
    $rtn = ''
    $list = $param -split ''
    foreach ($char in $list){
        if($char -ne ''){
            if([int]([char]$char) -gt 32 -and [int]([char]$char) -lt 127){
                $rtn  += $char
            }
            else{
                $rtn  += ("\u" + ("{0:x}" -f [int]([char]$char)))
            }
        }
    }
    return $rtn
}
###########################################################################
class PPOCR {
    [System.Object]$process # Subprocess object
    [int] $runFlag = 0 # Run flag. 0 initializing, 1 running normally
    [System.Diagnostics.Process] $stdSender # Save subprocess stdout pipe
    [int] $__ENABLE_CLIPBOARD = 0 # Whether clipboard is enabled. 0 disabled, 1 enabled
    [string] $imgJson # Cache image recognition result json string
    [string] $processID # OCR subprocess id, used to form identifier
    [string] $eventInit # OCR initialization completion event identifier, defined at startup
    [string] $eventJson # One get json completion event identifier, redefined when passing image

    PPOCR( [string]$exePath,[string]$arg ) {
        # Initialize the recognizer.
        # :exePath: Path to the recognizer `PaddleOCR_json.exe`.
        $WorkingDirectory = Split-Path -Path $exePath # Working directory is parent directory
        # Initialize process information, redirect input and output
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $exePath
        $psi.WorkingDirectory = $WorkingDirectory
        $psi.Arguments = $arg
        $psi.RedirectStandardInput = $true
        $psi.RedirectStandardOutput = $true
        $psi.CreateNoWindow = $true
        $psi.UseShellExecute = $false
        # Initialize process object
        $this.process = New-Object System.Diagnostics.Process
        $this.process.StartInfo = $psi
        # Add OutputDataReceived event subscription to process object (to get process output content). Pass $this to action scope via -MessageData.
        Register-ObjectEvent -InputObject $this.process -EventName OutputDataReceived -action $this.getStdout -MessageData $this
        # Start process
        $this.process.Start()
        $this.process.BeginOutputReadLine()
        # Wait for initialization completion event
        $this.processID = $this.process.Id # Record process ID
        $nowTime = Get-Date -Format "HHmmssffff"
        $this.eventInit = "OCRinit" + $this.processID + $nowTime # Generate event identifier
        # Write-Host "Initializing OCR, event identifier is "($this.eventInit)
        Wait-Event -SourceIdentifier $this.eventInit # Block, wait for initialization completion event
        Write-Host "OCR initialization successful, process ID is "($this.process.Id)
    }

    # Action to receive stdout data
    [ScriptBlock] $getStdout = {
        $this_ = $Event.MessageData # Get the owning object
        $this_.stdSender = $Event.Sender # Send input interface to calling object
        $getData = $Event.SourceEventArgs.Data
        switch ( $this_.runFlag ) {
            # Initializing, waiting for completion flag
            0 {
                if ( $getData.contains("OCR init completed.") ) {
                    $this_.runFlag = 1
                    New-Event -SourceIdentifier $this_.eventInit # Send initialization completion event
                }
                elseif ( $getData.contains("OCR clipboard enbaled.") ) {
                    $this_.__ENABLE_CLIPBOARD = 1 # Detected clipboard is enabled
                }
                break
            }
            # Running normally
            1 {
                $this_.imgJson = $getData
                New-Event -SourceIdentifier $this_.eventJson # Send get json event
                break
            }
        }
    }

    [PSCustomObject] isClipboardEnabled() {
        return $this.__ENABLE_CLIPBOARD;
    }

    [PSCustomObject] runDict( [string]$writeDict ) {
        if ($this.stdSender) {
            $nowTime = Get-Date -Format "HHmmssffff"
            $this.eventJson = "OCRjson" + $this.processID + $nowTime # Update event identifier
            $this.stdSender.StandardInput.WriteLine($writeDict); # Write to pipe
            Wait-Event -SourceIdentifier $this.eventJson # Block, wait for get json
            try {
                $getdict = $this.imgJson | ConvertFrom-Json
                return $getdict
            }
            catch {
                return @{code = 402; data = "Recognizer output deserialization JSON failed, suspected passed non-existent or unrecognizable image. Exception info: $($PSItem.ToString()) Raw content: $($this.imgJson)" }
            }
        }
        else {
            # Input stream does not exist, may be not initialized yet
            return @{ code = 400; data = "Subprocess input stream does not exist" }
        }
    }


    # Recognize image
    [PSCustomObject] run( [string]$imgPath ) {
        # Recognize text in an image.
        # :imgPath: Image path.
        $writeDict = asc (@{ image_path = $imgPath } | ConvertTo-Json -Compress) # Update image path to json format, asc function replaces Chinese
        return $this.runDict($writeDict); # Write image path to pipe
    }

    [PSCustomObject] runClipboard() {
        if ( $this.__ENABLE_CLIPBOARD ) {
            return $this.run("clipboard");
        }
        else {
            throw "Clipboard function does not exist or is disabled."
        }
    }

    [PSCustomObject] runBase64( [string]$imgBase64 ) {
        $writeDict = @{ image_base64 = $imgBase64 } | ConvertTo-Json -Compress
        return $this.runDict($writeDict);
    }

    [PSCustomObject] runByte( $imgByte ) {
        $imgBase64 = [convert]::ToBase64String($imgByte)
        return $this.runBase64($imgBase64);
    }

    # End subprocess
    [void] stop() {
        $this.stdSender.StandardInput.WriteLine('{"exit":""}')
        Write-Host "Recognizer process ended."
    }
}
