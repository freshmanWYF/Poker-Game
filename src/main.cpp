#include <QtWidgets/QApplication>
#include "ui/MainWindow.h"
#include "core/GameEngine.h"
#include "controller/GameController.h"

/**
 * @brief 炸金花游戏入口
 * 初始化 MVC 架构并启动 Qt 事件循环
 */
int main(int argc, char *argv[]) {
    // 1. 创建 Qt 应用程序对象
    QApplication a(argc, argv);

    // 2. 初始化 Model (Engine)
    GameEngine engine;

    // 3. 初始化 View (MainWindow)
    MainWindow view;

    // 4. 初始化 Controller (连接 Model 与 View)
    GameController controller(&engine, &view);

    // 5. 显示主窗口
    view.show();

    // 6. 进入 Qt 事件循环
    return a.exec();
}
