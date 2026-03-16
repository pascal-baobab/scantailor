/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "VectorPdfDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QString>
#include <QStringList>

VectorPdfDialog::VectorPdfDialog(QString const& defaultOutDir, QWidget* parent)
:	QDialog(parent),
	m_defaultOutDir(defaultOutDir)
{
	setWindowTitle(tr("Vectorize PDF — Searchable Export"));
	setMinimumWidth(520);
	setupUi();
}

void
VectorPdfDialog::setupUi()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// ── OCR Settings ────────────────────────────────────────────────────────
	QGroupBox* optionsGroup = new QGroupBox(tr("OCR Settings"));
	QFormLayout* formLayout = new QFormLayout(optionsGroup);

	// Multi-language list with checkboxes
	m_langList = new QListWidget;
	m_langList->setSelectionMode(QAbstractItemView::NoSelection);
	m_langList->setMaximumHeight(130);

	struct LangEntry { const char* name; const char* code; bool defaultOn; };
	static const LangEntry langs[] = {
		{ "English",             "eng",     true  },
		{ "Italian",             "ita",     false },
		{ "French",              "fra",     false },
		{ "German",              "deu",     false },
		{ "Spanish",             "spa",     false },
		{ "Portuguese",          "por",     false },
		{ "Russian",             "rus",     false },
		{ "Chinese (Simplified)","chi_sim", false },
		{ "Japanese",            "jpn",     false },
		{ "Arabic",              "ara",     false },
	};
	for (auto const& l : langs) {
		QListWidgetItem* item = new QListWidgetItem(tr(l.name));
		item->setData(Qt::UserRole, QString(l.code));
		item->setCheckState(l.defaultOn ? Qt::Checked : Qt::Unchecked);
		m_langList->addItem(item);
	}

	QLabel* langHint = new QLabel(
		tr("<small>Check all languages present in your document. "
		   "More languages = slower OCR.</small>"));
	langHint->setWordWrap(true);
	langHint->setTextFormat(Qt::RichText);

	QVBoxLayout* langLayout = new QVBoxLayout;
	langLayout->setContentsMargins(0, 0, 0, 0);
	langLayout->addWidget(m_langList);
	langLayout->addWidget(langHint);

	QWidget* langWidget = new QWidget;
	langWidget->setLayout(langLayout);
	formLayout->addRow(tr("OCR Language:"), langWidget);

	// ── JPEG Quality ────────────────────────────────────────────────────────
	QHBoxLayout* qualityLayout = new QHBoxLayout;

	m_qualitySlider = new QSlider(Qt::Horizontal);
	m_qualitySlider->setRange(10, 100);
	m_qualitySlider->setValue(65);
	m_qualitySlider->setTickPosition(QSlider::TicksBelow);
	m_qualitySlider->setTickInterval(10);
	qualityLayout->addWidget(m_qualitySlider, 1);

	m_qualitySpin = new QSpinBox;
	m_qualitySpin->setRange(10, 100);
	m_qualitySpin->setValue(65);
	m_qualitySpin->setSuffix("%");
	qualityLayout->addWidget(m_qualitySpin);

	connect(m_qualitySlider, SIGNAL(valueChanged(int)),
		m_qualitySpin, SLOT(setValue(int)));
	connect(m_qualitySpin, SIGNAL(valueChanged(int)),
		m_qualitySlider, SLOT(setValue(int)));
	connect(m_qualitySpin, SIGNAL(valueChanged(int)),
		this, SLOT(updateSizeEstimate()));

	formLayout->addRow(tr("JPEG Quality:"), qualityLayout);

	m_qualityHintLabel = new QLabel(
		tr("<small>Lower = smaller file, higher = better image. "
		   "65 is recommended for scans.</small>"));
	m_qualityHintLabel->setWordWrap(true);
	m_qualityHintLabel->setTextFormat(Qt::RichText);
	formLayout->addRow("", m_qualityHintLabel);

	// ── Resolution ──────────────────────────────────────────────────────────
	m_dpiSpin = new QSpinBox;
	m_dpiSpin->setRange(72, 600);
	m_dpiSpin->setValue(300);
	m_dpiSpin->setSuffix(" dpi");
	connect(m_dpiSpin, SIGNAL(valueChanged(int)),
		this, SLOT(updateSizeEstimate()));
	formLayout->addRow(tr("Resolution:"), m_dpiSpin);

	// ── Size estimate ────────────────────────────────────────────────────────
	m_sizeEstLabel = new QLabel;
	m_sizeEstLabel->setTextFormat(Qt::RichText);
	formLayout->addRow(tr("Est. size:"), m_sizeEstLabel);

	mainLayout->addWidget(optionsGroup);

	// ── Output path ──────────────────────────────────────────────────────────
	QGroupBox* outputGroup = new QGroupBox(tr("Output"));
	QHBoxLayout* pathLayout = new QHBoxLayout(outputGroup);

	m_outputPathEdit = new QLineEdit;
	QString const defaultPdf = QDir(m_defaultOutDir).filePath("PDF/output_searchable.pdf");
	m_outputPathEdit->setText(defaultPdf);
	pathLayout->addWidget(m_outputPathEdit, 1);

	m_browseBtn = new QPushButton(tr("Browse..."));
	connect(m_browseBtn, SIGNAL(clicked()), this, SLOT(browseOutputPath()));
	pathLayout->addWidget(m_browseBtn);

	mainLayout->addWidget(outputGroup);

	// ── Progress ─────────────────────────────────────────────────────────────
	m_progressBar = new QProgressBar;
	m_progressBar->setRange(0, 100);
	m_progressBar->setValue(0);
	m_progressBar->setVisible(false);
	mainLayout->addWidget(m_progressBar);

	m_statusLabel = new QLabel;
	m_statusLabel->setVisible(false);
	mainLayout->addWidget(m_statusLabel);

	// ── Buttons ──────────────────────────────────────────────────────────────
	QHBoxLayout* btnLayout = new QHBoxLayout;
	btnLayout->addStretch();

	m_startBtn = new QPushButton(tr("Export Searchable PDF"));
	m_startBtn->setDefault(true);
	connect(m_startBtn, SIGNAL(clicked()), this, SLOT(startExport()));
	btnLayout->addWidget(m_startBtn);

	m_closeBtn = new QPushButton(tr("Close"));
	connect(m_closeBtn, SIGNAL(clicked()), this, SLOT(reject()));
	btnLayout->addWidget(m_closeBtn);

	mainLayout->addLayout(btnLayout);

	// Populate estimate on first show
	updateSizeEstimate();
}

