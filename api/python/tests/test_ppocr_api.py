"""Tests for PPOCR_api.py - PaddleOCR-json Python API."""

import pytest
import unittest.mock as mock
import json
import os
import subprocess
import socket
from unittest.mock import patch, MagicMock, mock_open

from PPOCR_api import PPOCR_pipe, PPOCR_socket, GetOcrApi


class TestPPOCR_pipe:
    """Test the PPOCR_pipe class."""

    def test_init_basic(self, mock_ocr_exe):
        """Test basic initialization of PPOCR_pipe."""
        with patch('os.path.abspath') as mock_abspath, \
             patch('subprocess.Popen') as mock_popen:

            mock_abspath.return_value = mock_ocr_exe
            mock_process = MagicMock()
            mock_popen.return_value = mock_process

            api = PPOCR_pipe(mock_ocr_exe)

            assert api is not None
            mock_popen.assert_called_once()

    def test_init_with_models_path(self, mock_ocr_exe):
        """Test initialization with models path."""
        models_path = "/path/to/models"

        with patch('os.path.abspath') as mock_abspath, \
             patch('os.path.exists') as mock_exists, \
             patch('os.path.isdir') as mock_isdir, \
             patch('subprocess.Popen') as mock_popen:

            mock_abspath.return_value = mock_ocr_exe
            mock_exists.return_value = True
            mock_isdir.return_value = True
            mock_process = MagicMock()
            mock_popen.return_value = mock_process

            api = PPOCR_pipe(mock_ocr_exe, modelsPath=models_path)

            # Check that models_path argument was added to command
            args, kwargs = mock_popen.call_args
            cmd = args[0]
            assert "--models_path" in cmd

    def test_init_invalid_models_path(self, mock_ocr_exe):
        """Test initialization with invalid models path."""
        invalid_models_path = "/invalid/path"

        with patch('os.path.abspath') as mock_abspath, \
             patch('os.path.exists') as mock_exists:

            mock_abspath.return_value = mock_ocr_exe
            mock_exists.return_value = False

            with pytest.raises(Exception) as exc_info:
                PPOCR_pipe(mock_ocr_exe, modelsPath=invalid_models_path)

            assert "doesn't exist" in str(exc_info.value)

    def test_init_with_arguments(self, mock_ocr_exe):
        """Test initialization with custom arguments."""
        arguments = {
            "use_gpu": True,
            "cpu_threads": 4,
            "det": True,
            "cls": False,
            "limit_side_len": 960
        }

        with patch('os.path.abspath') as mock_abspath, \
             patch('subprocess.Popen') as mock_popen:

            mock_abspath.return_value = mock_ocr_exe
            mock_process = MagicMock()
            mock_popen.return_value = mock_process

            api = PPOCR_pipe(mock_ocr_exe, argument=arguments)

            # Check that arguments were added to command
            args, kwargs = mock_popen.call_args
            cmd = args[0]
            assert "--use_gpu=True" in cmd
            assert "--cpu_threads" in cmd
            assert "4" in cmd

    @patch('subprocess.Popen')
    @patch('os.path.abspath')
    def test_run_success(self, mock_abspath, mock_popen, mock_ocr_exe, sample_ocr_response, test_image_path):
        """Test successful OCR run."""
        mock_abspath.return_value = mock_ocr_exe

        # Mock process communication
        mock_process = MagicMock()
        mock_process.communicate.return_value = (
            json.dumps(sample_ocr_response).encode('utf-8'), b""
        )
        mock_process.returncode = 0
        mock_popen.return_value = mock_process

        api = PPOCR_pipe(mock_ocr_exe)
        result = api.run(test_image_path)

        assert result == sample_ocr_response
        assert result["code"] == 100
        assert len(result["data"]) == 1
        assert result["data"][0]["text"] == "Hello World"

    @patch('subprocess.Popen')
    @patch('os.path.abspath')
    def test_run_bytes_success(self, mock_abspath, mock_popen, mock_ocr_exe, sample_ocr_response, test_image_bytes):
        """Test successful OCR run with bytes input."""
        mock_abspath.return_value = mock_ocr_exe

        mock_process = MagicMock()
        mock_process.communicate.return_value = (
            json.dumps(sample_ocr_response).encode('utf-8'), b""
        )
        mock_process.returncode = 0
        mock_popen.return_value = mock_process

        api = PPOCR_pipe(mock_ocr_exe)
        result = api.runBytes(test_image_bytes)

        assert result == sample_ocr_response

    @patch('subprocess.Popen')
    @patch('os.path.abspath')
    def test_run_error_response(self, mock_abspath, mock_popen, mock_ocr_exe, sample_error_response):
        """Test OCR run with error response."""
        mock_abspath.return_value = mock_ocr_exe

        mock_process = MagicMock()
        mock_process.communicate.return_value = (
            json.dumps(sample_error_response).encode('utf-8'), b""
        )
        mock_process.returncode = 0
        mock_popen.return_value = mock_process

        api = PPOCR_pipe(mock_ocr_exe)
        result = api.run("/non/existent/path.jpg")

        assert result == sample_error_response
        assert result["code"] == 200

    def test_print_result_success(self, sample_ocr_response, capsys):
        """Test printResult method with successful response."""
        with patch('os.path.abspath'), \
             patch('subprocess.Popen'):

            api = PPOCR_pipe("/mock/exe")
            api.printResult(sample_ocr_response)

            captured = capsys.readouterr()
            assert "Hello World" in captured.out
            assert "0.95" in captured.out

    def test_print_result_error(self, sample_error_response, capsys):
        """Test printResult method with error response."""
        with patch('os.path.abspath'), \
             patch('subprocess.Popen'):

            api = PPOCR_pipe("/mock/exe")
            api.printResult(sample_error_response)

            captured = capsys.readouterr()
            assert "Error" in captured.out or "does not exist" in captured.out


