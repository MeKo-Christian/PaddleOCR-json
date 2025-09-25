// Package ppocr provides Go bindings for PaddleOCR-json
// Project homepage: https://github.com/MeKo-Christian/PaddleOCR-json
package ppocr

import (
	"bufio"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"strconv"
	"strings"
	"sync"
)

// OCRResult represents the result of an OCR operation
type OCRResult struct {
	Code int         `json:"code"`
	Data interface{} `json:"data"`
}

// TextBlock represents a single text block in OCR results
type TextBlock struct {
	Box   [][]int `json:"box"`
	Score float64 `json:"score"`
	Text  string  `json:"text"`
	End   string  `json:"end,omitempty"`
}

// PPOCRInterface defines the interface for OCR operations
type PPOCRInterface interface {
	Run(imagePath string) OCRResult
	RunBytes(imageBytes []byte) OCRResult
	RunBase64(imageBase64 string) OCRResult
	RunClipboard() OCRResult
	IsClipboardEnabled() bool
	GetRunningMode() string
	Exit()
}

// PPOCRPipe implements pipe-based communication with the OCR engine
type PPOCRPipe struct {
	cmd             *exec.Cmd
	stdin           io.WriteCloser
	stdout          *bufio.Scanner
	enableClipboard bool
	mutex           sync.Mutex
}

// NewPPOCRPipe creates a new pipe-based OCR client
func NewPPOCRPipe(exePath string, modelsPath *string, args map[string]interface{}) (*PPOCRPipe, error) {
	exePath, err := filepath.Abs(exePath)
	if err != nil {
		return nil, fmt.Errorf("invalid exe path: %w", err)
	}

	cwd := filepath.Dir(exePath)
	cmdArgs := []string{exePath}

	// Add models path if provided
	if modelsPath != nil {
		if _, err := os.Stat(*modelsPath); os.IsNotExist(err) {
			return nil, fmt.Errorf("models path does not exist: %s", *modelsPath)
		}
		cmdArgs = append(cmdArgs, "--models_path", *modelsPath)
	}

	// Add arguments
	for key, value := range args {
		switch v := value.(type) {
		case bool:
			cmdArgs = append(cmdArgs, fmt.Sprintf("--%s=%t", key, v))
		case string:
			cmdArgs = append(cmdArgs, fmt.Sprintf("--%s", key), v)
		default:
			cmdArgs = append(cmdArgs, fmt.Sprintf("--%s", key), fmt.Sprintf("%v", v))
		}
	}

	cmd := exec.Command(cmdArgs[0], cmdArgs[1:]...)
	cmd.Dir = cwd

	// Set up pipes
	stdin, err := cmd.StdinPipe()
	if err != nil {
		return nil, fmt.Errorf("failed to create stdin pipe: %w", err)
	}

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return nil, fmt.Errorf("failed to create stdout pipe: %w", err)
	}

	// Hide console window on Windows
	if runtime.GOOS == "windows" {
		// Note: Windows-specific code would go here
		// cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	}

	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("failed to start OCR process: %w", err)
	}

	scanner := bufio.NewScanner(stdout)
	enableClipboard := false

	// Read initialization messages
	for scanner.Scan() {
		line := scanner.Text()
		if strings.Contains(line, "OCR init completed.") {
			break
		}
		if strings.Contains(line, "OCR clipboard enbaled.") {
			enableClipboard = true
		}
		if strings.Contains(line, "OCR init fail") {
			cmd.Process.Kill()
			return nil, fmt.Errorf("OCR initialization failed")
		}
	}

	if err := scanner.Err(); err != nil {
		cmd.Process.Kill()
		return nil, fmt.Errorf("error reading init output: %w", err)
	}

	ppocr := &PPOCRPipe{
		cmd:             cmd,
		stdin:           stdin,
		stdout:          bufio.NewScanner(stdout),
		enableClipboard: enableClipboard,
	}

	return ppocr, nil
}

// PPOCRSocket implements socket-based communication with the OCR engine
type PPOCRSocket struct {
	ip              string
	port            int
	runningMode     string
	enableClipboard bool
	localCmd        *exec.Cmd
	localStdin      io.WriteCloser
	localStdout     *bufio.Scanner
	mutex           sync.Mutex
}

