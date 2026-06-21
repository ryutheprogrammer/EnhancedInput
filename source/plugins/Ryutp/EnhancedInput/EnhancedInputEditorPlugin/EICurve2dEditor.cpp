#include "EICurve2dEditor.h"
#include <UnigineVector.h>
#include <QDialog>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
// Fixed [-1, 1] × [-1, 1] viewport — matches the natural axis range so
// asymmetric responses are expressible directly. Inputs past [-1, 1]
// saturate via Curve2d's REPEAT_MODE_CLAMP.
constexpr float kXMin = -1.0f, kXMax = 1.0f;
constexpr float kYMin = -1.0f, kYMax = 1.0f;
constexpr int kPadLeft = 36;
constexpr int kPadRight = 10;
constexpr int kPadTop = 10;
constexpr int kPadBottom = 20;
constexpr int kKeyRadius = 4;
constexpr int kKeyHitRadius = 8;
constexpr int kTickCount = 4;       // 5 ticks: -1, -0.5, 0, 0.5, 1
constexpr int kMaxUndoDepth = 200;
constexpr float kMirrorEps = 1e-4f; // tolerance for the x=0 / mirror split
}

EICurve2dEditor::EICurve2dEditor(Unigine::Curve2dPtr curve, QWidget *parent)
	: QWidget(parent)
	, _curve(std::move(curve))
{
	setMinimumSize(380, 260);
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);

	// Window-scoped shortcuts so Ctrl+Z / Ctrl+Y work no matter which child
	// of the editor's dialog (Mirror buttons, Save, …) currently has focus.
	// keyPressEvent on the editor wouldn't fire in those cases.
	auto *undoSc = new QShortcut(QKeySequence::Undo, this);
	undoSc->setContext(Qt::WindowShortcut);
	QObject::connect(undoSc, &QShortcut::activated, this, &EICurve2dEditor::undo);
	auto *redoSc = new QShortcut(QKeySequence::Redo, this);
	redoSc->setContext(Qt::WindowShortcut);
	QObject::connect(redoSc, &QShortcut::activated, this, &EICurve2dEditor::redo);
}

QPointF EICurve2dEditor::toWidget(float x, float y) const
{
	const float w = width() - kPadLeft - kPadRight;
	const float h = height() - kPadTop - kPadBottom;
	return QPointF(
		kPadLeft + (x - kXMin) / (kXMax - kXMin) * w,
		kPadTop + (1.0f - (y - kYMin) / (kYMax - kYMin)) * h);
}

void EICurve2dEditor::fromWidget(const QPointF &pt, float &x, float &y) const
{
	const float w = width() - kPadLeft - kPadRight;
	const float h = height() - kPadTop - kPadBottom;
	x = kXMin + float((pt.x() - kPadLeft) / w) * (kXMax - kXMin);
	y = kYMin + float(1.0 - (pt.y() - kPadTop) / h) * (kYMax - kYMin);
}

int EICurve2dEditor::hitTestKey(const QPointF &pos) const
{
	if (!_curve)
		return -1;
	for (int i = 0; i < _curve->getNumKeys(); ++i)
	{
		const auto p = _curve->getKeyPoint(i);
		const auto wp = toWidget(p.x, p.y);
		const auto dx = pos.x() - wp.x();
		const auto dy = pos.y() - wp.y();
		if (dx * dx + dy * dy <= kKeyHitRadius * kKeyHitRadius)
			return i;
	}
	return -1;
}

