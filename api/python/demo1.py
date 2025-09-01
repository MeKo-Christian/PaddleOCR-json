# 👉 demo1.py: Demonstriert grundlegende OCR-Funktionen
#    demo2.py: Demonstriert Visualisierungsinterface
#    demo3.py: Demonstriert OCR-Textsegment-Nachverarbeitung (Absatz-Zusammenführung) Interface

from PPOCR_api import GetOcrApi

import os

# Testbildpfad
TestImagePath = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test.jpg")

# Initialisiere das Erkenner-Objekt, übergebe den PaddleOCR-json Engine-Pfad.
# Engine-Download-Adresse: https://github.com/hiroi-sora/PaddleOCR-json/releases
# Windows: Übergebe den Pfad zu PaddleOCR-json.exe.
# Linux: Übergebe den Pfad zu run.sh
ocr = GetOcrApi(r"Your Path/PaddleOCR-json.exe")

if ocr.getRunningMode() == "local":
    print(f"初始化OCR成功，进程号为{ocr.ret.pid}")
elif ocr.getRunningMode() == "remote":
    print(f"连接远程OCR引擎成功，ip：{ocr.ip}，port：{ocr.port}")
print(f"\n测试图片路径：{TestImagePath}")

# Beispiel 1: Erkennen lokales Bild
res = ocr.run(TestImagePath)
print(f"\n示例1-图片路径识别结果（原始信息）：\n{res}")
print(f"\n示例1-图片路径识别结果（格式化输出）：")
ocr.printResult(res)

# Beispiel 2: Erkennen Bild-Byte-Stream
with open(TestImagePath, "rb") as f:  # Hole Bild-Byte-Stream
    # In der Praxis kann der Byte-Stream durch Netzwerk-Download oder Screenshot erhalten werden, direkt in OCR eingeben, ohne auf lokale Zwischenspeicherung zu speichern.
    imageBytes = f.read()
res = ocr.runBytes(imageBytes)
print(f"\n示例2-字节流识别结果：")
ocr.printResult(res)

# Beispiel 3: Erkennen PIL Image Objekt
try:
    from PIL import Image
    from io import BytesIO
except Exception:
    print("安装Pillow库后方可测试示例3。")
    Image = None
if Image:
    # Erstelle ein PIL Image Objekt
    pilImage = Image.open(TestImagePath)
    # Image Objekt zu Byte-Stream konvertieren
    buffered = BytesIO()
    pilImage.save(buffered, format="PNG")
    imageBytes = buffered.getvalue()
    # In OCR eingeben
    res = ocr.runBytes(imageBytes)
    print(f"\n示例3-PIL Image 识别结果：")
    ocr.printResult(res)

# Folgende Beispiele standardmäßig deaktiviert
# Beispiel 4: Erkennen Zwischenablage-Bild
if ocr.isClipboardEnabled():
    res = ocr.runClipboard()
    if res["code"] == 212:
        print(f"\n示例4-当前剪贴板中没有图片。")
    else:
        print(f"\n示例4-剪贴板识别结果：")
        ocr.printResult(res)
