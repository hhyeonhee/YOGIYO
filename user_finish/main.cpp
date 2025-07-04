#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont ubuntuFont("DejaVu Sans");
    ubuntuFont.setStyleStrategy(QFont::PreferAntialias); // 부드러운 렌더링
    a.setFont(ubuntuFont);

    MainWindow w;
    w.show();


    return a.exec();
}
