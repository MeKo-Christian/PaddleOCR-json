// Package tbpu provides text block processing units for OCR result post-processingpackage tbpu

package tbpu

// TextBlock represents a text block with OCR data
type TextBlock struct {
	Box   [][]int `json:"box"`
	Score float64 `json:"score"`
	Text  string  `json:"text"`
	End   string  `json:"end,omitempty"`
}

// TbpuInterface defines the interface for text block processors
type TbpuInterface interface {
	Run(textBlocks []TextBlock) []TextBlock
	GetName() string
}

// BaseTbpu provides a base implementation for text block processors
type BaseTbpu struct {
	Name string
}

// GetName returns the processor name
func (b *BaseTbpu) GetName() string {
	return b.Name
}

// ParserNone implements no processing - just adds default line breaks
type ParserNone struct {
	BaseTbpu
}

// NewParserNone creates a new no-processing parser
func NewParserNone() *ParserNone {
	return &ParserNone{
		BaseTbpu: BaseTbpu{Name: "Layout parsing - no processing"},
	}
}

// Run processes text blocks with no changes except adding default line breaks
func (p *ParserNone) Run(textBlocks []TextBlock) []TextBlock {
	for i := range textBlocks {
		if textBlocks[i].End == "" {
			textBlocks[i].End = "\n"
		}
	}
	return textBlocks
}

// ParserSingleLine implements single column line-based processing
type ParserSingleLine struct {
	BaseTbpu
}

// NewParserSingleLine creates a new single line parser
func NewParserSingleLine() *ParserSingleLine {
	return &ParserSingleLine{
		BaseTbpu: BaseTbpu{Name: "Single column - always line break"},
	}
}

// Run processes text blocks for single column with line breaks
func (p *ParserSingleLine) Run(textBlocks []TextBlock) []TextBlock {
	// Sort by vertical position (top to bottom)
	sortedBlocks := make([]TextBlock, len(textBlocks))
	copy(sortedBlocks, textBlocks)

	// Simple sort by Y coordinate of top-left corner
	for i := 0; i < len(sortedBlocks)-1; i++ {
		for j := i + 1; j < len(sortedBlocks); j++ {
			if len(sortedBlocks[i].Box) > 0 && len(sortedBlocks[j].Box) > 0 {
				y1 := sortedBlocks[i].Box[0][1]
				y2 := sortedBlocks[j].Box[0][1]
				if y1 > y2 {
					sortedBlocks[i], sortedBlocks[j] = sortedBlocks[j], sortedBlocks[i]
				}
			}
		}
	}

	// Add line breaks to all blocks
	for i := range sortedBlocks {
		sortedBlocks[i].End = "\n"
	}

	return sortedBlocks
}

// ParserMultiPara implements multi-column natural paragraph processing
type ParserMultiPara struct {
	BaseTbpu
}

// NewParserMultiPara creates a new multi-column paragraph parser
func NewParserMultiPara() *ParserMultiPara {
	return &ParserMultiPara{
		BaseTbpu: BaseTbpu{Name: "Multi-column - natural paragraphs"},
	}
}

// Run processes text blocks for multi-column layout with natural paragraphs
func (p *ParserMultiPara) Run(textBlocks []TextBlock) []TextBlock {
	if len(textBlocks) == 0 {
		return textBlocks
	}

	// Sort by reading order (left to right, top to bottom)
	sortedBlocks := make([]TextBlock, len(textBlocks))
	copy(sortedBlocks, textBlocks)

	// Sort by Y first, then by X
	for i := 0; i < len(sortedBlocks)-1; i++ {
		for j := i + 1; j < len(sortedBlocks); j++ {
			if len(sortedBlocks[i].Box) > 0 && len(sortedBlocks[j].Box) > 0 {
				y1 := sortedBlocks[i].Box[0][1]
				y2 := sortedBlocks[j].Box[0][1]
				x1 := sortedBlocks[i].Box[0][0]
				x2 := sortedBlocks[j].Box[0][0]

				// If Y coordinates are similar (within threshold), sort by X
				if abs(y1-y2) < 20 {
					if x1 > x2 {
						sortedBlocks[i], sortedBlocks[j] = sortedBlocks[j], sortedBlocks[i]
					}
				} else if y1 > y2 {
					sortedBlocks[i], sortedBlocks[j] = sortedBlocks[j], sortedBlocks[i]
				}
			}
		}
	}

	// Simple paragraph detection based on vertical gaps
	for i := 0; i < len(sortedBlocks)-1; i++ {
		if len(sortedBlocks[i].Box) > 0 && len(sortedBlocks[i+1].Box) > 0 {
			y1 := sortedBlocks[i].Box[2][1]   // bottom Y of current block
			y2 := sortedBlocks[i+1].Box[0][1] // top Y of next block

			// If gap is large, it's a paragraph break
			if y2-y1 > 30 {
				sortedBlocks[i].End = "\n\n"
			} else {
				sortedBlocks[i].End = " "
			}
		}
	}

	// Last block always gets a line break
	if len(sortedBlocks) > 0 {
		sortedBlocks[len(sortedBlocks)-1].End = "\n"
	}

	return sortedBlocks
}

// abs returns the absolute value of an integer
func abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}

// GetParser returns a parser instance by name
func GetParser(name string) TbpuInterface {
	switch name {
	case "none":
		return NewParserNone()
	case "single_line":
		return NewParserSingleLine()
	case "multi_para":
		return NewParserMultiPara()
	default:
		return NewParserNone() // Default to no processing
	}
}
