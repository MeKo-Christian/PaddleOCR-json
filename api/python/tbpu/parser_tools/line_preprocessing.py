# =========================================
# =============== Line Preprocessing ===============
# =========================================

from math import atan2, cos, sin, sqrt, pi, radians, degrees
from statistics import median  # Median

angle_threshold = 3  # Minimum angle threshold for performing some operations
angle_threshold_rad = radians(angle_threshold)


# Calculate distance between two points
def _distance(point1, point2):
    return sqrt((point2[0] - point1[0]) ** 2 + (point2[1] - point1[1]) ** 2)


# Calculate rotation angle of a box
def _calculateAngle(box):
    # Get width and height
    width = _distance(box[0], box[1])
    height = _distance(box[1], box[2])
    # Choose the pair of vertices with larger distance, calculate angle in radians
    if width < height:
        angle_rad = atan2(box[2][1] - box[1][1], box[2][0] - box[1][0])
    else:
        angle_rad = atan2(box[1][1] - box[0][1], box[1][0] - box[0][0])
    # Normalize angle to [-pi/2, pi/2) range (with threshold)
    if angle_rad < -pi / 2 + angle_threshold_rad:
        angle_rad += pi
    elif angle_rad >= pi / 2 + angle_threshold_rad:
        angle_rad -= pi
    return angle_rad


# Estimate rotation angle of a group of text blocks
def _estimateRotation(textBlocks):
    # blocks["box"] = [top-left, top-right, bottom-right, bottom-left]
    angle_rads = (_calculateAngle(block["box"]) for block in textBlocks)
    median_angle = median(angle_rads)  # Median
    return median_angle


# Get rotated standard bbox. angle_threshold is the threshold for performing rotation (minimum angle value).
def _getBboxes(textBlocks, rotation_rad):
    # If angle is below threshold (close to 0°), do not rotate to improve performance.
    if abs(rotation_rad) <= angle_threshold_rad:
        bboxes = [
            (  # Construct bbox directly
                min(x for x, y in tb["box"]),
                min(y for x, y in tb["box"]),
                max(x for x, y in tb["box"]),
                max(y for x, y in tb["box"]),
            )
            for tb in textBlocks
        ]
    # Otherwise, perform rotation operation.
    else:
        # print(f"Text block preprocessing rotation {degrees(rotation_rad):.2f} °")
        bboxes = []
        min_x, min_y = float("inf"), float("inf")  # Initialize minimum x and y coordinates
        cos_angle = cos(-rotation_rad)  # Calculate angle cosine value
        sin_angle = sin(-rotation_rad)
        for tb in textBlocks:
            box = tb["box"]
            rotated_box = [  # Rotate each vertex of box
                (cos_angle * x - sin_angle * y, sin_angle * x + cos_angle * y)
                for x, y in box
            ]
            # Unpack rotated vertex coordinates, get all x and y values respectively
            xs, ys = zip(*rotated_box)
            # Build standard bbox (top-left x, top-left y, bottom-right x, bottom-right y)
            bbox = (min(xs), min(ys), max(xs), max(ys))
            bboxes.append(bbox)
            min_x, min_y = min(min_x, bbox[0]), min(min_y, bbox[1])
        # If negative coordinates exist after rotation, translate all bounding boxes so that minimum x and y coordinates are 0, ensuring all coordinates are non-negative
        if min_x < 0 or min_y < 0:
            bboxes = [
                (x - min_x, y - min_y, x2 - min_x, y2 - min_y)
                for (x, y, x2, y2) in bboxes
            ]
    return bboxes


# Preprocess textBlocks, convert bounding boxes ["box"] to normalized bbox
def linePreprocessing(textBlocks):
    # Determine angle
    rotation_rad = _estimateRotation(textBlocks)
    # Get normalized bbox
    bboxes = _getBboxes(textBlocks, rotation_rad)
    # Write to tb
    for i, tb in enumerate(textBlocks):
        tb["normalized_bbox"] = bboxes[i]
    # Sort by y
    textBlocks.sort(key=lambda tb: tb["normalized_bbox"][1])
    return textBlocks
