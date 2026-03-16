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

#include "BlackOnWhiteEstimator.h"
#include "imageproc/Binarize.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Grayscale.h"
#include "imageproc/Morphology.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/RasterOp.h"
#include "imageproc/Transform.h"
#include "DebugImages.h"
#include "Despeckle.h"
#include "TaskStatus.h"
#include "Dpi.h"
#include <stdexcept>

using namespace imageproc;

bool BlackOnWhiteEstimator::isBlackOnWhiteRefining(
	GrayImage const& grayImage,
	ImageTransformation const& xform,
	TaskStatus const& status,
	DebugImages* dbg)
{
	BinaryImage bw150;
	{
		ImageTransformation xform150dpi(xform);
		xform150dpi.preScaleToDpi(Dpi(150, 150));

		if (xform150dpi.resultingRect().toRect().isEmpty()) {
			return true;
		}

		QImage gray150(transformToGray(
			grayImage, xform150dpi.transform(),
			xform150dpi.resultingRect().toRect(),
			OutsidePixels::assumeColor(Qt::white)
		));
		bw150 = binarizeOtsu(gray150);

		Despeckle::despeckleInPlace(bw150, Dpi(150, 150), Despeckle::NORMAL, status);
		bw150.invert();
		Despeckle::despeckleInPlace(bw150, Dpi(150, 150), Despeckle::NORMAL, status);
		bw150.invert();
		if (dbg) dbg->add(bw150, "bw150");
	}

	status.throwIfCancelled();

	BinaryImage contentMask;
	{
		// White top-hat = original AND NOT(openBrick(original))
		BinaryImage whiteTopHat(bw150);
		BinaryImage opened = openBrick(bw150, QSize(13, 13));
		rasterOp<RopAnd<RopDst, RopNot<RopSrc>>>(whiteTopHat, opened);

		// Black top-hat = closeBrick(original) AND NOT(original)
		BinaryImage blackTopHat = closeBrick(bw150, QSize(13, 13));
		rasterOp<RopAnd<RopDst, RopNot<RopSrc>>>(blackTopHat, bw150);

		contentMask = whiteTopHat;
		rasterOp<RopOr<RopSrc, RopDst>>(contentMask, blackTopHat);

		contentMask = closeBrick(contentMask, QSize(200, 200));
		contentMask = dilateBrick(contentMask, QSize(30, 30));
		if (dbg) dbg->add(contentMask, "content_mask");
	}

	status.throwIfCancelled();

	rasterOp<RopAnd<RopSrc, RopDst>>(bw150, contentMask);
	return (2 * bw150.countBlackPixels() <= contentMask.countBlackPixels());
}

bool BlackOnWhiteEstimator::isBlackOnWhite(
	GrayImage const& grayImage,
	ImageTransformation const& xform,
	TaskStatus const& status,
	DebugImages* dbg)
{
	if (isBlackOnWhite(grayImage, xform.resultingPreCropArea())) {
		return true;
	} else {
		return isBlackOnWhiteRefining(grayImage, xform, status, dbg);
	}
}

bool BlackOnWhiteEstimator::isBlackOnWhite(
	GrayImage const& img,
	BinaryImage const& mask)
{
	if (img.isNull()) {
		throw std::invalid_argument("BlackOnWhiteEstimator: image is null.");
	}
	if (img.size() != mask.size()) {
		throw std::invalid_argument("BlackOnWhiteEstimator: img and mask have different sizes");
	}

	BinaryImage bwImage(img, BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)));
	rasterOp<RopAnd<RopSrc, RopDst>>(bwImage, mask);
	return (2 * bwImage.countBlackPixels() <= mask.countBlackPixels());
}

bool BlackOnWhiteEstimator::isBlackOnWhite(
	GrayImage const& img,
	QPolygonF const& cropArea)
{
	if (img.isNull()) {
		throw std::invalid_argument("BlackOnWhiteEstimator: image is null.");
	}
	if (cropArea.intersected(QRectF(img.rect())).isEmpty()) {
		throw std::invalid_argument("BlackOnWhiteEstimator: the cropping area is wrong.");
	}

	BinaryImage mask(img.size(), BLACK);
	PolygonRasterizer::fillExcept(mask, WHITE, cropArea, Qt::WindingFill);
	return isBlackOnWhite(img, mask);
}
