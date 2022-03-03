#include "MainWindow.hpp"
#include <QMessageBox>

typedef QString string;
namespace {
	int getValFromBox(QDoubleSpinBox *box, QSlider *bar) {
		return (bar->maximum() - bar->minimum()) * (box->value() - box->minimum()) / (box->maximum() - box->minimum());
	}

	double getValFromBar(QDoubleSpinBox *box, QSlider *bar) {
		return box->minimum() + (box->maximum() - box->minimum()) * (bar->value() - bar->minimum()) / (bar->maximum() - bar->minimum());
	}

	void saveImageToFile(const QImage &image, const QString &fileName) {
		image.save(fileName);
	}
}// namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	prevSize = this->size();
	ui->rotationSlider->setValue(100 * FractalData::defaultSpeed);
	connectBoxBar();
	setValues();
	updateButtons();
	ui->fractalWidget->setFractalData(&data);
	readAndDraw();
	makeMenu();
	hideBorders();
}

MainWindow::~MainWindow() {
	delete ui;
	delete timer;
	delete elapsedTimer;
	delete temporaryDir;
}

void MainWindow::makeMenu() {
	auto fileMenu = menuBar()->addMenu("File");
	fileMenu->addAction("Load fractal", [&]() { loadFromFile(); });
	fileMenu->addAction("Save fractal", [&]() { saveToFile(); });
	fileMenu->addAction("Save as image", [&]() { saveToImage(); });
	fileMenu->addAction("Exit", [&]() { QApplication::quit(); });

	auto settingsMenu = menuBar()->addMenu("Settings");
	settingsMenu->addAction(
			"Fullscreen view", [&]() { hideAndShow(); }, QKeySequence(tr("F11")));
	auto aboutMenu = menuBar()->addMenu("Help");
	aboutMenu->addAction("About", [&]() {
		QMessageBox aboutBox;
		aboutBox.setIcon(QMessageBox::Information);
		aboutBox.setWindowTitle("About Fractals 3D");
		aboutBox.setTextFormat(Qt::RichText);
		aboutBox.setText("<strong>3D fractals generation</strong>");
		aboutBox.exec();
	});
}

void MainWindow::updateButtons() {
	ui->fractalColorButton->setPalette(QPalette(data.fractalColor));
	ui->fractalColorButton->setText(data.fractalColor.name());
	ui->ambienceColorButton->setPalette(QPalette(data.ambienceColor));
	ui->ambienceColorButton->setText(data.ambienceColor.name());
}

void MainWindow::chooseColor(QColor const &color, ColorType type = FRACTAL) {
	QColor *colorMemory;
	if(type == AMBIENCE)
		colorMemory = &data.ambienceColor;
	else
		colorMemory = &data.fractalColor;
	if(color.isValid()) {
		*colorMemory = color;
		updateButtons();
		readAndDraw();
	}
}

void MainWindow::askColor(ColorType type) {
	QString title;
	if(type == AMBIENCE)
		title = "Select ambience color";
	else
		title = "Select fractal color";
	chooseColor(QColorDialog::getColor(Qt::green, this, title), type);
}

void MainWindow::connectBoxBar() {
	connect(ui->firstCoordBox, &QDoubleSpinBox::valueChanged, ui->firstCoordBar, [&]() { ui->firstCoordBar->setValue(getValFromBox(ui->firstCoordBox, ui->firstCoordBar)); });
	connect(ui->firstCoordBar, &QSlider::valueChanged, ui->firstCoordBox, [&]() { ui->firstCoordBox->setValue(getValFromBar(ui->firstCoordBox, ui->firstCoordBar)); });
	connect(ui->secondCoordBox, &QDoubleSpinBox::valueChanged, ui->secondCoordBar, [&]() { ui->secondCoordBar->setValue(getValFromBox(ui->secondCoordBox, ui->secondCoordBar)); });
	connect(ui->secondCoordBar, &QSlider::valueChanged, ui->secondCoordBox, [&]() { ui->secondCoordBox->setValue(getValFromBar(ui->secondCoordBox, ui->secondCoordBar)); });
	connect(ui->thirdCoordBox, &QDoubleSpinBox::valueChanged, ui->thirdCoordBar, [&]() { ui->thirdCoordBar->setValue(getValFromBox(ui->thirdCoordBox, ui->thirdCoordBar)); });
	connect(ui->thirdCoordBar, &QSlider::valueChanged, ui->thirdCoordBox, [&]() { ui->thirdCoordBox->setValue(getValFromBar(ui->thirdCoordBox, ui->thirdCoordBar)); });
	connect(ui->powerSpinBox, &QSpinBox::valueChanged, ui->powerBarSlider, [&]() { ui->powerBarSlider->setValue(ui->powerSpinBox->value() / 2); });
	connect(ui->powerBarSlider, &QSlider::valueChanged, ui->powerSpinBox, [&]() { ui->powerSpinBox->setValue(2 * ui->powerBarSlider->value()); });
	connect(ui->firstCoordBox, &QDoubleSpinBox::valueChanged, [&]() { readAndDraw(); });
	connect(ui->firstCoordBar, &QSlider::valueChanged, [&]() { readAndDraw(); });
	connect(ui->secondCoordBox, &QDoubleSpinBox::valueChanged, [&]() { readAndDraw(); });
	connect(ui->secondCoordBar, &QSlider::valueChanged, [&]() { readAndDraw(); });
	connect(ui->thirdCoordBox, &QDoubleSpinBox::valueChanged, [&]() { readAndDraw(); });
	connect(ui->thirdCoordBar, &QSlider::valueChanged, [&]() { readAndDraw(); });
	connect(ui->powerSpinBox, &QSpinBox::valueChanged, [&]() { readAndDraw(); });
	connect(ui->powerBarSlider, &QSlider::valueChanged, [&]() { readAndDraw(); });
	connect(ui->typeBox, &QComboBox::currentIndexChanged, [&]() { readAndDraw(); });
	connect(ui->fractalColorButton, &QPushButton::clicked, [&]() { askColor(FRACTAL); });
	connect(ui->ambienceColorButton, &QPushButton::clicked, [&]() { askColor(AMBIENCE); });
	connect(ui->randomizeButton, &QPushButton::clicked, [&]() { generateRandom(); });
	connect(ui->rotationBox, &QCheckBox::stateChanged, [&]() { readAndDraw(); });
	connect(ui->zoomButton, &QPushButton::clicked, [&]() {
		data.setZoomCoefficient();
		ui->fractalWidget->repaint();
	});
	connect(ui->rotationSlider, &QSlider::valueChanged, [&]() {
		ui->rotationBox->setCheckState(Qt::Checked);
		data.setAbsoluteSpeed(ui->rotationSlider->value() / 100.0);
	});
}

