/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "VectorPdfExporter.h"
#include "PotraceVectorizer.h"
#include "TiffReader.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageView.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QByteArray>
#include <QPair>
#include <QDebug>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

// ==================== Two-Phase PDF Writer ====================
//
// Phase 1: Load all pages, run OCR in parallel (N threads = N cores).
// Phase 2: Sequential PDF assembly (JPEG encode + write).
//
// Without OCR: Phase 1 just loads images, Phase 2 is instant.

// Helper: find tessdata directory
static QString findTessDataDir(QString const& userPath, QString const& language)
{
	QStringList candidates;
	if (!userPath.isEmpty()) {
		if (userPath.endsWith("/tessdata") || userPath.endsWith("\\tessdata")) {
			candidates << userPath;
		} else if (userPath.endsWith("/tessdata/") || userPath.endsWith("\\tessdata\\")) {
			QString p = userPath;
			p.chop(1);
			candidates << p;
		} else {
			candidates << (userPath + "/tessdata");
		}
	}

	static QStringList const systemDirs = {
		"C:/msys64/mingw64/share/tessdata",
		"/usr/share/tessdata",
		"/usr/local/share/tessdata"
	};
	for (QString const& td : systemDirs) {
		if (QDir(td).exists()) {
			candidates << td;
		}
	}

	QString firstLang = language.split('+').first();
	for (QString const& c : candidates) {
		if (QFile::exists(c + "/" + firstLang + ".traineddata")) {
			return c;
		}
	}
	return QString();
}