void EICurve2dEditor::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), QColor(40, 40, 40));

	// Grid.
	p.setPen(QColor(60, 60, 60));
	for (int i = 0; i <= kTickCount; ++i)
	{
		const float t = float(i) / kTickCount;
		const float vx = kXMin + t * (kXMax - kXMin);
		const float vy = kYMin + t * (kYMax - kYMin);
		p.drawLine(toWidget(vx, kYMin), toWidget(vx, kYMax));
		p.drawLine(toWidget(kXMin, vy), toWidget(kXMax, vy));
	}

	// Center axes (x=0, y=0) — brighter than the grid so they read as axes.
	p.setPen(QColor(95, 95, 95));
	p.drawLine(toWidget(0.0f, kYMin), toWidget(0.0f, kYMax));
	p.drawLine(toWidget(kXMin, 0.0f), toWidget(kXMax, 0.0f));

	// Frame.
	p.setPen(QColor(110, 110, 110));
	p.drawRect(QRectF(toWidget(kXMin, kYMax), toWidget(kXMax, kYMin)));

	// Axis labels.
	QFont labelFont = p.font();
	labelFont.setPointSize(8);
	p.setFont(labelFont);
	p.setPen(QColor(170, 170, 170));
	const QFontMetrics fm(labelFont);
	for (int i = 0; i <= kTickCount; ++i)
	{
		const float t = float(i) / kTickCount;
		const float vx = kXMin + t * (kXMax - kXMin);
		const float vy = kYMin + t * (kYMax - kYMin);
		const QString sx = QString::number(vx, 'g', 2);
		const QString sy = QString::number(vy, 'g', 2);

		const QPointF xp = toWidget(vx, kYMin);
		p.drawText(QPointF(xp.x() - fm.horizontalAdvance(sx) / 2.0,
			xp.y() + fm.ascent() + 3), sx);

		const QPointF yp = toWidget(kXMin, vy);
		p.drawText(QPointF(yp.x() - fm.horizontalAdvance(sy) - 4,
			yp.y() + fm.ascent() / 2.0 - 1), sy);
	}

	if (!_curve || _curve->getNumKeys() == 0)
	{
		p.setPen(QColor(160, 160, 160));
		p.drawText(QRectF(toWidget(kXMin, kYMax), toWidget(kXMax, kYMin)),
			Qt::AlignCenter, "Double-click to add a key");
		return;
	}

	// Curve.
	p.setPen(QPen(QColor(120, 180, 255), 2));
	QPainterPath path;
	const int n = std::max(1, int(width() - kPadLeft - kPadRight));
	for (int i = 0; i <= n; ++i)
	{
		const float x = kXMin + float(i) / n * (kXMax - kXMin);
		const float y = _curve->evaluate(x);
		const auto pt = toWidget(x, y);
		if (i == 0)
			path.moveTo(pt);
		else
			path.lineTo(pt);
	}
	p.drawPath(path);

	// Keys.
	p.setBrush(QColor(255, 220, 100));
	p.setPen(QPen(QColor(0, 0, 0), 1));
	for (int i = 0; i < _curve->getNumKeys(); ++i)
	{
		const auto kp = _curve->getKeyPoint(i);
		p.drawEllipse(toWidget(kp.x, kp.y), kKeyRadius, kKeyRadius);
	}
}

void EICurve2dEditor::mousePressEvent(QMouseEvent *e)
{
	if (!_curve)
		return;
	setFocus(Qt::MouseFocusReason);
	if (e->button() == Qt::LeftButton)
	{
		_dragKey = hitTestKey(e->position());
		if (_dragKey >= 0)
		{
			_dragSnapshot = Unigine::Curve2d::create(_curve);
			_dragMoved = false;
		}
	}
	else if (e->button() == Qt::RightButton)
	{
		const int i = hitTestKey(e->position());
		if (i < 0)
			return;
		// Keep at least one key — evaluating an empty curve is UB and the
		// modifier would silently turn into a no-op anyway.
		if (_curve->getNumKeys() <= 1)
			return;
		auto snap = Unigine::Curve2d::create(_curve);
		_curve->removeKey(i);
		pushUndo(snap);
		emit curveChanged();
		update();
	}
}

void EICurve2dEditor::mouseMoveEvent(QMouseEvent *e)
{
	if (_dragKey < 0 || !_curve)
		return;
	float x, y;
	fromWidget(e->position(), x, y);
	x = std::clamp(x, kXMin, kXMax);
	y = std::clamp(y, kYMin, kYMax);
	_dragKey = _curve->moveKey(_dragKey, Unigine::Math::vec2(x, y));
	_dragMoved = true;
	emit curveChanged();
	update();
}

void EICurve2dEditor::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() != Qt::LeftButton)
		return;
	if (_dragKey >= 0 && _dragMoved && _dragSnapshot)
		pushUndo(_dragSnapshot);
	_dragKey = -1;
	_dragSnapshot = Unigine::Curve2dPtr();
	_dragMoved = false;
}

void EICurve2dEditor::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (!_curve || e->button() != Qt::LeftButton)
		return;
	if (hitTestKey(e->position()) >= 0)
		return;
	const QRectF inner(toWidget(kXMin, kYMax), toWidget(kXMax, kYMin));
	if (!inner.contains(e->position()))
		return;
	float x, y;
	fromWidget(e->position(), x, y);
	x = std::clamp(x, kXMin, kXMax);
	y = std::clamp(y, kYMin, kYMax);
	auto snap = Unigine::Curve2d::create(_curve);
	_curve->addKey(Unigine::Math::vec2(x, y));
	pushUndo(snap);
	emit curveChanged();
	update();
}

void EICurve2dEditor::pushUndo(Unigine::Curve2dPtr snapshot)
{
	_undoStack.push(snapshot);
	_redoStack.clear();
	while (_undoStack.size() > kMaxUndoDepth)
		_undoStack.removeFirst();
}

void EICurve2dEditor::undo()
{
	if (_undoStack.isEmpty() || !_curve)
		return;
	auto snap = _undoStack.pop();
	_redoStack.push(Unigine::Curve2d::create(_curve));
	_curve->copy(snap);
	emit curveChanged();
	update();
}

void EICurve2dEditor::redo()
{
	if (_redoStack.isEmpty() || !_curve)
		return;
	auto snap = _redoStack.pop();
	_undoStack.push(Unigine::Curve2d::create(_curve));
	_curve->copy(snap);
	emit curveChanged();
	update();
}

