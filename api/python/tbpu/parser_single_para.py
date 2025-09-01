# Layout parsing - single column - natural paragraphs

from .parser_single_line import SingleLine
from .parser_tools.line_preprocessing import linePreprocessing  # Line preprocessing
from .parser_tools.paragraph_parse import ParagraphParse  # Paragraph analyzer


class SinglePara(SingleLine):
    def __init__(self):
        self.tbpuName = "Layout parsing - single column - natural paragraphs"

        # Paragraph analyzer object
        get_info = lambda tb: (tb["normalized_bbox"], tb["text"])

        def set_end(tb, end):  # Get predicted block ending separator
            tb["line"][-1]["end"] = end

        self.pp = ParagraphParse(get_info, set_end)

    def run(self, textBlocks):
        textBlocks = linePreprocessing(textBlocks)  # Preprocessing
        lines = self.get_lines(textBlocks)  # Get each line
        # Package lines as tb
        temp_tbs = []
        for line in lines:
            b0, b1, b2, b3 = line[0]["normalized_bbox"]
            # Search bbox
            for i in range(1, len(line)):
                bb = line[i]["normalized_bbox"]
                b1 = min(b1, bb[1])
                b2 = max(b1, bb[2])
                b3 = max(b1, bb[3])
            # Build tb
            temp_tbs.append(
                {
                    "normalized_bbox": (b0, b1, b2, b3),
                    "text": line[0]["text"][0] + line[-1]["text"][-1],
                    "line": line,
                }
            )
        # Predict ending separator
        self.pp.run(temp_tbs)
        # Unpack
        textBlocks = []
        for t in temp_tbs:
            for tb in t["line"]:
                del tb["normalized_bbox"]
                textBlocks.append(tb)
        return textBlocks