VectorPdfExporter::ExportResult
VectorPdfExporter::exportPdf(
	IntrusivePtr<ProjectPages> const& pages,
	OutputFileNameGenerator const& fileNameGen,
	QString const& outputPdfPath,
	Options const& opts,
	ProgressCallback progress)
{
	ExportResult result;
	result.pageCount = 0;
	result.ocrWordCount = 0;
	result.tessInitOk = false;

	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const totalPages = static_cast<int>(seq.numPages());

	if (totalPages == 0) {
		return result;
	}

	// ══════════════════════════════════════════════════════════
	// Phase 1: Load all pages + parallel OCR
	// ══════════════════════════════════════════════════════════

	struct PageData {
		QImage img;
		QString imgPath;
		QList<OcrWord> ocrWords;
		bool valid;
	};

	std::vector<PageData> pageData(totalPages);

	// Load all page images
	for (int i = 0; i < totalPages; ++i) {
		if (progress && progress(i, totalPages)) {
			result.errorDetail = "Cancelled by user";
			return result;
		}

		PageInfo const& info = seq.pageAt(i);
		pageData[i].imgPath = fileNameGen.filePathFor(info.id());
		pageData[i].valid = false;

		QFile tiffFile(pageData[i].imgPath);
		if (!tiffFile.open(QIODevice::ReadOnly)) {
			continue;
		}
		pageData[i].img = TiffReader::readImage(tiffFile);
		tiffFile.close();
		pageData[i].valid = !pageData[i].img.isNull();
	}

	// Parallel OCR (if enabled)
#ifdef HAVE_TESSERACT
	if (opts.enableOcr) {
		QString const tessDataDir = findTessDataDir(opts.tessDataPath, opts.language);
		QByteArray const langBytes = opts.language.toUtf8();
		QByteArray const pathBytes = tessDataDir.toUtf8();

		// Limit OpenMP threads per Tesseract instance to 1 — we control
		// parallelism ourselves with std::thread.
#ifdef _WIN32
		_putenv("OMP_THREAD_LIMIT=1");
#else
		setenv("OMP_THREAD_LIMIT", "1", 1);
#endif

		int const nThreads = qMax(1, static_cast<int>(
			std::thread::hardware_concurrency()));

		std::atomic<int> ocrWordTotal(0);
		std::atomic<bool> tessInitSuccess(false);
		std::mutex progressMutex;
		std::atomic<bool> cancelled(false);

		auto ocrWorker = [&](int threadId) {
			tesseract::TessBaseAPI api;
			char const* dataDir = tessDataDir.isEmpty()
				? nullptr : pathBytes.constData();

			if (api.Init(dataDir, langBytes.constData()) != 0) {
				return; // Init failed for this thread
			}
			api.SetVariable("tessedit_do_invert", "0");
			tessInitSuccess.store(true);

			for (int i = threadId; i < totalPages; i += nThreads) {
				if (cancelled.load()) break;
				if (!pageData[i].valid) continue;

				QImage const& img = pageData[i].img;
				double const actualDpi = (img.dotsPerMeterX() > 0)
					? img.dotsPerMeterX() / 39.3701
					: static_cast<double>(opts.dpi);

				QImage gray = img.convertToFormat(QImage::Format_Grayscale8);

				// Downscale to ~300 DPI for OCR
				double ocrScale = 1.0;
				int ocrDpi = static_cast<int>(actualDpi);
				if (ocrDpi > 350) {
					ocrScale = 300.0 / ocrDpi;
					gray = gray.scaled(
						static_cast<int>(gray.width() * ocrScale),
						static_cast<int>(gray.height() * ocrScale),
						Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
					ocrDpi = 300;
				}

				api.SetImage(gray.constBits(), gray.width(), gray.height(),
					1, gray.bytesPerLine());
				api.SetSourceResolution(ocrDpi > 0 ? ocrDpi : opts.dpi);

				if (api.Recognize(nullptr) == 0) {
					tesseract::ResultIterator* ri = api.GetIterator();
					if (ri) {
						tesseract::PageIteratorLevel const level =
							tesseract::RIL_WORD;
						QList<OcrWord> words;
						do {
							char const* text = ri->GetUTF8Text(level);
							if (!text) continue;
							OcrWord word;
							word.text = QString::fromUtf8(text);
							word.confidence = ri->Confidence(level);
							delete[] text;
							if (word.confidence < 10.0f
								|| word.text.trimmed().isEmpty())
								continue;
							int x1, y1, x2, y2;
							ri->BoundingBox(level, &x1, &y1, &x2, &y2);
							if (ocrScale < 1.0) {
								double const inv = 1.0 / ocrScale;
								x1 = static_cast<int>(x1 * inv);
								y1 = static_cast<int>(y1 * inv);
								x2 = static_cast<int>(x2 * inv);
								y2 = static_cast<int>(y2 * inv);
							}
							word.x = x1;
							word.y = y1;
							word.w = x2 - x1;
							word.h = y2 - y1;
							words.append(word);
						} while (ri->Next(level));
						pageData[i].ocrWords = words;
						ocrWordTotal.fetch_add(words.size());
					}
				}
				api.Clear();

				// Report progress from OCR threads
				if (progress) {
					std::lock_guard<std::mutex> lock(progressMutex);
					if (progress(i, totalPages)) {
						cancelled.store(true);
					}
				}
			}
			api.End();
		};

		// Launch worker threads
		std::vector<std::thread> threads;
		for (int t = 0; t < nThreads; ++t) {
			threads.emplace_back(ocrWorker, t);
		}
		for (auto& t : threads) {
			t.join();
		}

		result.tessInitOk = tessInitSuccess.load();
		result.ocrWordCount = ocrWordTotal.load();

		if (cancelled.load()) {
			result.errorDetail = "Cancelled by user";
			return result;
		}
	}
#else
	if (opts.enableOcr) {
		result.errorDetail = "Tesseract not compiled (HAVE_TESSERACT not defined)";
	}
#endif

	// ══════════════════════════════════════════════════════════
	// Phase 2: Sequential PDF assembly
	// ══════════════════════════════════════════════════════════

	QDir().mkpath(QFileInfo(outputPdfPath).absolutePath());

	QFile file(outputPdfPath);
	if (!file.open(QIODevice::WriteOnly)) {
		result.pageCount = -1;
		result.errorDetail = "Cannot open file: " + outputPdfPath;
		return result;
	}

	// ── PDF object bookkeeping ──
	int nextObjId = 1;
	QList<QPair<int, qint64>> xrefEntries;

	auto writeObj = [&](QByteArray const& content) -> int {
		int id = nextObjId++;
		xrefEntries.append(qMakePair(id, file.pos()));
		file.write(QByteArray::number(id) + " 0 obj\n");
		file.write(content);
		file.write("\nendobj\n");
		return id;
	};

	// ── PDF header ──
	QByteArray pdfHeader = "%PDF-" + opts.pdfVersion.toUtf8() + "\n%\xE2\xE3\xCF\xD3\n";
	file.write(pdfHeader);

	int const fontId = writeObj(
		"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"
	);

	int const pagesObjId = nextObjId++;
	QList<int> pageObjIds;

	// ── Write each page ──
	for (int i = 0; i < totalPages; ++i) {
		if (!pageData[i].valid) continue;

		QImage& img = pageData[i].img;
		QList<OcrWord> const& ocrWords = pageData[i].ocrWords;

		// ── Detect grayscale vs color ──
		QImage::Format const fmt = img.format();
		bool isGray = (fmt == QImage::Format_Grayscale8
		            || fmt == QImage::Format_Mono
		            || fmt == QImage::Format_MonoLSB
		            || fmt == QImage::Format_Indexed8);
		if (!isGray && (fmt == QImage::Format_RGB32 || fmt == QImage::Format_ARGB32)) {
			isGray = true;
			int const step = qMax(1, qMin(img.width(), img.height()) / 30);
			for (int y = 0; y < img.height() && isGray; y += step) {
				QRgb const* line = reinterpret_cast<QRgb const*>(img.constScanLine(y));
				for (int x = 0; x < img.width() && isGray; x += step) {
					QRgb const px = line[x];
					if (qAbs(qRed(px) - qGreen(px)) > 4 || qAbs(qGreen(px) - qBlue(px)) > 4)
						isGray = false;
				}
			}
		}

		double const imgDpi = (img.dotsPerMeterX() > 0)
		    ? img.dotsPerMeterX() / 39.3701
		    : static_cast<double>(opts.dpi);

		int const imgW = img.width();
		int const imgH = img.height();

		// ── Check for foreground TIFF (Mixed mode) ──
		QImage fgImg;
		if (opts.vectorize) {
			QFileInfo fi(pageData[i].imgPath);
			QString fgPath = fi.absolutePath() + "/" + fi.completeBaseName()
			               + "_foreground.tif";
			if (!QFile::exists(fgPath)) {
				fgPath = fi.absolutePath() + "/" + fi.completeBaseName()
				       + "_foreground.tiff";
			}
			if (QFile::exists(fgPath)) {
				QFile fgFile(fgPath);
				if (fgFile.open(QIODevice::ReadOnly)) {
					fgImg = TiffReader::readImage(fgFile);
					fgFile.close();
				}
			}
		}

		bool const is1bit = (fmt == QImage::Format_Mono
		                  || fmt == QImage::Format_MonoLSB);
		bool const useHybrid = opts.vectorize && !fgImg.isNull();
		bool const useVector = opts.vectorize && is1bit && !useHybrid;

		// ── Compute page dimensions ──
		double const imgPageW = (imgW / imgDpi) * 72.0;
		double const imgPageH = (imgH / imgDpi) * 72.0;

		double pageW, pageH;
		double imgScaleX = 1.0, imgScaleY = 1.0;
		double imgOffX = 0.0, imgOffY = 0.0;

		switch (opts.pageFormat) {
			case PAGE_A4:     pageW = 595.28; pageH = 841.89; break;
			case PAGE_A5:     pageW = 419.53; pageH = 595.28; break;
			case PAGE_LETTER: pageW = 612.0;  pageH = 792.0;  break;
			default:          pageW = imgPageW; pageH = imgPageH; break;
		}

		if (opts.pageFormat != PAGE_AUTO) {
			double const scaleX = pageW / imgPageW;
			double const scaleY = pageH / imgPageH;
			double const scale = qMin(scaleX, scaleY);
			double const scaledW = imgPageW * scale;
			double const scaledH = imgPageH * scale;
			imgScaleX = scaledW;
			imgScaleY = scaledH;
			imgOffX = (pageW - scaledW) / 2.0;
			imgOffY = (pageH - scaledH) / 2.0;
		} else {
			imgScaleX = pageW;
			imgScaleY = pageH;
		}

		// ── Try Potrace vectorization ──
		QByteArray potracePaths;
		if (useVector) {
			potracePaths = PotraceVectorizer::traceToPathStream(
				img, imgPageW, imgPageH, static_cast<int>(imgDpi));
		} else if (useHybrid) {
			potracePaths = PotraceVectorizer::traceToPathStream(
				fgImg, imgPageW, imgPageH, static_cast<int>(imgDpi));
		}
		fgImg = QImage();

		bool const hasVectors = !potracePaths.isEmpty();

		// ── Encode raster image ──
		int imgObjId = -1;
		bool needsRaster = !hasVectors || useHybrid;

		// ── Optional downsampling ──
		if (opts.downsample && imgDpi > opts.downsampleThreshold) {
			double const scale = double(opts.dpi) / imgDpi;
			img = img.scaled(
				static_cast<int>(img.width() * scale),
				static_cast<int>(img.height() * scale),
				Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		}

		if (needsRaster) {
			int const encW = img.width();
			int const encH = img.height();
			QByteArray streamData;
			QByteArray imgObj;
			imgObj += "<< /Type /XObject /Subtype /Image";
			imgObj += " /Width " + QByteArray::number(encW);
			imgObj += " /Height " + QByteArray::number(encH);

			// Select compression based on opts.compression
			CompressionMode const mode = opts.compression;

			if (mode == COMPRESS_TEXT_OPTIMAL
				&& (is1bit || (isGray && !useHybrid)))
			{
				// ── Binarize + FlateDecode 1-bit ──
				// ~40-70 KB/page for text. Matches Adobe approach.
				int const rowBytes = (encW + 7) / 8;
				QByteArray rawBits;
				rawBits.resize(rowBytes * encH);

				if (is1bit) {
					QImage monoImg = img.convertToFormat(QImage::Format_Mono);
					for (int y = 0; y < encH; ++y) {
						memcpy(rawBits.data() + y * rowBytes,
						       monoImg.constScanLine(y), rowBytes);
					}
					// Qt Format_Mono: bit0=white, bit1=black.
					// PDF DeviceGray: bit0=black, bit1=white.
					// Invert all bits to fix polarity.
					for (int b = 0; b < rawBits.size(); ++b) {
						rawBits.data()[b] = ~rawBits.data()[b];
					}
				} else {
					// Pre-binarization smoothing to reduce grain.
					// sharpening: 0=max denoise (sigma 1.5), 50=neutral, 100=sharpen
					QImage grayImg = img.convertToFormat(
						QImage::Format_Grayscale8);

					if (opts.sharpening < 50) {
						// Denoise: 3x3 box blur (fast, no dependency).
						// Apply multiple passes for stronger smoothing.
						int passes = (50 - opts.sharpening) / 15 + 1;
						for (int p = 0; p < passes; ++p) {
							QImage blurred(encW, encH, QImage::Format_Grayscale8);
							for (int y = 0; y < encH; ++y) {
								uchar* dst = blurred.scanLine(y);
								for (int x = 0; x < encW; ++x) {
									int sum = 0, count = 0;
									for (int dy = -1; dy <= 1; ++dy) {
										int const ny = y + dy;
										if (ny < 0 || ny >= encH) continue;
										uchar const* row = grayImg.constScanLine(ny);
										for (int dx = -1; dx <= 1; ++dx) {
											int const nx = x + dx;
											if (nx < 0 || nx >= encW) continue;
											sum += row[nx];
											++count;
										}
									}
									dst[x] = static_cast<uchar>(sum / count);
								}
							}
							grayImg = blurred;
						}
					} else if (opts.sharpening > 50) {
						// Sharpen: unsharp mask (original + amount*(original-blur))
						QImage blurred(encW, encH, QImage::Format_Grayscale8);
						for (int y = 0; y < encH; ++y) {
							uchar* dst = blurred.scanLine(y);
							for (int x = 0; x < encW; ++x) {
								int sum = 0, count = 0;
								for (int dy = -1; dy <= 1; ++dy) {
									int const ny = y + dy;
									if (ny < 0 || ny >= encH) continue;
									uchar const* row = grayImg.constScanLine(ny);
									for (int dx = -1; dx <= 1; ++dx) {
										int const nx = x + dx;
										if (nx < 0 || nx >= encW) continue;
										sum += row[nx];
										++count;
									}
								}
								dst[x] = static_cast<uchar>(sum / count);
							}
						}
						double const amount = (opts.sharpening - 50) / 50.0 * 1.5;
						for (int y = 0; y < encH; ++y) {
							uchar const* orig = grayImg.constScanLine(y);
							uchar const* blur = blurred.constScanLine(y);
							uchar* dst = grayImg.scanLine(y);
							for (int x = 0; x < encW; ++x) {
								int val = static_cast<int>(
									orig[x] + amount * (orig[x] - blur[x]));
								dst[x] = static_cast<uchar>(qBound(0, val, 255));
							}
						}
					}

					// PDF DeviceGray 1-bit: 0=black, 1=white.
					// Initialize all bits to 0 (black), set bit=1 for
					// white pixels (>= 128).
					memset(rawBits.data(), 0, rawBits.size());
					for (int y = 0; y < encH; ++y) {
						uchar const* src = grayImg.constScanLine(y);
						uchar* dst = reinterpret_cast<uchar*>(
							rawBits.data() + y * rowBytes);
						for (int x = 0; x < encW; ++x) {
							if (src[x] >= 128) {
								dst[x >> 3] |= (0x80 >> (x & 7));
							}
						}
					}
				}

				streamData = qCompress(rawBits, 9);
				streamData = streamData.mid(4); // strip Qt 4-byte header

				imgObj += " /ColorSpace /DeviceGray";
				imgObj += " /BitsPerComponent 1";
				imgObj += " /Filter /FlateDecode";
				imgObj += " /DecodeParms << /Columns "
				        + QByteArray::number(encW)
				        + " /BitsPerComponent 1 >>";
			}
			else if (mode == COMPRESS_LOSSLESS)
			{
				// ── 8-bit grayscale FlateDecode ──
				QImage grayImg = (isGray || opts.forceGrayscale)
					? img.convertToFormat(QImage::Format_Grayscale8)
					: img.convertToFormat(QImage::Format_RGB888);
				bool const isGrayEnc = (grayImg.format() == QImage::Format_Grayscale8);
				int const bpp = isGrayEnc ? 1 : 3;
				int const rowLen = grayImg.width() * bpp;

				QByteArray rawData;
				rawData.resize(rowLen * encH);
				for (int y = 0; y < encH; ++y) {
					memcpy(rawData.data() + y * rowLen,
					       grayImg.constScanLine(y), rowLen);
				}
				streamData = qCompress(rawData, 9);
				streamData = streamData.mid(4);

				imgObj += isGrayEnc
					? " /ColorSpace /DeviceGray"
					: " /ColorSpace /DeviceRGB";
				imgObj += " /BitsPerComponent 8";
				imgObj += " /Filter /FlateDecode";
			}
			else if (mode == COMPRESS_NONE)
			{
				// ── Uncompressed (debug) ──
				QImage grayImg = img.convertToFormat(QImage::Format_Grayscale8);
				streamData.resize(encW * encH);
				for (int y = 0; y < encH; ++y) {
					memcpy(streamData.data() + y * encW,
					       grayImg.constScanLine(y), encW);
				}
				imgObj += " /ColorSpace /DeviceGray";
				imgObj += " /BitsPerComponent 8";
			}
			else
			{
				// ── JPEG (default fallback + COMPRESS_JPEG) ──
				bool const encGray = (isGray || opts.forceGrayscale);
				QImage encImg = encGray
					? img.convertToFormat(QImage::Format_Grayscale8)
					: img.convertToFormat(QImage::Format_RGB888);
				QBuffer buffer(&streamData);
				buffer.open(QIODevice::WriteOnly);
				encImg.save(&buffer, "JPEG", opts.jpegQuality);
				imgObj += encGray
					? " /ColorSpace /DeviceGray"
					: " /ColorSpace /DeviceRGB";
				imgObj += " /BitsPerComponent 8";
				imgObj += " /Filter /DCTDecode";
			}

			imgObj += " /Length " + QByteArray::number(streamData.size());
			imgObj += " >>\nstream\n";
			imgObj += streamData;
			imgObj += "\nendstream";

			imgObjId = writeObj(imgObj);
		}

		// Release the image now that it's encoded.
		img = QImage();

		// ── Content stream ──
		QByteArray content;

		if (needsRaster && imgObjId >= 0) {
			content += "q\n";
			content += QByteArray::number(imgScaleX, 'f', 2) + " 0 0 "
			         + QByteArray::number(imgScaleY, 'f', 2) + " "
			         + QByteArray::number(imgOffX, 'f', 2) + " "
			         + QByteArray::number(imgOffY, 'f', 2) + " cm\n";
			content += "/Img Do\n";
			content += "Q\n";
		}

		if (hasVectors) {
			if (opts.pageFormat != PAGE_AUTO) {
				double const sx = imgScaleX / imgPageW;
				double const sy = imgScaleY / imgPageH;
				content += "q\n";
				content += QByteArray::number(sx, 'f', 6) + " 0 0 "
				         + QByteArray::number(sy, 'f', 6) + " "
				         + QByteArray::number(imgOffX, 'f', 2) + " "
				         + QByteArray::number(imgOffY, 'f', 2) + " cm\n";
				content += potracePaths;
				content += "Q\n";
			} else {
				content += potracePaths;
			}
		}
		potracePaths.clear();

		// Invisible OCR text overlay
		if (!ocrWords.isEmpty()) {
			double const origW = imgW;
			double const origH = imgH;

			content += "BT\n";
			content += "3 Tr\n";

			for (int w = 0; w < ocrWords.size(); ++w) {
				OcrWord const& word = ocrWords[w];
				if (word.text.isEmpty()) continue;

				double const x = (word.x / origW) * imgScaleX + imgOffX;
				double const y = (imgScaleY + imgOffY)
				               - ((word.y + word.h) / origH) * imgScaleY;
				double fontSize = (word.h / origH) * imgScaleY * 0.9;
				if (fontSize < 1.0) fontSize = 1.0;

				content += "/F1 " + QByteArray::number(fontSize, 'f', 1) + " Tf\n";
				content += "1 0 0 1 "
				         + QByteArray::number(x, 'f', 2) + " "
				         + QByteArray::number(y, 'f', 2) + " Tm\n";

				QByteArray hex = "<FEFF";
				QString const& txt = word.text;
				for (int c = 0; c < txt.size(); ++c) {
					ushort code = txt[c].unicode();
					char buf[5];
					std::snprintf(buf, sizeof(buf), "%04X", code);
					hex += buf;
				}
				hex += ">";
				content += hex + " Tj\n";
			}
			content += "ET\n";
		}

		QByteArray contentObj;
		contentObj += "<< /Length " + QByteArray::number(content.size()) + " >>\n";
		contentObj += "stream\n";
		contentObj += content;
		contentObj += "\nendstream";

		int const contentObjId = writeObj(contentObj);

		// ── Page object ──
		QByteArray pageObj;
		pageObj += "<< /Type /Page";
		pageObj += " /Parent " + QByteArray::number(pagesObjId) + " 0 R";
		pageObj += " /MediaBox [0 0 "
		         + QByteArray::number(pageW, 'f', 2) + " "
		         + QByteArray::number(pageH, 'f', 2) + "]";
		pageObj += " /Contents " + QByteArray::number(contentObjId) + " 0 R";
		pageObj += " /Resources <<";
		if (imgObjId >= 0) {
			pageObj += " /XObject << /Img " + QByteArray::number(imgObjId) + " 0 R >>";
		}
		pageObj += " /Font << /F1 " + QByteArray::number(fontId) + " 0 R >>"
		           " >>";
		pageObj += " >>";

		int const pageObjId = writeObj(pageObj);
		pageObjIds.append(pageObjId);
		++result.pageCount;
	}

	if (pageObjIds.isEmpty()) {
		file.close();
		return result;
	}

	// ── Pages catalog ──
	xrefEntries.append(qMakePair(pagesObjId, file.pos()));
	file.write(QByteArray::number(pagesObjId) + " 0 obj\n");
	QByteArray pagesObj;
	pagesObj += "<< /Type /Pages /Kids [";
	for (int i = 0; i < pageObjIds.size(); ++i) {
		if (i > 0) pagesObj += " ";
		pagesObj += QByteArray::number(pageObjIds[i]) + " 0 R";
	}
	pagesObj += "] /Count " + QByteArray::number(pageObjIds.size()) + " >>";
	file.write(pagesObj);
	file.write("\nendobj\n");

	int const catalogId = writeObj(
		"<< /Type /Catalog /Pages " + QByteArray::number(pagesObjId) + " 0 R >>"
	);

	// ── Cross-reference table ──
	qint64 const xrefOffset = file.pos();

	std::sort(xrefEntries.begin(), xrefEntries.end(),
		[](QPair<int,qint64> const& a, QPair<int,qint64> const& b) {
			return a.first < b.first;
		}
	);

	file.write("xref\n");
	file.write("0 " + QByteArray::number(nextObjId) + "\n");
	file.write("0000000000 65535 f \n");
	for (int i = 0; i < xrefEntries.size(); ++i) {
		char buf[21];
		std::snprintf(buf, sizeof(buf), "%010lld 00000 n \n",
			static_cast<long long>(xrefEntries[i].second));
		file.write(buf, 20);
	}

	file.write("trailer\n");
	file.write("<< /Size " + QByteArray::number(nextObjId)
	         + " /Root " + QByteArray::number(catalogId) + " 0 R >>\n");
	file.write("startxref\n");
	file.write(QByteArray::number(xrefOffset) + "\n");
	file.write("%%EOF\n");

	file.close();

	if (progress) {
		progress(totalPages, totalPages);
	}

	return result;
}