// NewPPOCRSocket creates a new socket-based OCR client
func NewPPOCRSocket(exePath string, modelsPath *string, args map[string]interface{}) (*PPOCRSocket, error) {
	// Set default arguments
	if args == nil {
		args = make(map[string]interface{})
	}
	if _, ok := args["port"]; !ok {
		args["port"] = 0 // Random port
	}
	if _, ok := args["addr"]; !ok {
		args["addr"] = "loopback"
	}

	runningMode, ip, port := parseExePath(exePath)

	var cmd *exec.Cmd
	var stdin io.WriteCloser
	var stdout *bufio.Scanner
	enableClipboard := false

	if runningMode == "local" {
		absPath, absErr := filepath.Abs(exePath)
		if absErr != nil {
			return nil, fmt.Errorf("invalid exe path: %w", absErr)
		}
		exePath = absPath

		cwd := filepath.Dir(exePath)
		cmdArgs := []string{exePath}

		// Add models path if provided
		if modelsPath != nil {
			if _, err := os.Stat(*modelsPath); os.IsNotExist(err) {
				return nil, fmt.Errorf("models path does not exist: %s", *modelsPath)
			}
			cmdArgs = append(cmdArgs, "--models_path", *modelsPath)
		}

		// Add arguments
		for key, value := range args {
			switch v := value.(type) {
			case bool:
				cmdArgs = append(cmdArgs, fmt.Sprintf("--%s=%t", key, v))
			case string:
				cmdArgs = append(cmdArgs, fmt.Sprintf("--%s", key), v)
			default:
				cmdArgs = append(cmdArgs, fmt.Sprintf("--%s", key), fmt.Sprintf("%v", v))
			}
		}

		cmd = exec.Command(cmdArgs[0], cmdArgs[1:]...)
		cmd.Dir = cwd

		// Set up pipes
		var err error
		stdin, err = cmd.StdinPipe()
		if err != nil {
			return nil, fmt.Errorf("failed to create stdin pipe: %w", err)
		}

		stdoutPipe, err := cmd.StdoutPipe()
		if err != nil {
			return nil, fmt.Errorf("failed to create stdout pipe: %w", err)
		}

		if err := cmd.Start(); err != nil {
			return nil, fmt.Errorf("failed to start OCR process: %w", err)
		}

		scanner := bufio.NewScanner(stdoutPipe)

		// Read initialization messages
		for scanner.Scan() {
			line := scanner.Text()
			if strings.Contains(line, "Socket init completed.") {
				parts := strings.Split(line, ":")
				if len(parts) >= 2 {
					ip = strings.TrimSpace(strings.Split(parts[0], "Socket init completed.")[1])
					port, _ = strconv.Atoi(strings.TrimSpace(parts[1]))
				}
				break
			}
			if strings.Contains(line, "OCR clipboard enbaled.") {
				enableClipboard = true
			}
		}

		if err := scanner.Err(); err != nil {
			cmd.Process.Kill()
			return nil, fmt.Errorf("error reading init output: %w", err)
		}

		stdout = bufio.NewScanner(stdoutPipe)
		stdoutPipe.Close() // Close pipe after reading init, prevent buffer issues
	}

	// Test connection for both local and remote modes
	testResult := runDictSocket(ip, port, map[string]interface{}{})
	if testResult.Code >= 902 && testResult.Code <= 905 {
		if cmd != nil && cmd.Process != nil {
			cmd.Process.Kill()
		}
		return nil, fmt.Errorf("socket connection failed")
	}

	fmt.Printf("Socket server connection successful. %s:%d\n", ip, port)

	return &PPOCRSocket{
		ip:              ip,
		port:            port,
		runningMode:     runningMode,
		enableClipboard: enableClipboard,
		localCmd:        cmd,
		localStdin:      stdin,
		localStdout:     stdout,
	}, nil
}

// Run performs OCR on an image file
func (p *PPOCRPipe) Run(imagePath string) OCRResult {
	return p.runDict(map[string]interface{}{"image_path": imagePath})
}

// RunBytes performs OCR on image bytes
func (p *PPOCRPipe) RunBytes(imageBytes []byte) OCRResult {
	imageBase64 := base64.StdEncoding.EncodeToString(imageBytes)
	return p.RunBase64(imageBase64)
}

// RunBase64 performs OCR on a base64 encoded image
func (p *PPOCRPipe) RunBase64(imageBase64 string) OCRResult {
	return p.runDict(map[string]interface{}{"image_base64": imageBase64})
}

