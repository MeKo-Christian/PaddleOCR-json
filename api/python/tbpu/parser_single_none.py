# Layout parsing - single column - no line breaks

from .parser_single_line import SingleLine
from .parser_tools.paragraph_parse import word_separator  # Inter-sentence separator


class SingleNone(SingleLine):
    def __init__(self):
        self.tbpuName = "Layout parsing - single column - no line breaks"

    def run(self, textBlocks):
        textBlocks = super().run(textBlocks)
        # Find line breaks, change to separators
        for i in range(len(textBlocks) - 1):
            if textBlocks[i]["end"] == "\n":
                letter1 = textBlocks[i]["text"][-1]
                letter2 = textBlocks[i + 1]["text"][0]
                textBlocks[i]["end"] = word_separator(letter1, letter2)
        return textBlocks
