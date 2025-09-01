# tbpu : text block processing unit text block post-processing

from .tbpu import Tbpu
from .parser_none import ParserNone
from .parser_multi_para import MultiPara
from .parser_multi_line import MultiLine
from .parser_multi_none import MultiNone
from .parser_single_para import SinglePara
from .parser_single_line import SingleLine
from .parser_single_none import SingleNone
from .parser_single_code import SingleCode

# Layout parsing
Parser = {
    "none": ParserNone,  # No processing
    "multi_para": MultiPara,  # Multi-column - natural paragraphs
    "multi_line": MultiLine,  # Multi-column - always line break
    "multi_none": MultiNone,  # Multi-column - no line break
    "single_para": SinglePara,  # Single column - natural paragraphs
    "single_line": SingleLine,  # Single column - always line break
    "single_none": SingleNone,  # Single column - no line break
    "single_code": SingleCode,  # Single column - code block
}


# Get layout parser object
def GetParser(key) -> Tbpu:
    if key in Parser:
        return Parser[key]()