// RunClipboard performs OCR on clipboard content
func (p *PPOCRPipe) RunClipboard() OCRResult {
	if !p.enableClipboard {
		return OCRResult{Code: 212, Data: "Clipboard function not available"}
	}
	return p.Run("clipboard")
}

// IsClipboardEnabled returns whether clipboard functionality is available
func (p *PPOCRPipe) IsClipboardEnabled() bool {
	return p.enableClipboard
}

// GetRunningMode returns the running mode (always "local" for pipe mode)
func (p *PPOCRPipe) GetRunningMode() string {
	return "local"
}

// GetPID returns the process ID of the OCR engine (pipe mode only)
func (p *PPOCRPipe) GetPID() int {
	if p.cmd != nil && p.cmd.Process != nil {
		return p.cmd.Process.Pid
	}
	return -1
}

// Exit terminates the OCR process and cleans up resources
func (p *PPOCRPipe) Exit() {
	p.mutex.Lock()
	defer p.mutex.Unlock()

	if p.stdin != nil {
		p.stdin.Close()
	}

	if p.cmd != nil && p.cmd.Process != nil {
		p.cmd.Process.Kill()
		p.cmd.Wait()
	}
}

// runDict sends a command dictionary to the OCR process
func (p *PPOCRPipe) runDict(writeDict map[string]interface{}) OCRResult {
	p.mutex.Lock()
	defer p.mutex.Unlock()

	if p.cmd == nil || p.cmd.Process == nil {
		return OCRResult{Code: 901, Data: "Engine instance does not exist"}
	}

	// Check if process is still running
	if p.cmd.ProcessState != nil && p.cmd.ProcessState.Exited() {
		return OCRResult{Code: 902, Data: "Subprocess has crashed"}
	}

	// Send command
	writeBytes, err := json.Marshal(writeDict)
	if err != nil {
		return OCRResult{Code: 904, Data: fmt.Sprintf("JSON marshal error: %v", err)}
	}

	if _, err := p.stdin.Write(append(writeBytes, '\n')); err != nil {
		return OCRResult{Code: 902, Data: fmt.Sprintf("Failed to write to process: %v", err)}
	}

	// Read response
	if !p.stdout.Scan() {
		if err := p.stdout.Err(); err != nil {
			return OCRResult{Code: 903, Data: fmt.Sprintf("Failed to read from process: %v", err)}
		}
		return OCRResult{Code: 903, Data: "No response from process"}
	}

	var result OCRResult
	if err := json.Unmarshal(p.stdout.Bytes(), &result); err != nil {
		return OCRResult{Code: 904, Data: fmt.Sprintf("JSON unmarshal error: %v", err)}
	}

	return result
}

// Run performs OCR on an image file
func (p *PPOCRSocket) Run(imagePath string) OCRResult {
	return p.runDict(map[string]interface{}{"image_path": imagePath})
}

// RunBytes performs OCR on image bytes
func (p *PPOCRSocket) RunBytes(imageBytes []byte) OCRResult {
	imageBase64 := base64.StdEncoding.EncodeToString(imageBytes)
	return p.RunBase64(imageBase64)
}

// RunBase64 performs OCR on a base64 encoded image
func (p *PPOCRSocket) RunBase64(imageBase64 string) OCRResult {
	return p.runDict(map[string]interface{}{"image_base64": imageBase64})
}

// RunClipboard performs OCR on clipboard content
func (p *PPOCRSocket) RunClipboard() OCRResult {
	if !p.enableClipboard {
		return OCRResult{Code: 212, Data: "Clipboard function not available"}
	}
	return p.Run("clipboard")
}

// IsClipboardEnabled returns whether clipboard functionality is available
func (p *PPOCRSocket) IsClipboardEnabled() bool {
	return p.enableClipboard
}

// GetRunningMode returns the running mode
func (p *PPOCRSocket) GetRunningMode() string {
	return p.runningMode
}

// GetIP returns the IP address of the socket connection
func (p *PPOCRSocket) GetIP() string {
	return p.ip
}

// GetPort returns the port number of the socket connection
func (p *PPOCRSocket) GetPort() int {
	return p.port
}

