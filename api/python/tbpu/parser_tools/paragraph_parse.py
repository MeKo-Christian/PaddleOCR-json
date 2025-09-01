# Paragraph analyzer
# For text blocks that are already within one column block, determine their paragraph relationships.

from typing import Callable
import unicodedata


# Pass previous sentence ending character and next sentence starting character, return separator
def word_separator(letter1, letter2):

    # Determine if Unicode character belongs to Chinese, Japanese or Korean character sets
    def is_cjk(character):
        cjk_unicode_ranges = [
            (0x4E00, 0x9FFF),  # Chinese
            (0x3040, 0x30FF),  # Japanese
            (0x1100, 0x11FF),  # Korean
            (0x3130, 0x318F),  # Korean compatibility letters
            (0xAC00, 0xD7AF),  # Korean syllables
            # Full-width symbols
            (0x3000, 0x303F),  # Chinese symbols and punctuation
            (0xFE30, 0xFE4F),  # Chinese compatibility form punctuation
            (0xFF00, 0xFFEF),  # Half-width and full-width form characters
        ]
        return any(start <= ord(character) <= end for start, end in cjk_unicode_ranges)

    if is_cjk(letter1) and is_cjk(letter2):
        return ""

    # Special case: previous text is hyphen.
    if letter1 == "-":
        return ""
    # Special case: next text is any punctuation symbol.
    if unicodedata.category(letter2).startswith("P"):
        return ""
    # Other normal cases add space
    return " "


TH = 1.2  # Line height used as comparison threshold


class ParagraphParse:
    def __init__(self, get_info: Callable, set_end: Callable) -> None:
        """
        :param get_info: Function, pass single text block,
                return the text block's info tuple: ( (x0, y0, x1, y1), "text" )
        :param set_end: Function, pass single text block and text ending separator, this function should save the separator.
        """
        self.get_info = get_info
        self.set_end = set_end

    # ======================= Call Interface: Predict ending separators for text block list =====================
    def run(self, text_blocks: list):
        """
        For text block list belonging to one block area, perform paragraph analysis, predict ending separator for each text block.

        :param text_blocks: Text block object list
        :return: Sorted text block list
        """
        # Package block units
        units = self._get_units(text_blocks, self.get_info)
        # Execute analysis
        self._parse(units)
        return text_blocks

    # ======================= Package Block Unit List =====================
    # Package original text blocks as ( (x0,y0,x2,y2), ("start","end"), original ) .
    def _get_units(self, text_blocks, get_info):
        units = []
        for tb in text_blocks:
            bbox, text = get_info(tb)
            units.append((bbox, (text[0], text[-1]), tb))
        return units

    # ======================= Analysis =====================

    # Execute analysis
    def _parse(self, units):
        units.sort(key=lambda a: a[0][1])  # Ensure ordered from top to bottom
        para_l, para_top, para_r, para_bottom = units[0][0]  # Current paragraph left and right
        para_line_h = para_bottom - para_top  # Current paragraph line height
        para_line_s = None  # Current paragraph line spacing
        now_para = [units[0]]  # Current paragraph blocks
        paras = []  # Total paragraphs
        paras_line_space = []  # Total paragraph line spacing
        # Take left and right equal as main body of natural paragraph
        for i in range(1, len(units)):
            l, top, r, bottom = units[i][0]  # Current block top bottom left right edges
            h = bottom - top
            ls = top - para_bottom  # Line spacing
            # Detect if same paragraph
            if (  # Both left and right edges equal
                abs(para_l - l) <= para_line_h * TH
                and abs(para_r - r) <= para_line_h * TH
                # Line spacing not large
                and (para_line_s == None or ls < para_line_s + para_line_h * 0.5)
            ):
                # Update data
                para_l = (para_l + l) / 2
                para_r = (para_r + r) / 2
                para_line_h = (para_line_h + h) / 2
                para_line_s = ls if para_line_s == None else (para_line_s + ls) / 2
                # Add to current paragraph
                now_para.append(units[i])
            else:  # Not same paragraph, archive previous paragraph, create new paragraph
                paras.append(now_para)
                paras_line_space.append(para_line_s)
                now_para = [units[i]]
                para_l, para_r, para_line_h = l, r, bottom - top
                para_line_s = None
            para_bottom = bottom
        # Archive last paragraph
        paras.append(now_para)
        paras_line_space.append(para_line_s)

        # Merge single-line paragraphs, add to previous/next paragraph as first/last sentence
        for i1 in reversed(range(len(paras))):
            para = paras[i1]
            if len(para) == 1:
                l, top, r, bottom = para[0][0]
                up_flag = down_flag = False
                # Previous paragraph end condition: left aligned, right not exceeding, line spacing small enough
                if i1 > 0:
                    # Check left and right
                    up_l, up_top, up_r, up_bottom = paras[i1 - 1][-1][0]
                    up_dist, up_h = abs(up_l - l), up_bottom - up_top
                    up_flag = up_dist <= up_h * TH and r <= up_r + up_h * TH
                    # Check line spacing
                    if (
                        paras_line_space[i1 - 1] != None
                        and top - up_bottom > paras_line_space[i1 - 1] + up_h * 0.5
                    ):
                        up_flag = False
                # Next paragraph start condition: right aligned/single line exceeding, left indented
                if i1 < len(paras) - 1:
                    down_l, down_top, down_r, down_bottom = paras[i1 + 1][0][0]
                    down_h = down_bottom - down_top
                    # Left aligned or indented
                    if down_l - down_h * TH <= l <= down_l + down_h * (1 + TH):
                        if len(paras[i1 + 1]) > 1:  # Multi-line, right aligned
                            down_flag = abs(down_r - r) <= down_h * TH
                        else:  # Single line, right can exceed
                            down_flag = down_r - down_h * TH < r
                    # Check line spacing
                    if (
                        paras_line_space[i1 + 1] != None
                        and down_top - bottom > paras_line_space[i1 + 1] + down_h * 0.5
                    ):
                        down_flag = False

                # Choose to add to previous or next paragraph
                if up_flag and down_flag:  # Both paragraphs meet conditions, choose the one with closer vertical distance
                    if top - up_bottom < down_top - bottom:
                        paras[i1 - 1].append(para[0])
                    else:
                        paras[i1 + 1].insert(0, para[0])
                elif up_flag:  # Only one paragraph meets conditions, choose directly
                    paras[i1 - 1].append(para[0])
                elif down_flag:
                    paras[i1 + 1].insert(0, para[0])
                if up_flag or down_flag:
                    del paras[i1]
                    del paras_line_space[i1]

        # Refresh all paragraphs, add end
        for para in paras:
            for i1 in range(len(para) - 1):
                letter1 = para[i1][1][1]  # Line 1 ending letter
                letter2 = para[i1 + 1][1][0]  # Line 2 starting letter
                sep = word_separator(letter1, letter2)
                self.set_end(para[i1][2], sep)
            self.set_end(para[-1][2], "\n")
        return units
