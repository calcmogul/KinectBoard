//=============================================================================
//File Name: TestScreen.hpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#ifndef TEST_SCREEN_HPP
#define TEST_SCREEN_HPP

#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QRect>

#include "Processing.hpp"

class TestScreen : public QMainWindow {
public:
    explicit TestScreen(QRect windowPos = {0, 0, QApplication::desktop()->screenGeometry().width(),
            QApplication::desktop()->screenGeometry().height()}, QWidget* parent = nullptr);

    virtual ~TestScreen();

    void setColor(ProcColor borderColor);

protected:
    void paintEvent(QPaintEvent* event);

private:
    QColor m_outlineColor{255, 0, 0};
    ProcColor m_borderColor = ProcColor::Red;
};

#endif // TEST_SCREEN_HPP
