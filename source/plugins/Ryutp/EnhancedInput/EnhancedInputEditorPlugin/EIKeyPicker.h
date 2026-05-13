#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <QWidget>

class QPushButton;

class EIKeyPicker: public QWidget
{
	Q_OBJECT

public:
	EIKeyPicker(QWidget *parent = nullptr);
	~EIKeyPicker() override;

	void setKey(const EIKey &key);
	EIKey key() const { return _key; }

signals:
	void keyChanged(const EIKey &newKey);

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;

private:
	void buildPickMenu();
	void startCapture();
	void stopCapture();
	void updateLabel();

	EIKey qtKeyToEIKey(int qtKey) const;
	EIKey qtMouseButtonToEIKey(int qtButton) const;
	EIKey nativeScanCodeToEIKey(unsigned int scanCode) const;

	EIKey _key;
	QPushButton *_pickButton = nullptr;
	QPushButton *_captureButton = nullptr;
	bool _capturing = false;
};
