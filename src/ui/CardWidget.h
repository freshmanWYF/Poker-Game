#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>
#include "../core/Card.h"

class CardWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos WRITE move) // 显式支持位置动画

public:
    CardWidget(QWidget* parent = nullptr);

    void setCard(const Card& card);
    void setFaceDown(bool faceDown);
    bool isFaceDown() const { return m_isFaceDown; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Card m_card;
    bool m_isFaceDown;
    QLabel* m_label;
};

#endif // CARDWIDGET_H