// ── Getters ─────────────────────────────────────────────────────────────────

QString
VectorPdfDialog::language() const
{
	QStringList codes;
	for (int i = 0; i < m_langList->count(); ++i) {
		QListWidgetItem* item = m_langList->item(i);
		if (item->checkState() == Qt::Checked) {
			codes << item->data(Qt::UserRole).toString();
		}
	}
	if (codes.isEmpty()) {
		return "eng"; // fallback
	}
	return codes.join("+");
}

QString
VectorPdfDialog::outputPath() const
{
	return m_outputPathEdit->text().trimmed();
}

int
VectorPdfDialog::jpegQuality() const
{
	return m_qualitySpin->value();
}

int
VectorPdfDialog::dpi() const
{
	return m_dpiSpin->value();
}

void
VectorPdfDialog::setOutputPath(QString const& path)
{
	m_outputPathEdit->setText(path);
}

// ── Live size estimate ───────────────────────────────────────────────────────

void
VectorPdfDialog::updateSizeEstimate()
{
	double dpiVal = m_dpiSpin->value();
	double q      = m_qualitySpin->value();

	// Estimate JPEG size for an A4 page (8.27 × 11.69 in) at chosen DPI.
	// Rough JPEG compression ratio: cr = 20 - (q-10) * (17.5/90)
	// => q=10 → cr≈20×, q=65 → cr≈9×, q=100 → cr≈2.5×
	double pxW     = 8.27  * dpiVal;
	double pxH     = 11.69 * dpiVal;
	double rawMB   = pxW * pxH * 3.0 / (1024.0 * 1024.0);
	double cr      = 20.0 - (q - 10.0) * (17.5 / 90.0);
	if (cr < 1.0) cr = 1.0;
	double jpegMB  = rawMB / cr + 0.02; // +20 KB overhead per page

	QString est;
	if (jpegMB < 1.0)
		est = tr("~%1 KB / page (A4 estimate)").arg(int(jpegMB * 1024));
	else
		est = tr("~%1 MB / page (A4 estimate)").arg(jpegMB, 0, 'f', 1);

	// Colour hint: green < 1 MB, orange 1–3 MB, red > 3 MB
	QString color = jpegMB < 1.0 ? "#4CAF50" : (jpegMB < 3.0 ? "#FF8C00" : "#CC3333");
	m_sizeEstLabel->setText(
		QString("<small><span style='color:%1'>%2</span></small>").arg(color).arg(est));
}

