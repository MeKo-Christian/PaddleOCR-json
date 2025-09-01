# Layout parsing - no processing

from .tbpu import Tbpu


class ParserNone(Tbpu):
    def __init__(self):
        self.tbpuName = "Layout parsing - no processing"

    def run(self, textBlocks):
        for tb in textBlocks:
            if "end" not in tb:
                tb["end"] = "\n"  # Default ending separator is line break
        return textBlocks
