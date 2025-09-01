# Layout parsing - multi-column - single line

from .tbpu import Tbpu
from .parser_tools.line_preprocessing import linePreprocessing  # Line preprocessing
from .parser_tools.gap_tree import GapTree  # Gap tree sorting algorithm


class MultiLine(Tbpu):
    def __init__(self):
        self.tbpuName = "Layout parsing - multi-column - single line"

        # Build algorithm object, specify bounding box element position
        self.gtree = GapTree(lambda tb: tb["normalized_bbox"])

    def run(self, textBlocks):
        textBlocks = linePreprocessing(textBlocks)  # Preprocessing
        textBlocks = self.gtree.sort(textBlocks)  # Build gap tree
        # Add line end separator
        for tb in textBlocks:
            tb["end"] = "\n"
            del tb["normalized_bbox"]
        return textBlocks