namespace
{
// Replace one half of the curve with the point-mirror of the other. `sourceSign`
// is the sign of the source half's x; the destination is the opposite side.
// Keys near x=0 are treated as their own mirror and left alone.
void mirrorHalf(Unigine::Curve2dPtr &curve, float sourceSign)
{
	// Pass 1: snapshot the source half by value (indices about to shift).
	Unigine::Vector<Unigine::Math::vec2> source;
	for (int i = 0; i < curve->getNumKeys(); ++i)
	{
		const auto p = curve->getKeyPoint(i);
		if (sourceSign > 0 ? p.x > kMirrorEps : p.x < -kMirrorEps)
			source.append(p);
	}

	// Pass 2: clear the destination half. Iterate high-to-low so removal
	// doesn't shift indices we haven't visited yet.
	for (int i = curve->getNumKeys() - 1; i >= 0; --i)
	{
		const auto p = curve->getKeyPoint(i);
		if (sourceSign > 0 ? p.x < -kMirrorEps : p.x > kMirrorEps)
			curve->removeKey(i);
	}

	// Pass 3: write mirrored source into the destination half.
	for (const auto &p : source)
		curve->addKey(Unigine::Math::vec2(-p.x, -p.y));
}
}

void EICurve2dEditor::mirrorLeftToRight()
{
	if (!_curve || _curve->getNumKeys() == 0)
		return;
	auto snap = Unigine::Curve2d::create(_curve);
	mirrorHalf(_curve, -1.0f);
	pushUndo(snap);
	emit curveChanged();
	update();
}

void EICurve2dEditor::mirrorRightToLeft()
{
	if (!_curve || _curve->getNumKeys() == 0)
		return;
	auto snap = Unigine::Curve2d::create(_curve);
	mirrorHalf(_curve, +1.0f);
	pushUndo(snap);
	emit curveChanged();
	update();
}

EICurve2dPreview::EICurve2dPreview(Unigine::Curve2dPtr curve, QWidget *parent)
	: QWidget(parent)
	, _curve(std::move(curve))
{
	setFixedSize(120, 22);
	setCursor(Qt::PointingHandCursor);
	setToolTip("Click to edit curve");
}

void EICurve2dPreview::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.fillRect(rect(), QColor(50, 50, 50));

	p.setPen(QColor(80, 80, 80));
	p.drawLine(width() / 2, 0, width() / 2, height());
	p.drawLine(0, height() / 2, width(), height() / 2);

	p.setPen(QColor(95, 95, 95));
	p.drawRect(rect().adjusted(0, 0, -1, -1));

	if (!_curve || _curve->getNumKeys() == 0)
	{
		p.setPen(QColor(140, 140, 140));
		p.drawText(rect(), Qt::AlignCenter, "(empty)");
		return;
	}

	p.setPen(QPen(QColor(120, 180, 255), 1.5));
	QPainterPath path;
	const int n = std::max(1, width() - 4);
	for (int i = 0; i <= n; ++i)
	{
		const float x = -1.0f + 2.0f * float(i) / n;
		const float y = _curve->evaluate(x);
		const float yNorm = (y + 1.0f) * 0.5f;
		const QPointF pt(2 + i, height() - 2 - yNorm * (height() - 4));
		if (i == 0)
			path.moveTo(pt);
		else
			path.lineTo(pt);
	}
	p.drawPath(path);
}

void EICurve2dPreview::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() != Qt::LeftButton
		|| !rect().contains(e->position().toPoint()))
		return;
	if (!_curve)
		return;

	auto working = Unigine::Curve2d::create(_curve);

	QDialog dlg(this);
	dlg.setWindowTitle("Curve Editor");
	auto *layout = new QVBoxLayout(&dlg);

	auto *editor = new EICurve2dEditor(working, &dlg);

	auto *header = new QHBoxLayout;
	auto *mirrorLR = new QPushButton("Mirror L → R", &dlg);
	auto *mirrorRL = new QPushButton("Mirror R → L", &dlg);
	mirrorLR->setToolTip("Replace right half with point-mirror of the left half");
	mirrorRL->setToolTip("Replace left half with point-mirror of the right half");
	QObject::connect(mirrorLR, &QPushButton::clicked,
		editor, &EICurve2dEditor::mirrorLeftToRight);
	QObject::connect(mirrorRL, &QPushButton::clicked,
		editor, &EICurve2dEditor::mirrorRightToLeft);
	header->addWidget(mirrorLR);
	header->addWidget(mirrorRL);
	header->addStretch();
	layout->addLayout(header);

	layout->addWidget(editor);

	auto *btnRow = new QHBoxLayout;
	btnRow->addStretch();
	auto *saveBtn = new QPushButton("Save", &dlg);
	auto *cancelBtn = new QPushButton("Cancel", &dlg);
	saveBtn->setDefault(true);
	QObject::connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
	QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
	btnRow->addWidget(saveBtn);
	btnRow->addWidget(cancelBtn);
	layout->addLayout(btnRow);

	QTimer::singleShot(0, editor, qOverload<>(&QWidget::setFocus));

	if (dlg.exec() == QDialog::Accepted)
	{
		_curve->copy(working);
		update();
		emit curveChanged();
	}
}
