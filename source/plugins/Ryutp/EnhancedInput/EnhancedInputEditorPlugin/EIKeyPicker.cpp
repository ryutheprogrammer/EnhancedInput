#include "EIKeyPicker.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>

EIKeyPicker::EIKeyPicker(QWidget *parent)
	: QWidget(parent)
{
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);

	_pickButton = new QPushButton(this);
	_pickButton->setMinimumWidth(120);
	layout->addWidget(_pickButton, 1);

	_captureButton = new QPushButton("Capture", this);
	_captureButton->setFixedWidth(72);
	_captureButton->setToolTip("Press any key or mouse button to bind. ESC to cancel.");
	layout->addWidget(_captureButton);

	buildPickMenu();

	connect(_captureButton, &QPushButton::clicked, this, [this]() {
		if (_capturing)
			stopCapture();
		else
			startCapture();
	});

	updateLabel();
}

EIKeyPicker::~EIKeyPicker()
{
	if (_capturing)
		stopCapture();
}

void EIKeyPicker::setKey(const EIKey &key)
{
	if (_key == key)
		return;
	_key = key;
	updateLabel();
	emit keyChanged(_key);
}

void EIKeyPicker::updateLabel()
{
	_pickButton->setText(_key.getName().get());
}

void EIKeyPicker::buildPickMenu()
{
	auto *menu = new QMenu(_pickButton);

	QMenu *catMenu = nullptr;
	const char *currentCat = nullptr;

	const auto &keys = EIKey::getKeys();
	const auto &names = EIKey::getKeysNames();

	for (int i = 0; i < keys.size(); ++i)
	{
		const char *cat = keys[i].getCategoryName();
		if (cat != currentCat)
		{
			currentCat = cat;
			catMenu = cat ? menu->addMenu(cat) : nullptr;
		}
		QMenu *target = catMenu ? catMenu : menu;
		QAction *act = target->addAction(names[i].get());
		EIKey k = keys[i];
		connect(act, &QAction::triggered, this, [this, k]() { setKey(k); });
	}

	_pickButton->setMenu(menu);
}

void EIKeyPicker::startCapture()
{
	_capturing = true;
	_captureButton->setText("…");
	qApp->installEventFilter(this);
}

void EIKeyPicker::stopCapture()
{
	if (!_capturing)
		return;
	_capturing = false;
	_captureButton->setText("Capture");
	qApp->removeEventFilter(this);
}

bool EIKeyPicker::eventFilter(QObject *obj, QEvent *event)
{
	if (!_capturing)
		return QWidget::eventFilter(obj, event);

	switch (event->type())
	{
		case QEvent::KeyPress:
		{
			auto *ke = static_cast<QKeyEvent *>(event);
			if (ke->isAutoRepeat())
				return true;

			if (ke->key() == Qt::Key_Escape)
			{
				stopCapture();
				return true;
			}

			EIKey k = qtKeyToEIKey(ke->key());
			// Fallback for non-Latin layouts: Qt::Key reflects the layout,
			// nativeScanCode is the raw physical position.
			if (k.getPlainValue() == 0)
				k = nativeScanCodeToEIKey(ke->nativeScanCode());

			if (k.getPlainValue() != 0)
			{
				setKey(k);
				stopCapture();
			}
			return true;
		}

		case QEvent::MouseButtonPress:
		{
			// Click anywhere inside this picker cancels capture without binding.
			auto *w = qobject_cast<QWidget *>(obj);
			while (w)
			{
				if (w == this)
				{
					stopCapture();
					return true;
				}
				w = w->parentWidget();
			}

			auto *me = static_cast<QMouseEvent *>(event);
			EIKey k = qtMouseButtonToEIKey(me->button());
			if (k.getPlainValue() != 0)
			{
				setKey(k);
				stopCapture();
			}
			return true;
		}

		default:
			break;
	}

	return QWidget::eventFilter(obj, event);
}

