
#include "liveness.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Liveness w;
	w.show();
	return a.exec();
}
