#include "DarkStyle.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication a(argc, argv);
  QApplication::setApplicationName("Olivia");
  QApplication::setOrganizationName("org.keshavnrj.ubuntu");
  a.setStyle(new DarkStyle);
  MainWindow w;
  w.show();

  return a.exec();
}
