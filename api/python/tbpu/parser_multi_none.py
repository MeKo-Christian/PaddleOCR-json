# Layout parsing - multi-column - no line breaks

from .tbpu import Tbpu
from .parser_tools.line_preprocessing import linePreprocessing  # Line preprocessing
from .parser_tools.gap_tree import GapTree  # Gap tree sorting algorithm
from .parser_tools.paragraph_parse import word_separator  # Inter-sentence separator


class MultiNone(Tbpu):
    def __init__(self):
        self.tbpuName = "Layout parsing - multi-column - no line breaks"

        # Build algorithm object, specify bounding box element position
        self.gtree = GapTree(lambda tb: tb["normalized_bbox"])

    def run(self, textBlocks):
        textBlocks = linePreprocessing(textBlocks)  # Preprocessing
        textBlocks = self.gtree.sort(textBlocks)  # Build gap tree
        # Add line end separators
        for i in range(len(textBlocks)):
            tb = textBlocks[i]
            if i < len(textBlocks) - 1:
                letter1 = tb["text"][-1]  # Line 1 ending letter
                letter2 = textBlocks[i + 1]["text"][0]  # Line 2 starting letter
                tb["end"] = word_separator(letter1, letter2)  # Get separator
            else:
                tb["end"] = "\n"
            del tb["normalized_bbox"]
        return textBlocks
