# 【Gap·Tree·Sorting Algorithm】 GapTree_Sort_Algorithm
# Perform layout analysis on OCR results or PDF extracted text, sort by human reading order.
# Author: hiroi-sora
# https://github.com/hiroi-sora/GapTree_Sort_Algorithm

from typing import Callable


class GapTree:
    def __init__(self, get_bbox: Callable):
        """
        :param get_bbox: Function, pass single text block,
                        return the text block's top-left, bottom-right coordinate tuple (x0, y0, x1, y1)
        """
        self.get_bbox = get_bbox

    # ======================= Call Interface =====================
    # Sort text block list
    def sort(self, text_blocks: list):
        """
        Sort text block list by human reading order.

        :param text_blocks: Text block object list
        :return: Sorted text block list
        """

        # Package block units, and find page left and right edges
        units, page_l, page_r = self._get_units(text_blocks, self.get_bbox)
        # Find rows and vertical cutting lines
        cuts, rows = self._get_cuts_rows(units, page_l, page_r)
        # Find layout tree
        root = self._get_layout_tree(cuts, rows)
        # Find tree node sequence
        nodes = self._preorder_traversal(root)
        # Find sorted original text block sequence
        new_text_blocks = self._get_text_blocks(nodes)

        # Test: Cache intermediate variables for debug output
        self.current_rows = rows
        self.current_cuts = cuts
        self.current_nodes = nodes

        return new_text_blocks

    # Get text block two-layer list organized by blocks
    def get_nodes_text_blocks(self):
        """
        Get text block two-layer list organized by blocks. Call after sort.

        :return: [ [block1's text_blocks], [block2's text_blocks]... ]
        """
        result = []
        for node in self.current_nodes:
            tbs = []
            if node["units"]:
                for unit in node["units"]:
                    tbs.append(unit[1])
                result.append(tbs)
        return result

    # ======================= Package Block Unit List =====================
    # Package original text blocks as ( (x0,y0,x2,y2), original ). And check page boundaries.
    def _get_units(self, text_blocks, get_bbox):
        # Package unit list units [ ( (x0,y0,x2,y2), original text block ), ... ]
        units = []
        page_l, page_r = float("inf"), -1  # Record left and right extremes of text blocks as page boundaries
        for tb in text_blocks:
            x0, y0, x2, y2 = get_bbox(tb)
            units.append(((x0, y0, x2, y2), tb))
            if x0 < page_l:
                page_l = x0
            if x2 > page_r:
                page_r = x2
        units.sort(key=lambda a: a[0][1])  # Sort by top from top to bottom
        return units, page_l, page_r

    # ======================= Find Rows and Vertical Cutting Lines =====================
    """
    Scan all text blocks, get all rows and vertical cutting lines.
    A row consists of a group of text blocks with close vertical positions.
    A vertical cutting line consists of gaps at the same position in multiple consecutive rows. Gaps divide text blocks of different columns in the same row.
    Input: Text block unit list on a page units=[ ( (x0,y0,x2,y2), _ ) ]. Must be sorted from top to bottom.
    Return:
      Vertical cutting line list cuts=[ ( left edge x, right edge x, start row number, end row number ) ]. Sorted from left to right
      Rows on page rows=[ [unit...] ]. Sorted from top to bottom, left to right
    """

    def _get_cuts_rows(self, units, page_l, page_r):
        # Use gap group gaps2 to update gaps1. Return: updated gaps1, gaps removed from gaps1
        def update_gaps(gaps1, gaps2):
            flags1 = [True for _ in gaps1]  # Whether gaps1[i] is completely removed
            flags2 = [True for _ in gaps2]  # Whether gaps2[i] is newly added
            new_gaps1 = []
            for i1, g1 in enumerate(gaps1):
                l1, r1, _ = g1
                for i2, g2 in enumerate(gaps2):  # For each gap1, examine all gap2
                    l2, r2, _ = g2
                    # Calculate intersection start and end points
                    inter_l = max(l1, l2)
                    inter_r = min(r1, r2)
                    # If intersection is valid
                    if inter_l <= inter_r:
                        # Update gap1 left and right edges
                        new_gaps1.append((inter_l, inter_r, g1[2]))
                        flags1[i1] = False  # Old gap1 should not be removed
                        flags2[i2] = False  # New gap2 should not be added
            # gap2 newly added
            for i2, f2 in enumerate(flags2):
                if f2:
                    new_gaps1.append(gaps2[i2])
            # Record items completely removed from gaps1
            del_gaps1 = []
            for i1, f1 in enumerate(flags1):
                if f1:
                    del_gaps1.append(gaps1[i1])

            return new_gaps1, del_gaps1

        # ========================================

        page_l -= 1  # Ensure page left and right edges don't overlap with text blocks
        page_r += 1
        # Store all rows. "row" refers to unit blocks on the same horizontal line (may belong to multiple columns). [ [unit...] ]
        rows = []
        # Completed vertical cutting lines. [ ( left edge x, right edge x , start row number, end row number ) ]
        completed_cuts = []
        # Examining gaps. [ (left edge x, right edge x , start row number) ]
        gaps = []
        row_index = 0  # Current row number
        unit_index = 0  # Current block number
        # Traverse all text rows from top to bottom
        l_units = len(units)
        while unit_index < l_units:
            # ========== Find current row ==========
            unit = units[unit_index]  # Block at top of current row
            u_bottom = unit[0][3]
            row = [unit]  # Current row
            # Find remaining blocks in current row
            for i in range(unit_index + 1, len(units)):
                next_u = units[i]
                next_top = next_u[0][1]
                if next_top > u_bottom:
                    break  # Next block's top exceeds current bottom, end current row
                row.append(next_u)  # Add block to current row
                unit_index = i  # Step forward traversed block serial number
            # ========== Find current row gaps row_gaps ==========
            row.sort(key=lambda x: (x[0][0], x[0][2]))  # Blocks in current row sorted from left to right
            row_gaps = []  # Current row gaps [ ( ( left edge l, right edge r ), start row number) ]
            search_start = page_l  # This round search segment start point is page left edge
            for u in row:  # Traverse blocks in current row
                l = u[0][0]  # Block left side
                r = u[0][2]  # Block right side
                # If block start point is greater than search start point, add this part to result
                if l > search_start:
                    row_gaps.append((search_start, l, row_index))
                # If block end point is greater than search start point, update search start point
                if r > search_start:
                    search_start = r
            # Page right edge add last gap
            row_gaps.append((search_start, page_r, row_index))
            # ========== Update examining gap group ==========
            gaps, del_gaps = update_gaps(gaps, row_gaps)
            # Items removed from gaps, add to completed vertical cutting lines completed_cuts
            row_max = row_index - 1  # Vertical cutting line end row number
            for dg1 in del_gaps:
                completed_cuts.append((*dg1, row_max))
            # ========== End ==========
            rows.append(row)  # Add current row to total row list
            unit_index += 1
            row_index += 1
        # Traversal ended, collect remaining gaps in gaps, form vertical cutting lines extending to last row
        row_max = len(rows) - 1  # Vertical cutting line end row number
        for g in gaps:
            completed_cuts.append((*g, row_max))
        completed_cuts.sort(key=lambda c: c[0])
        return completed_cuts, rows

    # ======================= Find Layout Tree =====================
    """
    A layout tree node represents a block. Definition:
    node = {
        "x_left": node left edge x,
        "x_right": right edge x,
        "r_top": top row number,
        "r_bottom": bottom row number,
        "units": [], # text block list inside node (empty for root node, non-empty for other nodes)
        "children": [], # child nodes, ordered
    }
    """

    def _get_layout_tree(self, cuts, rows):
        # Vertical cutting lines cut a horizontal row, broken areas are "gaps".
        # Generate gap (left, right) coordinate list for each row
        rows_gaps = [[] for _ in rows]
        for g_i, cut in enumerate(cuts):
            for r_i in range(cut[2], cut[3] + 1):
                rows_gaps[r_i].append((cut[0], cut[1]))

        root = {  # Root node
            "x_left": cuts[0][0] - 1,
            "x_right": cuts[-1][1] + 1,
            "r_top": -1,
            "r_bottom": -1,
            "units": [],
            "children": [],
        }
        completed_nodes = [root]  # Nodes that have completed ending
        now_nodes = []  # Nodes currently being considered. No order

        # ========== End a node, add to node tree ==========
        def complete(node):
            node_r = node["x_right"] - 2  # Current node right boundary
            max_nodes = []  # Lowest completed node list that meets parent node conditions
            max_r = -2  # Lowest row count that meets parent node conditions
            # In completed list, find parent node
            for com_node in completed_nodes:
                # Parent node's vertical projection must contain current right boundary
                if node_r < com_node["x_left"] or node_r > com_node["x_right"] + 0.0001:
                    continue
                # Parent node bottom must be above current
                if com_node["r_bottom"] >= node["r_top"]:
                    continue
                # Encounter lower node that meets conditions
                if com_node["r_bottom"] > max_r:
                    max_r = com_node["r_bottom"]
                    max_nodes = [com_node]
                    continue
                # Encounter node at same level that meets conditions
                if com_node["r_bottom"] == max_r:
                    max_nodes.append(com_node)
                    continue
            # In lowest list, find rightmost node as parent node
            max_node = max(max_nodes, key=lambda n: n["x_right"])
            max_node["children"].append(node)  # Add to parent node
            completed_nodes.append(node)  # Add to completed list

        # ========== Traverse each row, update node tree ==========
        for r_i, row in enumerate(rows):
            row_gaps = rows_gaps[r_i]  # Current row gap group
            u_i = g_i = 0  # Current examining text block, gap index

            # ========== Check if any nodes being considered can end ==========
            new_nodes = []
            for node in now_nodes:  # Traverse nodes
                l_flag = r_flag = False  # Mark whether node left and right edges continue
                completed_flag = False  # Mark whether node can end
                x_left = node["x_left"]  # Left and right edge coordinates
                x_right = node["x_right"]
                for gap in row_gaps:  # Traverse all gaps in this row
                    if gap[1] == x_left:  # Node left edge continued by gap right side
                        l_flag = True
                    if gap[0] == x_right:  # Right edge continued by gap left side
                        r_flag = True
                    # Any gap below this node breaks this node
                    if x_left < gap[0] < x_right or x_left < gap[1] < x_right:
                        completed_flag = True
                        break
                if not l_flag or not r_flag:  # Either left or right edge cannot continue
                    completed_flag = True
                if completed_flag:  # Node ends, add to node tree
                    complete(node)
                else:  # Node continues
                    node["r_bottom"] = r_i
                    new_nodes.append(node)
            now_nodes = new_nodes

            # ========== Traverse from left to right, add text blocks to corresponding column nodes ==========
            while u_i < len(row):
                unit = row[u_i]  # Current block
                # ========== Current block unit is between gap g_i and g_i+1 ==========
                x_l = row_gaps[g_i][1]  # Left gap g_i right boundary
                x_r = row_gaps[g_i + 1][0]  # Right gap g_i+1 left boundary
                # Check if interval is correct
                if unit[0][0] + 0.0001 > x_r:  # Block is to the right of right gap, means reached next interval
                    g_i += 1  # Gap step forward, block not step forward
                    continue
                # ========== Check if current block can be added to existing node ==========
                flag = False
                for node in now_nodes:
                    # If some node's left and right coordinates match current block, current block joins node
                    if node["x_left"] == x_l and node["x_right"] == x_r:
                        node["units"].append(unit)
                        flag = True
                        break
                if flag:
                    u_i += 1  # Block step forward
                    continue
                # ========== Create new node based on current block, add to pending consideration nodes ==========
                now_nodes.append(
                    {
                        "x_left": x_l,
                        "x_right": x_r,
                        "r_top": r_i,
                        "r_bottom": r_i,
                        "units": [unit],
                        "children": [],
                    }
                )
                u_i += 1  # Block step forward
        # Add remaining nodes to node tree too
        for node in now_nodes:
            complete(node)
        # Organize all nodes
        for node in completed_nodes:
            # All child nodes sorted from left to right
            node["children"].sort(key=lambda n: n["x_left"])
            # All block units sorted from top to bottom
            node["units"].sort(key=lambda u: u[0][1])
        return root

    # ======================= Preorder traversal of layout tree, find node sequence =====================
    def _preorder_traversal(self, root):
        if not root:
            return []
        stack = [root]
        result = []
        while stack:
            node = stack.pop()
            result.append(node)
            # Push current node's child nodes into stack in reverse order to ensure left child nodes are processed before right child nodes
            stack += reversed(node["children"])
        return result

    # ======================= Extract original text block sequence from node sequence =====================
    def _get_text_blocks(self, nodes):
        result = []
        for node in nodes:
            for unit in node["units"]:
                result.append(unit[1])
        return result
