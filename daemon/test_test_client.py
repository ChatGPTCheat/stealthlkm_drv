import pytest
from unittest.mock import Mock
import daemon.test_client as tc

def test_get_base_connection_dropped(capsys):
    mock_sock = Mock()
    mock_sock.recv.return_value = b"short"

    result = tc.get_base(mock_sock, "libc.so")

    assert result == 0
    captured = capsys.readouterr()
    assert "[-] Connection dropped early" in captured.out
