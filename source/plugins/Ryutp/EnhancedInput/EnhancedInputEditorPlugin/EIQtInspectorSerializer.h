#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <climits>

class EIQtInspectorSerializer: public EISerializer
{
public:
	EIQtInspectorSerializer(QFormLayout *form)
		: _form(form)
	{}

	using EISerializer::io;

	void io(const char *name, bool &v) override
	{
		auto *cb = new QCheckBox;
		cb->setChecked(v);
		QObject::connect(cb, &QCheckBox::toggled, [&v](bool x) { v = x; });
		// Force row height to match other widgets so the checkbox sits
		// vertically centered instead of clinging to the top.
		cb->setMinimumHeight(22);
		_form->addRow(name, cb);
	}

	void io(const char *name, int &v) override
	{
		auto *sp = new QSpinBox;
		sp->setRange(INT_MIN, INT_MAX);
		sp->setValue(v);
		QObject::connect(sp, qOverload<int>(&QSpinBox::valueChanged), [&v](int x) { v = x; });
		_form->addRow(name, sp);
	}

	void io(const char *name, float &v) override
	{
		auto *sp = new QDoubleSpinBox;
		sp->setRange(-1e9, 1e9);
		sp->setDecimals(4);
		sp->setSingleStep(0.1);
		sp->setValue(v);
		QObject::connect(sp, qOverload<double>(&QDoubleSpinBox::valueChanged),
			[&v](double x) { v = static_cast<float>(x); });
		_form->addRow(name, sp);
	}

	void io(const char *name, Unigine::String &v) override
	{
		auto *le = new QLineEdit;
		le->setText(v.get());
		QObject::connect(le, &QLineEdit::textChanged,
			[&v](const QString &x) { v = x.toUtf8().constData(); });
		_form->addRow(name, le);
	}

protected:
	void ioEnum(const char *name, int &v, const char *const *items, int count) override
	{
		auto *cb = new QComboBox;
		for (int i = 0; i < count; ++i)
			cb->addItem(items[i]);
		if (v >= 0 && v < count)
			cb->setCurrentIndex(v);
		QObject::connect(cb, qOverload<int>(&QComboBox::currentIndexChanged),
			[&v](int x) { v = x; });
		_form->addRow(name, cb);
	}

private:
	QFormLayout *_form;
};
