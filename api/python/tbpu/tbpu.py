# tbpu : text block processing unit
# Base class for text block processors.
# In OCR results, an element containing text, bounding box, and confidence is called a "text block".
# A text block is not necessarily a complete sentence or paragraph. On the contrary, it is usually scattered text.
# An OCR result often consists of multiple text blocks.
# A text block processor: processes multiple input text blocks, such as merging, sorting, deleting text blocks.


class Tbpu:
    def __init__(self):
        self.tbpuName = "Text block processing unit - unknown"

    def run(self, textBlocks):
        """Input: textBlocks list. Example:\n
        [
            {'box': [[29, 19], [172, 19], [172, 44], [29, 44]], 'score': 0.89, 'text': 'text111'},
            {'box': [[29, 60], [161, 60], [161, 86], [29, 86]], 'score': 0.75, 'text': 'text222'},
        ]
        Output: Sorted textBlocks list, each block adds key:
        'end' ending separator
        """
        return textBlocks