class TestPPOCR_socket:
    """Test the PPOCR_socket class."""

    def test_init_basic(self):
        """Test basic initialization of PPOCR_socket."""
        with patch('socket.socket') as mock_socket:
            mock_socket_instance = MagicMock()
            mock_socket.return_value = mock_socket_instance

            api = PPOCR_socket("127.0.0.1", 8080)

            assert api.ip == "127.0.0.1"
            assert api.port == 8080

    def test_connect_success(self):
        """Test successful socket connection."""
        with patch('socket.socket') as mock_socket:
            mock_socket_instance = MagicMock()
            mock_socket.return_value = mock_socket_instance
            mock_socket_instance.connect.return_value = None

            api = PPOCR_socket("127.0.0.1", 8080)

            # The connection should be attempted during initialization
            mock_socket_instance.connect.assert_called_with(("127.0.0.1", 8080))

    def test_run_with_socket(self, sample_ocr_response, test_image_path):
        """Test OCR run via socket."""
        response_json = json.dumps(sample_ocr_response)

        with patch('socket.socket') as mock_socket:
            mock_socket_instance = MagicMock()
            mock_socket.return_value = mock_socket_instance
            mock_socket_instance.recv.return_value = response_json.encode('utf-8')

            api = PPOCR_socket("127.0.0.1", 8080)
            result = api.run(test_image_path)

            assert result == sample_ocr_response


class TestGetOcrApi:
    """Test the GetOcrApi factory function."""

    def test_get_pipe_api(self, mock_ocr_exe):
        """Test GetOcrApi returns pipe API for executable path."""
        with patch('os.path.abspath'), \
             patch('subprocess.Popen'):

            api = GetOcrApi(mock_ocr_exe)

            assert isinstance(api, PPOCR_pipe)

    def test_get_socket_api(self):
        """Test GetOcrApi returns socket API for IP:port format."""
        with patch('socket.socket'):
            api = GetOcrApi("127.0.0.1:8080")

            assert isinstance(api, PPOCR_socket)
            assert api.ip == "127.0.0.1"
            assert api.port == 8080

    def test_invalid_socket_format(self):
        """Test GetOcrApi with invalid socket format."""
        with pytest.raises(Exception):
            GetOcrApi("invalid:format:string")

    def test_invalid_port(self):
        """Test GetOcrApi with invalid port number."""
        with pytest.raises(Exception):
            GetOcrApi("127.0.0.1:invalid_port")