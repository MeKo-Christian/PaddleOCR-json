# Layout parsing - single column - single line

from .tbpu import Tbpu
from .parser_tools.line_preprocessing import linePreprocessing  # Line preprocessing
from .parser_tools.paragraph_parse import word_separator  # Inter-sentence separator


class SingleLine(Tbpu):
    def __init__(self):
        self.tbpuName = "Layout parsing - single column - single line"

    # Find all lines from text block list
    def get_lines(self, textBlocks):
        # Sort by x
        textBlocks.sort(key=lambda tb: tb["normalized_bbox"][0])
        lines = []
        for i1, tb1 in enumerate(textBlocks):
            if not tb1:
                continue
            # Leftmost block
            l1, top1, r1, bottom1 = tb1["normalized_bbox"]
            h1 = bottom1 - top1
            now_line = [tb1]
            # Examine which blocks on the right meet conditions
            for i2 in range(i1 + 1, len(textBlocks)):
                tb2 = textBlocks[i2]
                if not tb2:
                    continue
                l2, top2, r2, bottom2 = tb2["normalized_bbox"]
                h2 = bottom2 - top2
                # Line 2 left side too far forward
                if l2 < r1 - h1:
                    continue
                # Vertical distance too far
                if top2 < top1 - h1 * 0.5 or bottom2 > bottom1 + h1 * 0.5:
                    continue
                # Line height difference too large
                if abs(h1 - h2) > min(h1, h2) * 0.5:
                    continue
                # Meets conditions
                now_line.append(tb2)
                textBlocks[i2] = None
                # Update search conditions
                r1 = r2
            # Finished processing one line
            for i2 in range(len(now_line) - 1):
                # Check horizontal gap between adjacent text blocks in same line
                l1, t1, r1, b1 = now_line[i2]["normalized_bbox"]
                l2, t2, r2, b2 = now_line[i2 + 1]["normalized_bbox"]
                h = (b1 + b2 - t1 - l2) * 0.5
                if l2 - r1 > h * 1.5:  # Gap too large, force space
                    now_line[i2]["end"] = " "
                    continue
                letter1 = now_line[i2]["text"][-1]
                letter2 = now_line[i2 + 1]["text"][0]
                now_line[i2]["end"] = word_separator(letter1, letter2)
            now_line[-1]["end"] = "\n"
            lines.append(now_line)
            textBlocks[i1] = None
        # Sort all lines by y
        lines.sort(key=lambda tbs: tbs[0]["normalized_bbox"][1])
        return lines

    def run(self, textBlocks):
        textBlocks = linePreprocessing(textBlocks)  # Preprocessing
        lines = self.get_lines(textBlocks)  # Get each line
        # Unpack
        textBlocks = []
        for line in lines:
            for tb in line:
                del tb["normalized_bbox"]
                textBlocks.append(tb)
        return textBlocks
