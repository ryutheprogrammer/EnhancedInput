#pragma once
#include "EIComboBox.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSizePolicy>
#include <QSpinBox>
#include <climits>
#include <functional>

class EIQtInspectorSerializer: public EISerializer
{
public:
	// Match the tree serializer's column-aligned width so inspectors and
	// trigger/modifier parameter rows share one visual rhythm.
	static constexpr int kFieldWidth = 120;

	EIQtInspectorSerializer(QFormLayout *form, std::function<void()> onDirty = {})
		: _form(form)
		, _onDirty(std::move(onDirty))
	{}

	using EISerializer::io;

	void io(const char *name, bool &v) override
	{
		auto *cb = new QCheckBox;
		cb->setChecked(v);
		auto onDirty = _onDirty;
		QObject::connect(cb, &QCheckBox::toggled, [&v, onDirty](bool x) {
			v = x;
			if (onDirty) onDirty();
		});
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
		sp->setFixedWidth(kFieldWidth);
		auto onDirty = _onDirty;
		QObject::connect(sp, qOverload<int>(&QSpinBox::valueChanged),
			[&v, onDirty](int x) {
				v = x;
				if (onDirty) onDirty();
			});
		_form->addRow(name, sp);
	}

	void io(const char *name, float &v) override
	{
		auto *sp = new QDoubleSpinBox;
		sp->setRange(-1e9, 1e9);
		sp->setDecimals(4);
		sp->setSingleStep(0.1);
		sp->setValue(v);
		sp->setFixedWidth(kFieldWidth);
		auto onDirty = _onDirty;
		QObject::connect(sp, qOverload<double>(&QDoubleSpinBox::valueChanged),
			[&v, onDirty](double x) {
				v = static_cast<float>(x);
				if (onDirty) onDirty();
			});
		_form->addRow(name, sp);
	}

	void io(const char *name, Unigine::String &v) override
	{
		auto *le = new QLineEdit;
		le->setText(v.get());
		le->setFixedWidth(kFieldWidth);
		auto onDirty = _onDirty;
		QObject::connect(le, &QLineEdit::textChanged,
			[&v, onDirty](const QString &x) {
				v = x.toUtf8().constData();
				if (onDirty) onDirty();
			});
		_form->addRow(name, le);
	}

	void ioMultiline(const char *name, Unigine::String &v) override
	{
		auto *te = new QPlainTextEdit;
		te->setPlainText(v.get());
		te->setMinimumHeight(80);
		te->setMaximumHeight(160);
		// Allow horizontal stretching — width follows the inspector column,
		// no fixed width like single-line inputs.
		te->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		te->setLineWrapMode(QPlainTextEdit::WidgetWidth);
		auto onDirty = _onDirty;
		QObject::connect(te, &QPlainTextEdit::textChanged, [te, &v, onDirty]() {
			v = te->toPlainText().toUtf8().constData();
			if (onDirty) onDirty();
		});
		_form->addRow(name, te);
	}

protected:
	void ioEnum(const char *name, int &v, const char *const *items, int count) override
	{
		auto *cb = new EIComboBox;
		for (int i = 0; i < count; ++i)
			cb->addItem(items[i]);
		if (v >= 0 && v < count)
			cb->setCurrentIndex(v);
		cb->setFixedWidth(kFieldWidth);
		auto onDirty = _onDirty;
		QObject::connect(cb, qOverload<int>(&QComboBox::currentIndexChanged),
			[&v, onDirty](int x) {
				v = x;
				if (onDirty) onDirty();
			});
		_form->addRow(name, cb);
	}

private:
	QFormLayout *_form;
	std::function<void()> _onDirty;
};
