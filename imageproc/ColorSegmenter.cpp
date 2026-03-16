/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ColorSegmenter.h"
#include "BinaryImage.h"
#include "BinaryThreshold.h"
#include "ConnectivityMap.h"
#include "GrayImage.h"
#include "ImageCombination.h"
#include "InfluenceMap.h"
#include "RasterOp.h"
#include <QImage>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace imageproc
{

ColorSegmenter::ColorSegmenter(Dpi const& dpi, int const noiseThreshold,
                               int const redThresholdAdjustment,
                               int const greenThresholdAdjustment,
                               int const blueThresholdAdjustment)
:	m_dpi(dpi),
	m_noiseThreshold(noiseThreshold),
	m_redThresholdAdjustment(redThresholdAdjustment),
	m_greenThresholdAdjustment(greenThresholdAdjustment),
	m_blueThresholdAdjustment(blueThresholdAdjustment)
{
}

ColorSegmenter::ColorSegmenter(Dpi const& dpi, int const noiseThreshold)
:	ColorSegmenter(dpi, noiseThreshold, 0, 0, 0)
{
}

namespace
{

struct Component {
	uint32_t size;
	Component() : size(0) {}
	inline uint32_t square() const { return size; }
};

struct BoundingBox {
	int top, left, bottom, right;
	BoundingBox() {
		top = left = std::numeric_limits<int>::max();
		bottom = right = std::numeric_limits<int>::min();
	}
	inline int width() const { return right - left + 1; }
	inline int height() const { return bottom - top + 1; }
	inline void extend(int x, int y) {
		top = std::min(top, y);
		left = std::min(left, x);
		bottom = std::max(bottom, y);
		right = std::max(right, x);
	}
};

class ComponentCleaner
{
public:
	ComponentCleaner(Dpi const& dpi, int noiseThreshold);
	inline bool eligibleForDelete(Component const& component, BoundingBox const& boundingBox) const;
private:
	double m_minAverageWidthThreshold;
	uint32_t m_bigObjectThreshold;
};

ComponentCleaner::ComponentCleaner(Dpi const& dpi, int const noiseThreshold)
{
	int const averageDpi = (dpi.horizontal() + dpi.vertical()) / 2;
	double const dpiFactor = averageDpi / 300.0;
	m_minAverageWidthThreshold = 1.5 * dpiFactor;
	m_bigObjectThreshold = (uint32_t)std::round(std::pow(noiseThreshold, std::sqrt(2.0)) * dpiFactor);
}

bool ComponentCleaner::eligibleForDelete(Component const& component, BoundingBox const& boundingBox) const
{
	if (component.size <= m_bigObjectThreshold) {
		return true;
	}
	double squareRelation = double(component.square()) / (boundingBox.height() * boundingBox.width());
	double averageWidth = std::min(boundingBox.height(), boundingBox.width()) * squareRelation;
	return (averageWidth <= m_minAverageWidthThreshold);
}

enum RgbChannel { RED_CHANNEL, GREEN_CHANNEL, BLUE_CHANNEL };

GrayImage extractRgbChannel(QImage const& image, RgbChannel channel)
{
	if ((image.format() != QImage::Format_RGB32) && (image.format() != QImage::Format_ARGB32)) {
		throw std::invalid_argument("ColorSegmenter: wrong image format.");
	}

	GrayImage dst(image.size());
	uint32_t const* imgLine = reinterpret_cast<uint32_t const*>(image.bits());
	int const imgStride = image.bytesPerLine() / sizeof(uint32_t);
	uint8_t* dstLine = dst.data();
	int const dstStride = dst.stride();
	int const width = image.width();
	int const height = image.height();

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			switch (channel) {
				case RED_CHANNEL:
					dstLine[x] = static_cast<uint8_t>((imgLine[x] >> 16) & 0xff);
					break;
				case GREEN_CHANNEL:
					dstLine[x] = static_cast<uint8_t>((imgLine[x] >> 8) & 0xff);
					break;
				case BLUE_CHANNEL:
					dstLine[x] = static_cast<uint8_t>(imgLine[x] & 0xff);
					break;
			}
		}
		imgLine += imgStride;
		dstLine += dstStride;
	}
	return dst;
}

inline BinaryThreshold adjustThreshold(BinaryThreshold const threshold, int const adjustment)
{
	return BinaryThreshold(qBound(1, int(threshold) + adjustment, 255));
}

