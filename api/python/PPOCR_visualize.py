# Visualize PaddleOCR-json results
# Project homepage:
# https://github.com/hiroi-sora/PaddleOCR-json
from PIL import Image, ImageDraw, ImageFont
import math


class visualize:
    """Visualization"""

    # ================================ Static Methods ================================

    @staticmethod
    def createBox(textBlocks, size, fill="#00500040", outline="#11ff22", width=6):
        """Create bounding box layer, return PIL Image object.\n
        :textBlocks: Text block list.\n
        :size: Image size.\n
        The following are optional fields: (colors are 6-digit RGB or 8-digit RGBA hex strings, e.g. #112233ff)\n
        :fill: Bounding box fill color.\n
        :outline: Bounding box outline color.\n
        :width: Bounding box outline thickness, in pixels.
        """
        img = Image.new("RGBA", size, 0)
        draw = ImageDraw.Draw(img)
        for tb in textBlocks:
            box = [
                tuple(tb["box"][0]),
                tuple(tb["box"][1]),
                tuple(tb["box"][2]),
                tuple(tb["box"][3]),
            ]
            draw.polygon(box, fill=fill, outline=outline, width=width)
        return img

    @staticmethod
    def createText(
        textBlocks,
        size,
        ttfPath="C:\Windows\Fonts\msyh.ttc",
        ttfScale=0.9,
        fill="#ff0000",
    ):
        """Create text layer, return PIL Image object.\n
        :textBlocks: Text block list.\n
        :size: Image size.\n
        The following are optional fields:\n
        :ttfPath: Font file path. Defaults to Microsoft YaHei, will error if font doesn't exist.\n
        :ttfScale: Font size overall scaling factor, should be around 1.\n
        :fill: Text color, 6-digit RGB or 8-digit RGBA hex string, e.g. #112233ff.\n
        """
        img = Image.new("RGBA", size, 0)
        draw = ImageDraw.Draw(img)
        ttfDict = {}  # Cache font objects of different sizes
        for tb in textBlocks:
            text = tb["text"]
            xy = tuple(tb["box"][0])  # Top-left coordinate
            xy1 = tb["box"][3]  # Bottom-left coordinate # Line height
            hight = round(
                math.sqrt(((xy[0] - xy1[0]) ** 2) + ((xy[1] - xy1[1]) ** 2)) * ttfScale
            )
            if hight not in ttfDict:
                ttfDict[hight] = ImageFont.truetype(ttfPath, hight)  # Create new size font
            draw.text(xy, text, font=ttfDict[hight], fill=fill)
        return img

    @staticmethod
    def createOrder(
        textBlocks,
        size,
        ttfPath="C:\Windows\Fonts\msyh.ttc",
        ttfSize=50,
        fill="#2233ff",
        bg="#ffffffe0",
    ):
        """Create sequence number layer, return PIL Image object.\n
        :textBlocks: Text block list.\n
        :size: Image size.\n
        The following are optional fields:\n
        :ttfPath: Font file path. Defaults to Microsoft YaHei, will error if font doesn't exist.\n
        :ttfSize: Font size.\n
        :fill: Text color, 6-digit RGB or 8-digit RGBA hex string, e.g. #112233ff.\n
        """
        img = Image.new("RGBA", size, 0)
        draw = ImageDraw.Draw(img)
        ttf = ImageFont.truetype(ttfPath, ttfSize)  # Font
        for index, tb in enumerate(textBlocks):
            text = f"{index+1}"
            xy = tuple(tb["box"][0])  # Top-left coordinate
            x_, y_, w, h = ttf.getbbox(text)  # Get width and height. Only need w and h
            w *= 1.1
            h *= 1.1
            draw.rectangle((xy, (xy[0] + w, xy[1] + h)), fill=bg, width=0)  # Background rectangle
            draw.text(xy, text, font=ttf, fill=fill)  # Text
        return img

    @staticmethod
    def createContrast(img1, img2):
        """Concatenate two images side by side to create contrast layer, return PIL Image object."""
        size = (img1.size[0] + img2.size[0], max(img1.size[1], img2.size[1]))
        img = Image.new("RGBA", size, 0)
        img.paste(img1, (0, 0))
        img.paste(img2, (img1.size[0], 0))
        return img

    @staticmethod
    def composite(img1, img2):
        """Pass two PIL Image objects (RGBA format), use img1 as base, overlay img2 on top
        Return the generated image"""
        return Image.alpha_composite(img1, img2)

    # ================================ Quick Interface ================================

    def __init__(self, textBlocks, imagePath):
        """Create visualization object.\n
        :textBlocks: Text block list, i.e. the data part returned by OCR\n
        :imagePath: Corresponding image path.
        """
        self.imgSource = Image.open(imagePath).convert("RGBA")  # Original image layer
        self.size = self.imgSource.size
        self.imgBox = self.createBox(textBlocks, self.size)  # Bounding box layer
        self.imgText = self.createText(textBlocks, self.size)  # Text layer
        self.imgOrder = self.createOrder(textBlocks, self.size)  # Sequence number layer

    def get(self, isBox=True, isText=False, isOrder=False, isSource=True):
        """Return synthesized visualization result PIL Image.\n
        :isBox: T to return bounding box layer.\n
        :isText: T to return text layer.\n
        :isOrder: T to return sequence number layer.\n
        :isSource: T to return original image. F returns pure visualization result with transparent background.\n
        """
        img = Image.new("RGBA", self.size, 0)
        flags = (isSource, isBox, isText, isOrder)
        for index, im in enumerate(
            [self.imgSource, self.imgBox, self.imgText, self.imgOrder]
        ):
            if im and flags[index]:
                img = visualize.composite(img, im)
        return img

    def show(self, isBox=True, isText=False, isOrder=False, isSource=True):
        """Display visualization result image.\n
        :isBox: T to return bounding box layer.\n
        :isText: T to return text layer.\n
        :isOrder: T to return sequence number layer.\n
        :isSource: T to return original image. F returns pure visualization result with transparent background.\n
        """
        img = self.get(isBox, isText, isOrder, isSource)
        img.show()

    def save(self, path="", isBox=True, isText=False, isOrder=False, isSource=True):
        """Save visualization result image.\n
        :path: Save path.\n
        :isBox: T to return bounding box layer.\n
        :isText: T to return text layer.\n
        :isOrder: T to return sequence number layer.\n
        :isSource: T to return original image. F returns pure visualization result with transparent background.\n
        """
        img = self.get(isBox, isText, isOrder, isSource)
        img.save(path)