EIKey EIKeyPicker::qtKeyToEIKey(int qtKey) const
{
	if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
	{
		static const EIKey letters[] = {
			Key::A, Key::B, Key::C, Key::D, Key::E, Key::F, Key::G, Key::H, Key::I,
			Key::J, Key::K, Key::L, Key::M, Key::N, Key::O, Key::P, Key::Q, Key::R,
			Key::S, Key::T, Key::U, Key::V, Key::W, Key::X, Key::Y, Key::Z,
		};
		return letters[qtKey - Qt::Key_A];
	}
	if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
	{
		static const EIKey digits[] = {
			Key::D0, Key::D1, Key::D2, Key::D3, Key::D4,
			Key::D5, Key::D6, Key::D7, Key::D8, Key::D9,
		};
		return digits[qtKey - Qt::Key_0];
	}
	if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
	{
		static const EIKey fkeys[] = {
			Key::F1, Key::F2, Key::F3, Key::F4, Key::F5, Key::F6,
			Key::F7, Key::F8, Key::F9, Key::F10, Key::F11, Key::F12,
		};
		return fkeys[qtKey - Qt::Key_F1];
	}

	switch (qtKey)
	{
		case Qt::Key_Space:        return Key::Space;
		case Qt::Key_Return:
		case Qt::Key_Enter:        return Key::Enter;
		case Qt::Key_Tab:          return Key::Tab;
		case Qt::Key_Backspace:    return Key::Backspace;
		case Qt::Key_Shift:        return Key::LeftShift;
		case Qt::Key_Control:      return Key::LeftCtrl;
		case Qt::Key_Alt:          return Key::LeftAlt;
		case Qt::Key_Meta:         return Key::LeftCmd;
		case Qt::Key_CapsLock:     return Key::CapsLock;
		case Qt::Key_NumLock:      return Key::NumLock;
		case Qt::Key_ScrollLock:   return Key::ScrollLock;
		case Qt::Key_Print:        return Key::Printscreen;
		case Qt::Key_Pause:        return Key::Pause;
		case Qt::Key_Menu:         return Key::Menu;
		case Qt::Key_Up:           return Key::Up;
		case Qt::Key_Down:         return Key::Down;
		case Qt::Key_Left:         return Key::Left;
		case Qt::Key_Right:        return Key::Right;
		case Qt::Key_Home:         return Key::Home;
		case Qt::Key_End:          return Key::End;
		case Qt::Key_PageUp:       return Key::Pgup;
		case Qt::Key_PageDown:     return Key::Pgdown;
		case Qt::Key_Insert:       return Key::Insert;
		case Qt::Key_Delete:       return Key::Delete;
		case Qt::Key_QuoteLeft:    return Key::BackQuote;
		case Qt::Key_Minus:        return Key::Minus;
		case Qt::Key_Equal:        return Key::Equals;
		case Qt::Key_BracketLeft:  return Key::LeftBracket;
		case Qt::Key_BracketRight: return Key::RightBracket;
		case Qt::Key_Semicolon:    return Key::Semicolon;
		case Qt::Key_Apostrophe:   return Key::Quote;
		case Qt::Key_Backslash:    return Key::BackSlash;
		case Qt::Key_Less:         return Key::Less;
		case Qt::Key_Comma:        return Key::Comma;
		case Qt::Key_Period:       return Key::Dot;
		case Qt::Key_Slash:        return Key::Slash;
		default:                   return Key::Invalid;
	}
}

EIKey EIKeyPicker::qtMouseButtonToEIKey(int qtButton) const
{
	switch (qtButton)
	{
		case Qt::LeftButton:    return Key::MouseLeft;
		case Qt::RightButton:   return Key::MouseRight;
		case Qt::MiddleButton:  return Key::MouseMiddle;
		case Qt::XButton1:      return Key::MouseAux0;
		case Qt::XButton2:      return Key::MouseAux1;
		default:                return Key::Invalid;
	}
}