void reduceNoise(ConnectivityMap& segmentsMap, Dpi const& dpi, int const noiseThreshold)
{
	ComponentCleaner componentCleaner(dpi, noiseThreshold);
	std::vector<Component> components(segmentsMap.maxLabel() + 1);
	std::vector<BoundingBox> boundingBoxes(segmentsMap.maxLabel() + 1);

	QSize const size = segmentsMap.size();
	int const width = size.width();
	int const height = size.height();

	uint32_t const* mapLine = segmentsMap.data();
	int const mapStride = segmentsMap.stride();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const label = mapLine[x];
			++components[label].size;
			boundingBoxes[label].extend(x, y);
		}
		mapLine += mapStride;
	}

	std::unordered_set<uint32_t> labels;
	for (uint32_t label = 1; label <= segmentsMap.maxLabel(); ++label) {
		if (componentCleaner.eligibleForDelete(components[label], boundingBoxes[label])) {
			labels.insert(label);
		}
	}

	segmentsMap.removeComponents(labels);
}

QImage prepareToMap(QImage const& colorImage, BinaryImage const& image)
{
	QImage dst = colorImage;
	applyMask(dst, image);
	return dst;
}

BinaryImage componentFromChannel(QImage const& colorImage, RgbChannel channel, int adjustment)
{
	GrayImage channelImage = extractRgbChannel(colorImage, channel);
	BinaryThreshold threshold = BinaryThreshold::otsuThreshold(channelImage);
	return BinaryImage(channelImage, adjustThreshold(threshold, adjustment));
}

ConnectivityMap buildMapFromRgb(BinaryImage const& image, QImage const& colorImage,
                                Dpi const& dpi, int noiseThreshold,
                                int redAdj, int greenAdj, int blueAdj)
{
	QImage imageToMap = prepareToMap(colorImage, image);

	BinaryImage redComponent   = componentFromChannel(imageToMap, RED_CHANNEL,   redAdj);
	BinaryImage greenComponent = componentFromChannel(imageToMap, GREEN_CHANNEL, greenAdj);
	BinaryImage blueComponent  = componentFromChannel(imageToMap, BLUE_CHANNEL,  blueAdj);

	BinaryImage yellowComponent(redComponent);
	rasterOp<RopAnd<RopSrc, RopDst>>(yellowComponent, greenComponent);
	BinaryImage magentaComponent(redComponent);
	rasterOp<RopAnd<RopSrc, RopDst>>(magentaComponent, blueComponent);
	BinaryImage cyanComponent(greenComponent);
	rasterOp<RopAnd<RopSrc, RopDst>>(cyanComponent, blueComponent);

	BinaryImage blackComponent(blueComponent);
	rasterOp<RopAnd<RopSrc, RopDst>>(blackComponent, yellowComponent);

	rasterOp<RopSubtract<RopDst, RopSrc>>(redComponent,     blackComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(greenComponent,   blackComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(blueComponent,    blackComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(yellowComponent,  blackComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(magentaComponent, blackComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(cyanComponent,    blackComponent);

	rasterOp<RopSubtract<RopDst, RopSrc>>(redComponent,   yellowComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(redComponent,   magentaComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(greenComponent, yellowComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(greenComponent, cyanComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(blueComponent,  magentaComponent);
	rasterOp<RopSubtract<RopDst, RopSrc>>(blueComponent,  cyanComponent);

	ConnectivityMap segmentsMap(blackComponent,   CONN8);
	segmentsMap.addComponents(yellowComponent,  CONN8);
	segmentsMap.addComponents(magentaComponent, CONN8);
	segmentsMap.addComponents(cyanComponent,    CONN8);
	segmentsMap.addComponents(redComponent,     CONN8);
	segmentsMap.addComponents(greenComponent,   CONN8);
	segmentsMap.addComponents(blueComponent,    CONN8);

	reduceNoise(segmentsMap, dpi, noiseThreshold);

	segmentsMap = ConnectivityMap(InfluenceMap(segmentsMap, image));

	BinaryImage remainingComponents(image);
	rasterOp<RopSubtract<RopDst, RopSrc>>(remainingComponents, segmentsMap.getBinaryMask());
	segmentsMap.addComponents(remainingComponents, CONN8);
	return segmentsMap;
}

ConnectivityMap buildMapFromGrayscale(BinaryImage const& image)
{
	return ConnectivityMap(image, CONN8);
}

class ComponentColor
{
public:
	inline void addPixelColor(uint32_t color) {
		m_red   += qRed(color);
		m_green += qGreen(color);
		m_blue  += qBlue(color);
		++m_size;
	}
	inline uint32_t getColor() {
		return qRgb(getAverage(m_red, m_size), getAverage(m_green, m_size), getAverage(m_blue, m_size));
	}
private:
	static inline int getAverage(uint64_t sum, uint32_t size) {
		int average = (int)(sum / size);
		uint32_t reminder = (uint32_t)(sum % size);
		if (reminder >= (size - reminder)) ++average;
		return average;
	}
	uint32_t m_size = 0;
	uint64_t m_red = 0, m_green = 0, m_blue = 0;
};

QImage buildRgbImage(ConnectivityMap const& segmentsMap, QImage const& colorImage)
{
	int const width  = colorImage.width();
	int const height = colorImage.height();

	std::vector<ComponentColor> compColorMap(segmentsMap.maxLabel() + 1);
	{
		uint32_t const* mapLine = segmentsMap.data();
		int const mapStride = segmentsMap.stride();
		uint32_t const* imgLine = reinterpret_cast<uint32_t const*>(colorImage.bits());
		int const imgStride = colorImage.bytesPerLine() / sizeof(uint32_t);

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				uint32_t const label = mapLine[x];
				if (label == 0) continue;
				compColorMap[label].addPixelColor(imgLine[x]);
			}
			mapLine += mapStride;
			imgLine += imgStride;
		}
	}

	QImage dst(colorImage.size(), QImage::Format_RGB32);
	dst.fill(Qt::white);

	uint32_t* dstLine = reinterpret_cast<uint32_t*>(dst.bits());
	int const dstStride = dst.bytesPerLine() / sizeof(uint32_t);

	uint32_t const* mapLine = segmentsMap.data();
	int const mapStride = segmentsMap.stride();

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const label = mapLine[x];
			if (label == 0) continue;
			dstLine[x] = compColorMap[label].getColor();
		}
		mapLine += mapStride;
		dstLine += dstStride;
	}
	return dst;
}

GrayImage buildGrayImage(ConnectivityMap const& segmentsMap, GrayImage const& grayImage)
{
	int const width  = grayImage.width();
	int const height = grayImage.height();

	std::vector<uint8_t> colorMap(segmentsMap.maxLabel() + 1, 0);
	{
		uint32_t const* mapLine = segmentsMap.data();
		int const mapStride = segmentsMap.stride();
		uint8_t const* imgLine = grayImage.data();
		int const imgStride = grayImage.stride();

		std::vector<Component> components(segmentsMap.maxLabel() + 1);
		std::vector<uint32_t>  graySumMap(segmentsMap.maxLabel() + 1, 0);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				uint32_t const label = mapLine[x];
				if (label == 0) continue;
				++components[label].size;
				graySumMap[label] += imgLine[x];
			}
			mapLine += mapStride;
			imgLine += imgStride;
		}
		for (uint32_t label = 1; label <= segmentsMap.maxLabel(); ++label) {
			colorMap[label] = static_cast<uint8_t>(
				std::round(double(graySumMap[label]) / components[label].size));
		}
	}

	GrayImage dst(grayImage.size());
	dst.fill(0xff);

	uint8_t* dstLine = dst.data();
	int const dstStride = dst.stride();
	uint32_t const* mapLine = segmentsMap.data();
	int const mapStride = segmentsMap.stride();

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const label = mapLine[x];
			if (label == 0) continue;
			dstLine[x] = colorMap[label];
		}
		mapLine += mapStride;
		dstLine += dstStride;
	}
	return dst;
}

} // namespace

