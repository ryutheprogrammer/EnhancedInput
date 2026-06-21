#pragma once
#include <UnigineCurve2d.h>
#include <QStack>
#include <QWidget>

// Editor and preview share one viewport convention: curve coords map 1:1 to
// [-1, 1] x [-1, 1]. Drag clamps to that range; double-click outside the
// inner frame is ignored. Inputs outside [-1, 1] saturate at the curve's
// endpoints via Curve2d's REPEAT_MODE_CLAMP — callers chain a Saturate/Clamp
// upstream if they need finer control there.

class EICurve2dEditor: public QWidget
{
	Q_OBJECT

public:
	explicit EICurve2dEditor(Unigine::Curve2dPtr curve, QWidget *parent = nullptr);

	void undo();
	void redo();

	// One-shot symmetry helpers. Each takes one half of the curve as the
	// source and replaces the other half with the point-mirror of those
	// keys (x → -x, y → -y). Keys exactly at x=0 are left untouched —
	// they're their own mirror.
	void mirrorLeftToRight();
	void mirrorRightToLeft();

signals:
	void curveChanged();

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
	QPointF toWidget(float x, float y) const;
	void fromWidget(const QPointF &pt, float &x, float &y) const;
	int hitTestKey(const QPointF &pos) const;

	void pushUndo(Unigine::Curve2dPtr snapshot);

	Unigine::Curve2dPtr _curve;

	int _dragKey = -1;
	Unigine::Curve2dPtr _dragSnapshot;
	bool _dragMoved = false;

	QStack<Unigine::Curve2dPtr> _undoStack;
	QStack<Unigine::Curve2dPtr> _redoStack;
};

class EICurve2dPreview: public QWidget
{
	Q_OBJECT

public:
	explicit EICurve2dPreview(Unigine::Curve2dPtr curve, QWidget *parent = nullptr);

signals:
	void curveChanged();

protected:
	void paintEvent(QPaintEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;

private:
	Unigine::Curve2dPtr _curve;
};
