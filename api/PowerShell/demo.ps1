Import-Module -Force ".\PPOCR_api.ps1"  # Modify corresponding path

# Initialize recognizer object, pass path to PaddleOCR_json.exe
$exePath = Convert-Path ".\PaddleOCR-json_v.1.3.0_alpha.2\PaddleOCR-json.exe"   # Modify corresponding exe path
$arg = ""  # Parameters are actually not processed in API
$ocr = [PPOCR]::new($exePath,$arg)

while (1) {
    $prompt_str = "1:Image path`n2:Image Base64`n3:Image Byte`n"
    if ( $ocr.isClipboardEnabled() ) {
        $prompt_str += "4:Clipboard`n"
    }
    $prompt_str += "Others to exit"
    $choice = Read-Host $prompt_str
    switch($choice){
        1{
            # Recognize image
            $imgPath = read-host "Please enter image path"
            if ($imgPath) {
                $getObj = $ocr.run($imgPath)
                Write-Host "Image recognition completed, status code: $($getObj.'code') Result:`n$($getObj.'data'|Out-String)`n"
            }
        }
        2{
            $path = read-host "Please enter image path, automatically convert to BASE64"
            $imgBase64 = [convert]::ToBase64String([System.IO.FIle]::ReadAllBytes($path))
            if ($imgBase64) {
                $getObj = $ocr.runBase64($imgBase64)
                Write-Host "Image recognition completed, status code: $($getObj.'code') Result:`n$($getObj.'data'|Out-String)`n"
            }
        }
        3{
            $path = read-host "Please enter image path, automatically convert to Byte"
            $imgByte = [System.IO.FIle]::ReadAllBytes($path)
            if ($imgByte) {
                $getObj = $ocr.runByte($imgByte)
                Write-Host "Image recognition completed, status code: $($getObj.'code') Result:`n$($getObj.'data'|Out-String)`n"
            }
        }
        # The following example is disabled by default
        4{
            if ( $ocr.isClipboardEnabled() ) {
                $getObj = $ocr.runClipboard()
                Write-Host "Image recognition completed, status code: $($getObj.'code') Result:`n$($getObj.'data'|Out-String)`n"
            }
            else {
                $ocr.stop()  # End subprocess.
                Write-Host "Program ended."
                Exit
            }
        }
        Default
        {
            $ocr.stop()  # End subprocess.
            Write-Host "Program ended."
            Exit
        }
    }
}
