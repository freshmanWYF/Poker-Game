#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QList>
#include <QtCore/QString>
#include "../core/Player.h"
#include "CardWidget.h"

class PlayerWidget : public QWidget {
    Q_OBJECT
public:
    PlayerWidget(QWidget* parent = nullptr);

    void updatePlayer(const Player* player, bool revealCards = false, bool isCurrentTurn = false);

    QPoint getCardGlobalPos(int cardIndex) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle(bool isCurrentTurn);
    QLabel* m_nameLabel;
    QLabel* m_chipsLabel;
    QLabel* m_statusLabel;
    QList<CardWidget*> m_cardWidgets;
};

#endif // PLAYERWIDGET_H
