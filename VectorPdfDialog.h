/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef VECTOR_PDF_DIALOG_H_
#define VECTOR_PDF_DIALOG_H_

#include <QDialog>
#include <QString>
#include <QListWidget>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

/**
 * Dialog for configuring and running vectorized PDF export.
 * Supports multi-language OCR, JPEG quality/DPI tuning with live size estimate.
 */
class VectorPdfDialog : public QDialog
{
	Q_OBJECT
public:
	explicit VectorPdfDialog(QString const& defaultOutDir, QWidget* parent = nullptr);

	/// Returns selected Tesseract language string, e.g. "eng+ita" for multi-language.
	QString language() const;
	QString outputPath() const;
	int jpegQuality() const;
	int dpi() const;

	/// Override the output path (e.g. to set SCNTLR_<input>.pdf).
	void setOutputPath(QString const& path);

signals:
	void exportRequested(QString const& outputPath, QString const& language,
	                     int jpegQuality, int dpi);

public slots:
	void setProgress(int current, int total);
	void exportFinished(int pageCount);

private slots:
	void browseOutputPath();
	void startExport();
	void updateSizeEstimate();

private:
	void setupUi();

	QListWidget*  m_langList;
	QLineEdit*    m_outputPathEdit;
	QPushButton*  m_browseBtn;
	QPushButton*  m_startBtn;
	QPushButton*  m_closeBtn;
	QProgressBar* m_progressBar;
	QLabel*       m_statusLabel;
	QSlider*      m_qualitySlider;
	QSpinBox*     m_qualitySpin;
	QLabel*       m_qualityHintLabel;
	QSpinBox*     m_dpiSpin;
	QLabel*       m_sizeEstLabel;
	QString       m_defaultOutDir;
};

#endif // VECTOR_PDF_DIALOG_H_
