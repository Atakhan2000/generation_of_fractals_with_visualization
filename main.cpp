#include "MainWindow.hpp"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	QApplication fractals3DApp(argc, argv);

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	QSurfaceFormat::setDefaultFormat(format);

	fractals3DApp.setApplicationName("generation 3D fractals");
#ifndef QT_NO_OPENGL
	MainWindow mainWindow{};
	mainWindow.show();
#else
	QLabel note("OpenGL Support required");
	note.show();
#endif

	return fractals3DApp.exec();
}
