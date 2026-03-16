/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "RenderParams.h"
#include "ColorParams.h"
#include "ColorGrayscaleOptions.h"
#include "BlackWhiteOptions.h"
#include "SplittingOptions.h"
#include "DespeckleLevel.h"

namespace output
{

RenderParams::RenderParams(ColorParams const& cp, SplittingOptions const* splitting)
:	m_mask(0)
{
	switch (cp.colorMode()) {
		case ColorParams::BLACK_AND_WHITE: {
			BlackWhiteOptions const& bw(cp.blackWhiteOptions());
			m_mask |= NEED_BINARIZATION;
			if (bw.whiteMargins()) {
				m_mask |= WHITE_MARGINS;
				if (bw.normalizeIllumination()) {
					m_mask |= NORMALIZE_ILLUMINATION;
				}
			}
			break;
		}
		case ColorParams::COLOR_GRAYSCALE: {
			ColorGrayscaleOptions const opt(
				cp.colorGrayscaleOptions()
			);
			if (opt.whiteMargins()) {
				m_mask |= WHITE_MARGINS;
				if (opt.normalizeIllumination()) {
					m_mask |= NORMALIZE_ILLUMINATION;
				}
			}
			break;
		}
		case ColorParams::MIXED: {
			BlackWhiteOptions const& bw(cp.blackWhiteOptions());
			m_mask |= NEED_BINARIZATION|MIXED_OUTPUT;
			if (bw.whiteMargins()) {
				m_mask |= WHITE_MARGINS;
				if (bw.normalizeIllumination()) {
					m_mask |= NORMALIZE_ILLUMINATION;
				}
			}
			break;
		}
	}

	if (splitting && splitting->isSplitOutput()) {
		m_mask |= SPLIT_OUTPUT;
		if (splitting->getSplittingMode() == output::COLOR_FOREGROUND) {
			m_mask |= COLOR_FOREGROUND;
		}
		if (splitting->isOriginalBackgroundEnabled()) {
			m_mask |= ORIGINAL_BACKGROUND;
		}
	}

	// Color segmentation: available in BLACK_AND_WHITE and MIXED modes
	if (cp.colorMode() == ColorParams::BLACK_AND_WHITE || cp.colorMode() == ColorParams::MIXED) {
		if (cp.blackWhiteOptions().getColorSegmenterOptions().isEnabled()) {
			m_mask |= COLOR_SEGMENTATION;
		}
	}

	// Posterization: available in COLOR_GRAYSCALE mode
	if (cp.colorMode() == ColorParams::COLOR_GRAYSCALE) {
		if (cp.colorGrayscaleOptions().getPosterizationOptions().isEnabled()) {
			m_mask |= POSTERIZE;
		}
	}
}

} // namespace output
