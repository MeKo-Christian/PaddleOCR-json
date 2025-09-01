# Layout parsing - multi-column - natural paragraphs

from .tbpu import Tbpu
from .parser_tools.line_preprocessing import linePreprocessing  # Line preprocessing
from .parser_tools.gap_tree import GapTree  # Gap tree sorting algorithm
from .parser_tools.paragraph_parse import ParagraphParse  # Paragraph analyzer


class MultiPara(Tbpu):
    def __init__(self):
        self.tbpuName = "Layout parsing - multi-column - natural paragraphs"

        # Gap tree object
        self.gtree = GapTree(lambda tb: tb["normalized_bbox"])

        # Paragraph analyzer object
        get_info = lambda tb: (tb["normalized_bbox"], tb["text"])

        def set_end(tb, end):  # Get predicted block ending separator
            tb["end"] = end

        self.pp = ParagraphParse(get_info, set_end)

    def run(self, textBlocks):
        textBlocks = linePreprocessing(textBlocks)  # Preprocessing
        textBlocks = self.gtree.sort(textBlocks)  # Build gap tree
        nodes = self.gtree.get_nodes_text_blocks()  # Get tree node sequence
        # For each node, perform natural paragraph analysis
        for tbs in nodes:
            self.pp.run(tbs)  # Predict ending separator
            for tb in tbs:
                del tb["normalized_bbox"]
        return textBlocks
