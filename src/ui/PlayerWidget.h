#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include "../core/Player.h"
#include "CardWidget.h"

class PlayerWidget : public QWidget {
    Q_OBJECT
public:
    PlayerWidget(QWidget* parent = nullptr);

    void updatePlayer(const Player* player, bool revealCards = false, bool isCurrentTurn = false);
    void setCountdown(int seconds); // 显示剩余秒数，-1=隐藏
    void startCountdown(int seconds); // 开始倒计时（内部自动每秒更新）
    void resetCountdown(); // 重置计时器
    void stopCountdown(); // 停止计时

    QPoint getCardGlobalPos(int cardIndex) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle(bool isCurrentTurn);
    QLabel* m_nameLabel;
    QLabel* m_chipsLabel;
    QLabel* m_statusLabel;
    QLabel* m_countdownLabel;
    int m_countdownRemaining = 0;
    QTimer* m_countdownTimer;
    QList<CardWidget*> m_cardWidgets;
};

#endif // PLAYERWIDGET_H