// ── Slots ────────────────────────────────────────────────────────────────────

void
VectorPdfDialog::browseOutputPath()
{
	QString path = QFileDialog::getSaveFileName(
		this,
		tr("Save Searchable PDF"),
		m_outputPathEdit->text(),
		tr("PDF Files (*.pdf)")
	);
	if (!path.isEmpty()) {
		m_outputPathEdit->setText(path);
	}
}

void
VectorPdfDialog::startExport()
{
	QString const path = outputPath();
	if (path.isEmpty()) {
		QMessageBox::warning(this, tr("Error"),
			tr("Please specify an output file path."));
		return;
	}

	QString const lang = language();

	// Disable controls during export.
	m_startBtn->setEnabled(false);
	m_langList->setEnabled(false);
	m_outputPathEdit->setEnabled(false);
	m_browseBtn->setEnabled(false);
	m_qualitySlider->setEnabled(false);
	m_qualitySpin->setEnabled(false);
	m_dpiSpin->setEnabled(false);
	m_progressBar->setVisible(true);
	m_statusLabel->setVisible(true);
	m_statusLabel->setText(tr("Starting OCR..."));

	emit exportRequested(path, lang, jpegQuality(), dpi());
}

void
VectorPdfDialog::setProgress(int current, int total)
{
	if (total > 0) {
		m_progressBar->setRange(0, total);
		m_progressBar->setValue(current);
		m_statusLabel->setText(
			tr("Processing page %1 of %2...").arg(current + 1).arg(total)
		);
	}
}

void
VectorPdfDialog::exportFinished(int pageCount)
{
	m_startBtn->setEnabled(true);
	m_langList->setEnabled(true);
	m_outputPathEdit->setEnabled(true);
	m_browseBtn->setEnabled(true);
	m_qualitySlider->setEnabled(true);
	m_qualitySpin->setEnabled(true);
	m_dpiSpin->setEnabled(true);
	m_progressBar->setValue(m_progressBar->maximum());

	if (pageCount > 0) {
		m_statusLabel->setText(
			tr("Done! Exported %1 page(s) to:\n%2")
				.arg(pageCount).arg(outputPath())
		);
		QMessageBox::information(this, tr("Vectorize PDF"),
			tr("Searchable PDF exported successfully!\n\n"
			   "%1 page(s) with OCR text overlay.\n"
			   "File: %2").arg(pageCount).arg(outputPath()));
	} else if (pageCount == 0) {
		m_statusLabel->setText(tr("No processed pages found. Run batch processing first."));
		QMessageBox::warning(this, tr("Vectorize PDF"),
			tr("No processed output images found.\n"
			   "Run batch processing first, then export."));
	} else {
		m_statusLabel->setText(tr("Export failed."));
		QMessageBox::critical(this, tr("Vectorize PDF"),
			tr("Failed to create PDF. Check that the output path is writable."));
	}
}
