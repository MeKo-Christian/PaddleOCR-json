# Layout parsing - single column - code segment

from .parser_single_line import SingleLine
from .parser_tools.line_preprocessing import linePreprocessing  # Line preprocessing

from bisect import bisect_left


class SingleCode(SingleLine):
    def __init__(self):
        self.tbpuName = "Layout parsing - single column - code segment"

    def merge_line(self, line):  # Merge one line
        A = line[0]
        ba = A["box"]
        ha = ba[3][1] - ba[0][1]  # Block A line height
        score = A["score"]
        for i in range(1, len(line)):
            B = line[i]
            bb = B["box"]
            ha = (ha + bb[3][1] - bb[0][1]) / 2
            # Merge text, add spaces equal to spacing
            space = 0
            if bb[0][0] > ba[1][0]:
                space = round((bb[0][0] - ba[1][0]) / ha)
            A["text"] += "  " * space + B["text"]
            print(space, bb[0][0], ba[1][0])
            # Merge bounding box
            yTop = min(ba[0][1], ba[1][1], bb[0][1], bb[1][1])
            yBottom = max(ba[2][1], ba[3][1], bb[2][1], bb[3][1])
            xLeft = min(ba[0][0], ba[3][0], bb[0][0], bb[3][0])
            xRight = max(ba[1][0], ba[2][0], bb[1][0], bb[2][0])
            ba[0][1] = ba[1][1] = yTop  # y top
            ba[2][1] = ba[3][1] = yBottom  # y bottom
            ba[0][0] = ba[3][0] = xLeft  # x left
            ba[1][0] = ba[2][0] = xRight  # x right
            # Confidence score
            score += B["score"]
        A["score"] = score / len(line)
        del A["normalized_bbox"]
        A["end"] = "\n"
        return A

    def indent(self, tbs):  # Analyze all lines, construct indentation
        lh = 0  # Average line height
        xMin = float("inf")  # Leftmost and rightmost x values of line beginnings
        xMax = float("-inf")
        for tb in tbs:
            b = tb["box"]
            lh += b[3][1] - b[0][1]
            x = b[0][0]
            xMin = min(xMin, x)
            xMax = max(xMax, x)
        lh /= len(tbs)
        lh2 = lh / 2
        # Build indentation level list
        levelList = []
        x = xMin
        while x < xMax:
            levelList.append(x)
            x += lh
        # According to levels, add spaces to beginning of each line and adjust bounding box
        for tb in tbs:
            b = tb["box"]
            level = bisect_left(levelList, b[0][0] + lh2) - 1  # Binary search for level point
            tb["text"] = "  " * level + tb["text"]  # Add spaces
            b[0][0] = b[3][0] = xMin  # Left side to zero

    def run(self, textBlocks):
        textBlocks = linePreprocessing(textBlocks)  # Preprocessing
        lines = self.get_lines(textBlocks)  # Get each line
        tbs = [self.merge_line(line) for line in lines]  # Merge all lines
        self.indent(tbs)  # Add indentation to beginning of each line
        return tbs
