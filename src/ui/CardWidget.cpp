#include "CardWidget.h"
#include <QtGui/QPainter>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QFile>
#include <QtWidgets/QGraphicsDropShadowEffect>

CardWidget::CardWidget(QWidget* parent) : QWidget(parent), m_isFaceDown(true) {
    setFixedSize(76, 106);
    
    // 添加阴影效果
    auto shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 150));
    shadow->setOffset(3, 3);
    setGraphicsEffect(shadow);
}

void CardWidget::setCard(const Card& card) {
    m_card = card;
    m_isFaceDown = false;
    update();
}

void CardWidget::setFaceDown(bool faceDown) {
    m_isFaceDown = faceDown;
    update();
}

void CardWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_isFaceDown) {
        const QRectF cardRect = QRectF(rect()).adjusted(2, 2, -2, -2);
        QLinearGradient gradient(cardRect.topLeft(), cardRect.bottomRight());
        gradient.setColorAt(0, QColor("#24485d"));
        gradient.setColorAt(1, QColor("#102936"));
        painter.setBrush(gradient);
        painter.setPen(QPen(QColor("#d5bc75"), 1.5));
        painter.drawRoundedRect(cardRect, 7, 7);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#55767f"), 1));
        painter.drawRoundedRect(cardRect.adjusted(6, 6, -6, -6), 4, 4);
        painter.setPen(QColor("#e4ce91"));
        QFont font = painter.font();
        font.setPixelSize(34);
        painter.setFont(font);
        painter.drawText(cardRect, Qt::AlignCenter, QStringLiteral("♠"));
        return;
    }
    QString imgPath = m_card.imagePath();
    
    // 尝试加载图片
    QPixmap pixmap(imgPath);

    if (!pixmap.isNull()) {
        // 使用整个矩形区域绘制，图片会自动拉伸填充
        painter.drawPixmap(rect(), pixmap);
    } else {
        // 如果图片加载失败，显示备用文字和红色边框
        painter.setBrush(m_isFaceDown ? Qt::blue : Qt::white);
        painter.setPen(Qt::red);
        painter.drawRoundedRect(rect(), 5, 5);
        painter.drawText(rect(), Qt::AlignCenter, "Missing:\n" + imgPath);
    }
}