QImage ColorSegmenter::segment(BinaryImage const& image, QImage const& colorImage) const
{
	if (image.size() != colorImage.size()) {
		throw std::invalid_argument("ColorSegmenter: images size doesn't match.");
	}
	if ((colorImage.format() == QImage::Format_Indexed8) && colorImage.isGrayscale()) {
		return segment(image, GrayImage(colorImage)).toQImage();
	}
	if ((colorImage.format() != QImage::Format_RGB32) && (colorImage.format() != QImage::Format_ARGB32)) {
		throw std::invalid_argument("ColorSegmenter: wrong image format.");
	}
	ConnectivityMap segmentsMap = buildMapFromRgb(image, colorImage, m_dpi, m_noiseThreshold,
	                                              m_redThresholdAdjustment,
	                                              m_greenThresholdAdjustment,
	                                              m_blueThresholdAdjustment);
	return buildRgbImage(segmentsMap, colorImage);
}

GrayImage ColorSegmenter::segment(BinaryImage const& image, GrayImage const& grayImage) const
{
	if (image.size() != grayImage.size()) {
		throw std::invalid_argument("ColorSegmenter: images size doesn't match.");
	}
	ConnectivityMap segmentsMap = buildMapFromGrayscale(image);
	return buildGrayImage(segmentsMap, grayImage);
}

} // namespace imageproc