// Exit terminates the OCR process and cleans up resources
func (p *PPOCRSocket) Exit() {
	p.mutex.Lock()
	defer p.mutex.Unlock()

	// For local mode, clean up subprocess
	if p.runningMode == "local" {
		if p.localStdin != nil {
			p.localStdin.Close()
		}

		if p.localCmd != nil && p.localCmd.Process != nil {
			p.localCmd.Process.Kill()
			p.localCmd.Wait()
		}
	}
	// For remote mode, send exit command if supported
	// The remote server should handle its own lifecycle
}

// runDict sends a command dictionary to the OCR process via socket
func (p *PPOCRSocket) runDict(writeDict map[string]interface{}) OCRResult {
	p.mutex.Lock()
	defer p.mutex.Unlock()

	// For local mode, check if process is still running
	if p.runningMode == "local" && (p.localCmd == nil || p.localCmd.Process == nil || p.localCmd.ProcessState != nil && p.localCmd.ProcessState.Exited()) {
		return OCRResult{Code: 901, Data: "Subprocess has crashed"}
	}

	return runDictSocket(p.ip, p.port, writeDict)
}

// runDictSocket sends a command dictionary via TCP socket
func runDictSocket(ip string, port int, writeDict map[string]interface{}) OCRResult {
	writeBytes, err := json.Marshal(writeDict)
	if err != nil {
		return OCRResult{Code: 904, Data: fmt.Sprintf("JSON marshal error: %v", err)}
	}

	conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", ip, port))
	if err != nil {
		return OCRResult{Code: 902, Data: fmt.Sprintf("Connection refused: %v", err)}
	}
	defer conn.Close()

	// Send data
	if _, err := conn.Write(append(writeBytes, '\n')); err != nil {
		return OCRResult{Code: 902, Data: fmt.Sprintf("Failed to send data: %v", err)}
	}

	// Signal end of data
	conn.(*net.TCPConn).CloseWrite()

	// Read response
	scanner := bufio.NewScanner(conn)
	if !scanner.Scan() {
		if err := scanner.Err(); err != nil {
			return OCRResult{Code: 903, Data: fmt.Sprintf("Failed to read response: %v", err)}
		}
		return OCRResult{Code: 903, Data: "No response from server"}
	}

	var result OCRResult
	if err := json.Unmarshal(scanner.Bytes(), &result); err != nil {
		return OCRResult{Code: 905, Data: fmt.Sprintf("JSON unmarshal error: %v", err)}
	}

	return result
}

// parseExePath parses the exe path to determine running mode and connection details
func parseExePath(exePath string) (string, string, int) {
	re := regexp.MustCompile(`remote://(.+):(\d+)`)
	matches := re.FindStringSubmatch(exePath)

	if matches != nil {
		ip := matches[1]
		port, _ := strconv.Atoi(matches[2])

		if ip == "any" {
			ip = "0.0.0.0"
		} else if ip == "loopback" {
			ip = "127.0.0.1"
		}

		return "remote", ip, port
	}

	return "local", "", 0
}

// GetOcrApi creates an OCR client with the specified parameters
func GetOcrApi(exePath string, modelsPath *string, args map[string]interface{}, ipcMode string) (PPOCRInterface, error) {
	switch ipcMode {
	case "socket":
		return NewPPOCRSocket(exePath, modelsPath, args)
	case "pipe":
		return NewPPOCRPipe(exePath, modelsPath, args)
	default:
		return nil, fmt.Errorf("ipcMode must be 'socket' or 'pipe', got: %s", ipcMode)
	}
}

// PrintResult prints OCR results in a formatted way
func PrintResult(result OCRResult) {
	switch result.Code {
	case 100:
		data, ok := result.Data.([]interface{})
		if !ok {
			fmt.Printf("Recognition successful but data format unexpected: %v\n", result.Data)
			return
		}

		for i, item := range data {
			block, ok := item.(map[string]interface{})
			if !ok {
				continue
			}

			score, _ := block["score"].(float64)
			text, _ := block["text"].(string)
			end, _ := block["end"].(string)

			fmt.Printf("%d-Confidence: %.2f, Text: %s", i+1, score, text)
			if end == "\n" {
				fmt.Print("\n")
			}
			fmt.Println()
		}
	case 101:
		fmt.Println("No text recognized in image.")
	default:
		fmt.Printf("Image recognition failed. Error code: %d, Error message: %v\n", result.Code, result.Data)
	}
}
