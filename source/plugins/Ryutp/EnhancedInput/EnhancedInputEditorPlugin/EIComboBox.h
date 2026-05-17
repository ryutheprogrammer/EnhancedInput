#pragma once
#include <QComboBox>
#include <QWheelEvent>

// QComboBox that ignores the mouse wheel. Default Qt behavior cycles through
// items when the cursor passes over a combo during scroll, which silently
// mutates settings the user didn't mean to touch.
class EIComboBox: public QComboBox
{
public:
	using QComboBox::QComboBox;

protected:
	void wheelEvent(QWheelEvent *e) override { e->ignore(); }
};
