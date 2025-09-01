# Python API to call PaddleOCR-json.exe
# Project Homepage:
# https://github.com/hiroi-sora/PaddleOCR-json

import os
import socket  # socket
import atexit  # exit handling
import subprocess  # process, pipe
import re  # regex
from json import loads as jsonLoads, dumps as jsonDumps
from sys import platform as sysPlatform  # popen silent mode
from base64 import b64encode  # base64 encoding


class PPOCR_pipe:  # Call OCR (pipe mode)
    def __init__(self, exePath: str, modelsPath: str = None, argument: dict = None):
        """Initialize the recognizer (Pipe mode).\n
        `exePath`: Path to the recognizer `PaddleOCR_json.exe`.\n
        `modelsPath`: Path to the recognition library `models` folder. If None, assumes the library is in the same directory as the recognizer.\n
        `argument`: Startup parameters, dictionary `{"key":value}`. Parameter description see https://github.com/hiroi-sora/PaddleOCR-json
        """
        # Private member variables
        self.__ENABLE_CLIPBOARD = False

        exePath = os.path.abspath(exePath)
        cwd = os.path.abspath(os.path.join(exePath, os.pardir))  # Get exe parent folder
        cmds = [exePath]
        # Process startup parameters
        if modelsPath is not None:
            if os.path.exists(modelsPath) and os.path.isdir(modelsPath):
                cmds += ["--models_path", os.path.abspath(modelsPath)]
            else:
                raise Exception(
                    f"Input modelsPath doesn't exist or isn't a directory. modelsPath: [{modelsPath}]"
                )
        if isinstance(argument, dict):
            for key, value in argument.items():
                # Popen() requires all elements in the input list to be str or bytes
                if isinstance(value, bool):
                    cmds += [f"--{key}={value}"]  # Boolean parameters must have key and value together
                elif isinstance(value, str):
                    cmds += [f"--{key}", value]
                else:
                    cmds += [f"--{key}", str(value)]
        # Set subprocess to enable silent mode, don't display console window
        self.ret = None
        startupinfo = None
        if "win32" in str(sysPlatform).lower():
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags = (
                subprocess.CREATE_NEW_CONSOLE | subprocess.STARTF_USESHOWWINDOW
            )
            startupinfo.wShowWindow = subprocess.SW_HIDE
        self.ret = subprocess.Popen(  # Open pipe
            cmds,
            cwd=cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,  # Discard stderr content
            startupinfo=startupinfo,  # Enable silent mode
        )
        # Start subprocess
        while True:
            if not self.ret.poll() == None:  # Subprocess has exited, initialization failed
                raise Exception(f"OCR init fail.")
            initStr = self.ret.stdout.readline().decode("utf-8", errors="ignore")
            if "OCR init completed." in initStr:  # Initialization successful
                break
            elif "OCR clipboard enbaled." in initStr:  # Detected clipboard enabled
                self.__ENABLE_CLIPBOARD = True
        atexit.register(self.exit)  # Register to execute forced stop subprocess when program terminates

    def isClipboardEnabled(self) -> bool:
        return self.__ENABLE_CLIPBOARD

    def getRunningMode(self) -> str:
        # Default pipe mode can only run locally
        return "local"

    def runDict(self, writeDict: dict):
        """Pass instruction dictionary, send to engine process.\n
        `writeDict`: Instruction dictionary.\n
        `return`:  {"code": identification code, "data": content list or error message string}\n"""
        # Check subprocess
        if not self.ret:
            return {"code": 901, "data": f"Engine instance does not exist."}
        if not self.ret.poll() == None:
            return {"code": 902, "data": f"Subprocess has crashed."}
        # Input information
        writeStr = jsonDumps(writeDict, ensure_ascii=True, indent=None) + "\n"
        try:
            self.ret.stdin.write(writeStr.encode("utf-8"))
            self.ret.stdin.flush()
        except Exception as e:
            return {
                "code": 902,
                "data": f"Failed to pass instruction to recognizer process, suspected subprocess has crashed. {e}",
            }
        # Get return value
        try:
            getStr = self.ret.stdout.readline().decode("utf-8", errors="ignore")
        except Exception as e:
            return {"code": 903, "data": f"Failed to read recognizer process output value. Exception info: [{e}]"}
        try:
            return jsonLoads(getStr)
        except Exception as e:
            return {
                "code": 904,
                "data": f"Recognizer output value JSON deserialization failed. Exception info: [{e}]. Original content: [{getStr}]",
            }

    def run(self, imgPath: str):
        """Perform text recognition on a local image.\n
        `exePath`: Image path.\n
        `return`:  {"code": identification code, "data": content list or error message string}\n"""
        writeDict = {"image_path": imgPath}
        return self.runDict(writeDict)

    def runClipboard(self):
        """Immediately perform text recognition on the first image in clipboard.\n
        `return`:  {"code": identification code, "data": content list or error message string}\n"""
        if self.__ENABLE_CLIPBOARD:
            return self.run("clipboard")
        else:
            raise Exception("Clipboard function does not exist or is disabled.")

    def runBase64(self, imageBase64: str):
        """Perform text recognition on an image encoded as base64 string.\n
        `imageBase64`: Image base64 string.\n
        `return`:  {"code": identification code, "data": content list or error message string}\n"""
        writeDict = {"image_base64": imageBase64}
        return self.runDict(writeDict)

    def runBytes(self, imageBytes):
        """Perform text recognition on image byte stream information.\n
        `imageBytes`: Image byte stream.\n
        `return`:  {"code": identification code, "data": content list or error message string}\n"""
        imageBase64 = b64encode(imageBytes).decode("utf-8")
        return self.runBase64(imageBase64)

    def exit(self):
        """Close engine subprocess"""
        if hasattr(self, "ret"):
            if not self.ret:
                return
            try:
                self.ret.kill()  # Close subprocess
            except Exception as e:
                print(f"[Error] ret.kill() {e}")
        self.ret = None
        atexit.unregister(self.exit)  # Remove exit handling
        print("###  PPOCR engine subprocess closed!")

    @staticmethod
    def printResult(res: dict):
        """For debugging, format and print recognition results.\n
        `res`: OCR recognition result."""

        # Recognition successful
        if res["code"] == 100:
            index = 1
            for line in res["data"]:
                print(
                    f"{index}-Confidence: {round(line['score'], 2)}, Text: {line['text']}",
                    end="\\n\n" if line.get("end", "") == "\n" else "\n",
                )
                index += 1
        elif res["code"] == 100:
            print("No text recognized in image.")
        else:
            print(f"Image recognition failed. Error code: {res['code']}, Error message: {res['data']}")

    def __del__(self):
        self.exit()


