//=============================================================================
//File Name: TestScreen.cpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#include "TestScreen.hpp"

#include <QPainter>
#include <QPen>

TestScreen::TestScreen(QRect windowPos, QWidget* parent) :
        QMainWindow(parent) {
    setGeometry(windowPos);
    setCursor(QCursor(Qt::BlankCursor));
}

TestScreen::~TestScreen() {
    unsetCursor();
}

void TestScreen::setColor(ProcColor borderColor) {
    if (borderColor == Red) {
        m_outlineColor.setRgb(255, 0, 0);
    }
    else if (borderColor == Green) {
        m_outlineColor.setRgb(0, 255, 0);
    }
    else if (borderColor == Blue) {
        m_outlineColor.setRgb(0, 0, 255);
    }
}

void TestScreen::paintEvent(QPaintEvent* event) {
    (void) event;

    QPainter painter(this);

    // Colored outline
    QPen pen(m_outlineColor);
    pen.setWidth(20);

    // White center
    QBrush brush(QColor(255, 255, 255));

    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(0, 0, size().width(), size().height());
}
