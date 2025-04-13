#include <QApplication>
#include "GoldenFlower.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 创建并显示主窗口
    GoldenFlowerWindow window;
    window.show();
    
    // 进入应用程序主循环
    int result = app.exec();
    
    return result;
}