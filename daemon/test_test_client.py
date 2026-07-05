import pytest
from unittest.mock import MagicMock
from test_client import read_mem

def test_read_mem_short_bytes():
    mock_sock = MagicMock()
    mock_sock.recv.return_value = b"short"  # Less than 16 bytes

    result = read_mem(mock_sock, 0x1000, 16)

    assert result == b""
    mock_sock.sendall.assert_called_once()
