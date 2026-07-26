import grp
import os
import stat
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
DMA_HEAP = ROOT / "dev/dma_heap/system"
GPU_RULES = ROOT / "etc/udev/rules.d/95-gpu.rules"
SYSTEM_DMA_HEAP_RULE = 'SUBSYSTEM=="dma_heap", KERNEL=="system", GROUP="gpu", MODE="0660"'


def test_system_dma_heap_rule():
  assert SYSTEM_DMA_HEAP_RULE in GPU_RULES.read_text().splitlines()


def test_system_dma_heap_access():
  if not DMA_HEAP.exists():
    pytest.skip("requires a system dma-heap")

  metadata = DMA_HEAP.stat()
  assert grp.getgrgid(metadata.st_gid).gr_name == "gpu"
  assert stat.S_IMODE(metadata.st_mode) == 0o660
  fd = os.open(DMA_HEAP, os.O_RDWR)
  os.close(fd)