void MainWindow::readAndDraw() {
	if(!isSetting) {
		data = FractalData(ui->firstCoordBox->value(), ui->secondCoordBox->value(), ui->thirdCoordBox->value(), ui->powerSpinBox->value(),
						   static_cast<FractalType>(ui->typeBox->currentIndex()), data.fractalColor, data.ambienceColor, data.camera, data.zoomCoefficient, ui->rotationBox->isChecked());
		ui->fractalWidget->repaint();
	}
}

void MainWindow::loadFromFile() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Fractal"), "", tr("3D Fractal Data (*.f3d);;All Files (*)"));
	if(fileName.isEmpty())
		return;
	else {
		QFile file(fileName);

		if(!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information(this, tr("Unable to open file"),
									 file.errorString());
			return;
		}

		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		data.readFrom(doc);
		file.close();

		setValues();
		readAndDraw();
	}
}

void MainWindow::saveToFile() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save fractal configuration"), "", tr("3D Fractal Data (*.f3d);;All files (*.*)"));
	if(!fileName.isEmpty()) {
		QFile file(fileName);

		if(!file.open(QIODevice::WriteOnly)) {
			QMessageBox::information(this, tr("Unable to save the file"), file.errorString());
			return;
		}

		QJsonObject output;
		output.insert("Fractal", data.serialize());

		QJsonDocument doc(output);
		file.write(doc.toJson());
		file.close();
	}
}

void MainWindow::saveToImage() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save fractal image"), "", tr("Image files (*.png *.jpg *.jpeg *.bmp)"));
	if(!fileName.isEmpty()) {
		QFileInfo fileInfo(fileName);
		if(fileInfo.exists() && !fileInfo.isWritable()) {
			QMessageBox::information(this, tr("Unable to write to the file"),
									 "Failed to save fractal data to " + fileInfo.fileName());
			return;
		}
		saveImageToFile(ui->fractalWidget->grabFramebuffer(), fileName);
	}
}

void MainWindow::setValues() {
	isSetting = true;
	ui->firstCoordBox->setValue(data.a);
	ui->secondCoordBox->setValue(data.b);
	ui->thirdCoordBox->setValue(data.c);
	ui->powerSpinBox->setValue(data.n);
	ui->typeBox->setCurrentIndex(data.type);
	updateButtons();
	isSetting = false;
}

void MainWindow::generateRandom() {
	data.genRandom();
	setValues();
	ui->fractalWidget->repaint();
}


void MainWindow::keyPressEvent(QKeyEvent *event) {
	if(event->key() == Qt::Key_F11) {
		hideAndShow();
	}
	if(event->key() == Qt::Key_Escape && isFullScreen) {
		hideAndShow();
	}
	QWidget::keyPressEvent(event);
}

void MainWindow::hideAndShow() {
	if(isFullScreen) {
		ui->menubar->show();
		ui->inputWidget->show();
		ui->statusbar->show();
		this->resize(prevSize);
		isFullScreen = false;
	} else {
		prevSize = this->size();
		ui->menubar->hide();
		ui->inputWidget->hide();
		ui->statusbar->hide();
		this->showMaximized();
		isFullScreen = true;
	}
}

void MainWindow::hideBorders() {
	ui->statusbar->hide();
	ui->centralwidget->setContentsMargins(0, 0, 0, 0);
}