EIKey EIKeyPicker::nativeScanCodeToEIKey(unsigned int scanCode) const
{
#if defined(Q_OS_LINUX)
	// X11/Wayland: nativeScanCode is evdev keycode + 8.
	switch (scanCode)
	{
		case 38: return Key::A;
		case 56: return Key::B;
		case 54: return Key::C;
		case 40: return Key::D;
		case 26: return Key::E;
		case 41: return Key::F;
		case 42: return Key::G;
		case 43: return Key::H;
		case 31: return Key::I;
		case 44: return Key::J;
		case 45: return Key::K;
		case 46: return Key::L;
		case 58: return Key::M;
		case 57: return Key::N;
		case 32: return Key::O;
		case 33: return Key::P;
		case 24: return Key::Q;
		case 27: return Key::R;
		case 39: return Key::S;
		case 28: return Key::T;
		case 30: return Key::U;
		case 55: return Key::V;
		case 25: return Key::W;
		case 53: return Key::X;
		case 29: return Key::Y;
		case 52: return Key::Z;
		case 19: return Key::D0;
		case 10: return Key::D1;
		case 11: return Key::D2;
		case 12: return Key::D3;
		case 13: return Key::D4;
		case 14: return Key::D5;
		case 15: return Key::D6;
		case 16: return Key::D7;
		case 17: return Key::D8;
		case 18: return Key::D9;
		case 49: return Key::BackQuote;
		case 20: return Key::Minus;
		case 21: return Key::Equals;
		case 34: return Key::LeftBracket;
		case 35: return Key::RightBracket;
		case 47: return Key::Semicolon;
		case 48: return Key::Quote;
		case 51: return Key::BackSlash;
		case 59: return Key::Comma;
		case 60: return Key::Dot;
		case 61: return Key::Slash;
		default: break;
	}
#elif defined(Q_OS_WIN)
	// Windows: nativeScanCode is PS/2 Set 1 hardware scancode.
	switch (scanCode)
	{
		case 0x1E: return Key::A;
		case 0x30: return Key::B;
		case 0x2E: return Key::C;
		case 0x20: return Key::D;
		case 0x12: return Key::E;
		case 0x21: return Key::F;
		case 0x22: return Key::G;
		case 0x23: return Key::H;
		case 0x17: return Key::I;
		case 0x24: return Key::J;
		case 0x25: return Key::K;
		case 0x26: return Key::L;
		case 0x32: return Key::M;
		case 0x31: return Key::N;
		case 0x18: return Key::O;
		case 0x19: return Key::P;
		case 0x10: return Key::Q;
		case 0x13: return Key::R;
		case 0x1F: return Key::S;
		case 0x14: return Key::T;
		case 0x16: return Key::U;
		case 0x2F: return Key::V;
		case 0x11: return Key::W;
		case 0x2D: return Key::X;
		case 0x15: return Key::Y;
		case 0x2C: return Key::Z;
		case 0x0B: return Key::D0;
		case 0x02: return Key::D1;
		case 0x03: return Key::D2;
		case 0x04: return Key::D3;
		case 0x05: return Key::D4;
		case 0x06: return Key::D5;
		case 0x07: return Key::D6;
		case 0x08: return Key::D7;
		case 0x09: return Key::D8;
		case 0x0A: return Key::D9;
		case 0x29: return Key::BackQuote;
		case 0x0C: return Key::Minus;
		case 0x0D: return Key::Equals;
		case 0x1A: return Key::LeftBracket;
		case 0x1B: return Key::RightBracket;
		case 0x27: return Key::Semicolon;
		case 0x28: return Key::Quote;
		case 0x2B: return Key::BackSlash;
		case 0x33: return Key::Comma;
		case 0x34: return Key::Dot;
		case 0x35: return Key::Slash;
		default:   break;
	}
#else
	(void)scanCode;
#endif
	return Key::Invalid;
}