class PPOCR_socket(PPOCR_pipe):
    """Call OCR (socket mode)"""

    def __init__(self, exePath: str, modelsPath: str = None, argument: dict = None):
        """Initialize recognizer (socket mode).\n
        `exePath`: Path to the recognizer `PaddleOCR_json.exe`.\n
        `modelsPath`: Path to the recognition library `models` folder. If None, assumes the library is in the same directory as the recognizer.\n
        `argument`: Startup parameters, dictionary `{"key":value}`. Parameter description see https://github.com/hiroi-sora/PaddleOCR-json
        """
        # Process parameters
        if not argument:
            argument = {}
        if "port" not in argument:
            argument["port"] = 0  # Random port number
        if "addr" not in argument:
            argument["addr"] = "loopback"  # Local loopback address

        # Process input path, may be local or remote path
        self.__runningMode = self.__configureExePath(exePath)

        # If local path: use PPOCR_pipe to start local engine process
        if self.__runningMode == "local":
            super().__init__(self.exePath, modelsPath, argument)  # Parent class constructor
            self.__ENABLE_CLIPBOARD = super().isClipboardEnabled()
            # Get another line of output, check if server started successfully
            initStr = self.ret.stdout.readline().decode("utf-8", errors="ignore")
            if not self.ret.poll() == None:  # Subprocess has exited, initialization failed
                raise Exception(f"Socket init fail.")
            if "Socket init completed. " in initStr:  # Initialization successful
                splits = initStr.split(":")
                self.ip = splits[0].split("Socket init completed. ")[1]
                self.port = int(splits[1])  # Extract port number
                self.ret.stdout.close()  # Close pipe redirection, prevent buffer filling causing blockage
                print(f"Socket server initialization successful. {self.ip}:{self.port}")
                return

        # If remote path: connect directly
        elif self.__runningMode == "remote":
            self.__ENABLE_CLIPBOARD = False
            # Send an empty instruction, detect remote server availability
            testServer = self.runDict({})
            if testServer["code"] in [902, 903, 904]:
                raise Exception(f"Socket connection fail.")
            print(f"Socket server connection successful. {self.ip}:{self.port}")
            return

        # Exception
        self.exit()
        raise Exception(f"Socket init fail.")

    def isClipboardEnabled(self) -> bool:
        return self.__ENABLE_CLIPBOARD

    def getRunningMode(self) -> str:
        return self.__runningMode

    def runDict(self, writeDict: dict):
        """Pass instruction dictionary, send to engine process.\n
        `writeDict`: Instruction dictionary.\n
        `return`:  {"code": identification code, "data": content list or error message string}\n"""

        # Only check engine process in local mode
        if self.__runningMode == "local":
            # Check subprocess
            if not self.ret.poll() == None:
                return {"code": 901, "data": f"Subprocess has crashed."}

        # Communication
        writeStr = jsonDumps(writeDict, ensure_ascii=True, indent=None) + "\n"
        try:
            # Create TCP connection
            clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            clientSocket.connect((self.ip, self.port))
            # Send data
            clientSocket.sendall(writeStr.encode())
            # After sending all data, close our socket, can only read data from server afterwards
            clientSocket.shutdown(socket.SHUT_WR)
            # Receive data
            resData = b""
            while True:
                chunk = clientSocket.recv(1024)
                if not chunk:
                    break
                resData += chunk
            getStr = resData.decode()
        except ConnectionRefusedError:
            return {"code": 902, "data": "Connection refused"}
        except TimeoutError:
            return {"code": 903, "data": "Connection timeout"}
        except Exception as e:
            return {"code": 904, "data": f"Network error: {e}"}
        finally:
            clientSocket.close()  # Close connection
        # Deserialize output information
        try:
            return jsonLoads(getStr)
        except Exception as e:
            return {
                "code": 905,
                "data": f"Recognizer output value JSON deserialization failed. Exception info: [{e}]. Original content: [{getStr}]",
            }

    def exit(self):
        """Close engine subprocess"""
        # Only close engine process in local mode
        if hasattr(self, "ret"):
            if self.__runningMode == "local":
                if not self.ret:
                    return
                try:
                    self.ret.kill()  # Close subprocess
                except Exception as e:
                    print(f"[Error] ret.kill() {e}")
            self.ret = None

        self.ip = None
        self.port = None
        atexit.unregister(self.exit)  # Remove exit handling
        print("###  PPOCR engine subprocess closed!")

    def __del__(self):
        self.exit()

    def __configureExePath(self, exePath: str) -> str:
        """Process recognizer path, automatically distinguish local path and remote path"""

        pattern = r"remote://(.*):(\d+)"
        match = re.search(pattern, exePath)
        try:
            if match:  # Remote mode
                self.ip = match.group(1)
                self.port = int(match.group(2))
                if self.ip == "any":
                    self.ip = "0.0.0.0"
                elif self.ip == "loopback":
                    self.ip = "127.0.0.1"
                return "remote"
            else:  # Local mode
                self.exePath = exePath
                return "local"
        except:
            return None


def GetOcrApi(
    exePath: str, modelsPath: str = None, argument: dict = None, ipcMode: str = "pipe"
):
    """Get recognizer API object.\n
    `exePath`: Path to the recognizer `PaddleOCR_json.exe`.\n
    `modelsPath`: Path to the recognition library `models` folder. If None, assumes the library is in the same directory as the recognizer.\n
    `argument`: Startup parameters, dictionary `{"key":value}`. Parameter description see https://github.com/hiroi-sora/PaddleOCR-json\n
    `ipcMode`: Process communication mode, optional values are socket mode `socket` or pipe mode `pipe`. Usage is completely consistent.
    """
    if ipcMode == "socket":
        return PPOCR_socket(exePath, modelsPath, argument)
    elif ipcMode == "pipe":
        return PPOCR_pipe(exePath, modelsPath, argument)
    else:
        raise Exception(
            f'ipcMode optional values are socket mode "socket" or pipe mode "pipe", {ipcMode} is not allowed.'
        )
