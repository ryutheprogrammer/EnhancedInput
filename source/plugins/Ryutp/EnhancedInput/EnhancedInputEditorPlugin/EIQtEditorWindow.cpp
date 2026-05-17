#include "EIQtEditorWindow.h"
#include "EIComboBox.h"
#include "EIQtInspectorSerializer.h"
#include "EIKeyPicker.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <UnigineWindowManager.h>
#include <editor/UnigineAssetManager.h>

using namespace Unigine;

namespace
{

QToolButton *makeIconBtn(const QString &text, const QString &tip = QString())
{
	auto *btn = new QToolButton;
	btn->setText(text);
	btn->setFocusPolicy(Qt::NoFocus);
	btn->setFixedSize(22, 22);
	btn->setCursor(Qt::PointingHandCursor);
	btn->setStyleSheet(
		"QToolButton { background: transparent; border: none; padding: 0px;"
		" color: palette(text); font-size: 18px; font-weight: bold; }"
		"QToolButton:hover { background: transparent; color: palette(highlight); }"
		"QToolButton:pressed { background: transparent; color: palette(highlight); }");
	if (!tip.isEmpty())
		btn->setToolTip(tip);
	return btn;
}


// Serializer that renders each parameter as its own QTreeWidgetItem child
// (instead of stacked QFormLayout rows). Each row gets a [label][value] widget
// via setItemWidget, so the tree's zebra stripes paint per parameter.
class EIQtTreeSerializer: public EISerializer
{
public:
	// Uniform width across spinboxes / comboboxes / line edits in the tree.
	// Checkboxes are exempt (they're tiny squares — fixing their width would
	// just stretch dead space around the indicator).
	static constexpr int kFieldWidth = 120;

	EIQtTreeSerializer(QTreeWidget *tree, QTreeWidgetItem *parent,
		std::function<void()> onDirty = {})
		: _tree(tree)
		, _parent(parent)
		, _onDirty(std::move(onDirty))
	{}

	using EISerializer::io;

	void io(const char *name, bool &v) override
	{
		auto *cb = new QCheckBox;
		cb->setChecked(v);
		cb->setMinimumHeight(22);
		auto onDirty = _onDirty;
		QObject::connect(cb, &QCheckBox::toggled, [&v, onDirty](bool x) {
			v = x;
			if (onDirty) onDirty();
		});
		addParamRow(name, cb);
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
		addParamRow(name, sp);
	}

	void io(const char *name, float &v) override
	{
		auto *sp = new QDoubleSpinBox;
		sp->setRange(-1e9, 1e9);
		sp->setDecimals(3);
		sp->setSingleStep(0.1);
		sp->setValue(v);
		sp->setFixedWidth(kFieldWidth);
		auto onDirty = _onDirty;
		QObject::connect(sp, qOverload<double>(&QDoubleSpinBox::valueChanged),
			[&v, onDirty](double x) {
				v = static_cast<float>(x);
				if (onDirty) onDirty();
			});
		addParamRow(name, sp);
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
		addParamRow(name, le);
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
		addParamRow(name, cb);
	}

private:
	void addParamRow(const char *name, QWidget *valueWidget)
	{
		auto *item = new QTreeWidgetItem();
		_parent->addChild(item);
		item->setFirstColumnSpanned(true);

		auto *row = new QWidget;
		row->setAttribute(Qt::WA_TranslucentBackground);
		auto *l = new QHBoxLayout(row);
		l->setContentsMargins(20, 2, 8, 2);
		l->addWidget(new QLabel(QString("%1:").arg(name)));
		l->addStretch();
		l->addWidget(valueWidget);

		row->adjustSize();
		item->setSizeHint(0, row->sizeHint());
		_tree->setItemWidget(item, 0, row);
	}

	QTreeWidget *_tree;
	QTreeWidgetItem *_parent;
	std::function<void()> _onDirty;
};

QHBoxLayout *makeSectionHeader(const QString &title, QToolButton *&addBtn)
{
	auto *row = new QHBoxLayout;
	row->setContentsMargins(0, 0, 0, 0);
	row->addWidget(new QLabel(QString("<b>%1</b>").arg(title)));
	row->addStretch();
	addBtn = makeIconBtn("+", QString("Create %1").arg(title.toLower()));
	row->addWidget(addBtn);
	return row;
}

// Render a list of trigger/modifier shared_ptr items: flat rows with HLine
// separators, no group box.
template <class T>
void renderCreatorList(QObject *owner, QVBoxLayout *layout, const QString &title,
	Unigine::Vector<std::shared_ptr<T>> &items, EICreatorRegistry<T> *registry,
	std::function<void()> onChanged, std::function<void()> onDirty = {})
{
	// Rebuild synchronously — clearLayout uses deleteLater so the click target
	// (combo/button) stays alive until the event loop unwinds.
	auto defer = onChanged;

	auto *headerRow = new QHBoxLayout;
	headerRow->setContentsMargins(0, 0, 0, 0);
	headerRow->setSpacing(4);

	auto *sectionToggle = new QToolButton;
	sectionToggle->setArrowType(Qt::DownArrow);
	sectionToggle->setAutoRaise(true);
	sectionToggle->setFocusPolicy(Qt::NoFocus);
	sectionToggle->setFixedSize(18, 22);
	headerRow->addWidget(sectionToggle);

	headerRow->addWidget(new QLabel(QString("<b>%1</b>").arg(title)));
	headerRow->addStretch();
	auto *addBtn = makeIconBtn("+", QString("Add %1").arg(title.toLower()));
	QObject::connect(addBtn, &QToolButton::clicked, owner, [&items, defer]() {
		items.append({});
		defer();
	});
	headerRow->addWidget(addBtn);
	layout->addLayout(headerRow);

	if (items.size() == 0)
	{
		sectionToggle->setEnabled(false);
		return;
	}

	auto *tree = new QTreeWidget;
	tree->setHeaderHidden(true);
	tree->setColumnCount(2);
	tree->setAlternatingRowColors(true);
	tree->setRootIsDecorated(true);
	tree->setFrameShape(QFrame::NoFrame);
	tree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	tree->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	// Selection means nothing here — rows are pure parameter containers.
	tree->setSelectionMode(QAbstractItemView::NoSelection);
	tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
	tree->setColumnWidth(1, 32);

	// Section collapse: toggle hides the whole tree.
	QObject::connect(sectionToggle, &QToolButton::clicked, tree, [sectionToggle, tree]() {
		bool visible = !tree->isVisible();
		tree->setVisible(visible);
		sectionToggle->setArrowType(visible ? Qt::DownArrow : Qt::RightArrow);
	});

	for (int i = 0; i < items.size(); ++i)
	{
		auto *headerItem = new QTreeWidgetItem();
		tree->addTopLevelItem(headerItem);

		// Col 0: type combo.
		auto *combo = new EIComboBox;
		combo->addItem("(none)");
		for (int n = 0; n < registry->getCount(); ++n)
			combo->addItem(registry->getName(n));
		int currentIdx = 0;
		if (items[i])
		{
			int byName = registry->getIndex(items[i]->getClassName());
			if (byName >= 0)
				currentIdx = byName + 1;
		}
		combo->setCurrentIndex(currentIdx);
		QObject::connect(combo, qOverload<int>(&QComboBox::activated), owner,
			[&items, i, registry, defer](int idx) {
				if (i < 0 || i >= items.size())
					return;
				items[i].reset(idx == 0 ? nullptr : registry->create(idx - 1));
				defer();
			});
		tree->setItemWidget(headerItem, 0, combo);

		// Col 1: × remove (right-aligned in transparent cell).
		auto *cell = new QWidget;
		cell->setAttribute(Qt::WA_TranslucentBackground);
		cell->setAttribute(Qt::WA_NoSystemBackground);
		cell->setAutoFillBackground(false);
		auto *cellLayout = new QHBoxLayout(cell);
		cellLayout->setContentsMargins(0, 0, 4, 0);
		cellLayout->setSpacing(0);
		cellLayout->addStretch();
		auto *removeBtn = makeIconBtn("×", "Remove");
		cellLayout->addWidget(removeBtn);
		QObject::connect(removeBtn, &QToolButton::clicked, owner, [&items, i, defer]() {
			if (i < 0 || i >= items.size())
				return;
			items.remove(i);
			defer();
		});
		tree->setItemWidget(headerItem, 1, cell);

		// Child items: one row per parameter (label left, value right).
		if (items[i])
		{
			EIQtTreeSerializer s(tree, headerItem, onDirty);
			items[i]->serialize(s);
		}
	}

	// On expand/collapse, ask the tree to recompute its size hint so the
	// outer QScrollArea grows/shrinks accordingly.
	QObject::connect(tree, &QTreeWidget::itemExpanded, owner,
		[tree](QTreeWidgetItem *) { tree->updateGeometry(); });
	QObject::connect(tree, &QTreeWidget::itemCollapsed, owner,
		[tree](QTreeWidgetItem *) { tree->updateGeometry(); });

	// Expand all rows by default — the tree is rebuilt on every change, so
	// leaving params collapsed after a combo change would surprise the user.
	tree->expandAll();

	layout->addWidget(tree);
}

} // namespace

EIQtEditorWindow::EIQtEditorWindow(QWidget *parent)
	: QWidget(parent)
{
	setObjectName("EIQtEditorWindow");

	// Compact combobox padding.
	setStyleSheet(
		"QComboBox { padding: 0px 6px; min-height: 18px; }"
		"QComboBox::drop-down { width: 16px; }");

	auto *root = new QHBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);

	auto *splitter = new QSplitter(Qt::Horizontal, this);
	root->addWidget(splitter);

	// Left pane: contexts list + actions list
	{
		auto *leftWidget = new QWidget;
		auto *leftLayout = new QVBoxLayout(leftWidget);
		leftLayout->setContentsMargins(4, 4, 4, 4);

		QToolButton *ctxAdd = nullptr;
		leftLayout->addLayout(makeSectionHeader("Contexts", ctxAdd));
		_contextFilter = new QLineEdit;
		_contextFilter->setPlaceholderText("Filter…");
		_contextFilter->setClearButtonEnabled(true);
		leftLayout->addWidget(_contextFilter);
		_contextList = new QTreeWidget;
		_contextList->setHeaderHidden(true);
		_contextList->setColumnCount(1);
		_contextList->setRootIsDecorated(false);
		_contextList->setAlternatingRowColors(true);
		leftLayout->addWidget(_contextList, 1);

		QToolButton *actAdd = nullptr;
		leftLayout->addLayout(makeSectionHeader("Actions", actAdd));
		_actionFilter = new QLineEdit;
		_actionFilter->setPlaceholderText("Filter…");
		_actionFilter->setClearButtonEnabled(true);
		leftLayout->addWidget(_actionFilter);
		_actionList = new QTreeWidget;
		_actionList->setHeaderHidden(true);
		_actionList->setColumnCount(1);
		_actionList->setRootIsDecorated(false);
		_actionList->setAlternatingRowColors(true);
		leftLayout->addWidget(_actionList, 1);

		splitter->addWidget(leftWidget);

		connect(ctxAdd, &QToolButton::clicked, this, &EIQtEditorWindow::onCreateContext);
		connect(actAdd, &QToolButton::clicked, this, &EIQtEditorWindow::onCreateAction);
		connect(_contextList, &QTreeWidget::currentItemChanged, this,
			[this](QTreeWidgetItem *cur, QTreeWidgetItem *) {
				int row = cur ? _contextList->indexOfTopLevelItem(cur) : -1;
				onContextSelected(row);
			});
		connect(_actionList, &QTreeWidget::currentItemChanged, this,
			[this](QTreeWidgetItem *cur, QTreeWidgetItem *) {
				int row = cur ? _actionList->indexOfTopLevelItem(cur) : -1;
				onActionSelected(row);
			});
		// Re-click on the already-selected row (currentItemChanged doesn't fire then).
		connect(_contextList, &QTreeWidget::itemClicked, this,
			[this](QTreeWidgetItem *, int) {
				if (_currentContext)
					populateInspectorForContext();
			});
		connect(_actionList, &QTreeWidget::itemClicked, this,
			[this](QTreeWidgetItem *, int) {
				if (_currentAction)
					populateInspectorForAction();
			});
		// Double-click to rename.
		connect(_contextList, &QTreeWidget::itemDoubleClicked, this,
			[this](QTreeWidgetItem *item, int) {
				if (item) onRenameContext(_contextList->indexOfTopLevelItem(item));
			});
		connect(_actionList, &QTreeWidget::itemDoubleClicked, this,
			[this](QTreeWidgetItem *item, int) {
				if (item) onRenameAction(_actionList->indexOfTopLevelItem(item));
			});

		// Filter inputs: substring match (case-insensitive), hide non-matching rows.
		connect(_contextFilter, &QLineEdit::textChanged, this,
			[this](const QString &) { applyContextFilter(); });
		connect(_actionFilter, &QLineEdit::textChanged, this,
			[this](const QString &) { applyActionFilter(); });
	}

	// Middle pane
	{
		_middleWidget = new QWidget;
		_middleLayout = new QVBoxLayout(_middleWidget);
		_middleLayout->setContentsMargins(6, 6, 6, 6);
		_middleLayout->setSpacing(6);
		_middleLayout->addWidget(new QLabel("Select a context or action"), 0, Qt::AlignTop);
		_middleLayout->addStretch();
		splitter->addWidget(_middleWidget);
	}

	// Right pane: scrollable inspector
	{
		auto *rightWidget = new QWidget;
		auto *rightOuter = new QVBoxLayout(rightWidget);
		rightOuter->setContentsMargins(0, 0, 0, 0);

		auto *scroll = new QScrollArea;
		scroll->setWidgetResizable(true);
		scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		auto *inner = new QWidget;
		_rightLayout = new QVBoxLayout(inner);
		_rightLayout->setContentsMargins(8, 6, 8, 6);
		_rightLayout->setSpacing(8);
		_rightLayout->addWidget(new QLabel("<b>Inspector</b>"), 0, Qt::AlignTop);
		_rightLayout->addStretch();

		scroll->setWidget(inner);
		rightOuter->addWidget(scroll);

		splitter->addWidget(rightWidget);
	}

	splitter->setSizes({200, 400, 300});

	refreshContexts();
	refreshActions();
}

EIQtEditorWindow::~EIQtEditorWindow()
{
	if (_currentContext)
		EISystem::get()->getContextRegistry()->destroy(_currentContext);
	// _currentAction is cached by the registry; do not destroy.
}

namespace
{
QWidget *makeAssetRowWidget(const QString &name, QToolButton **outDelBtn)
{
	auto *composite = new QWidget;
	composite->setAttribute(Qt::WA_TranslucentBackground);
	composite->setAttribute(Qt::WA_NoSystemBackground);
	composite->setAutoFillBackground(false);
	auto *layout = new QHBoxLayout(composite);
	layout->setContentsMargins(4, 0, 4, 0);
	layout->setSpacing(4);
	layout->addWidget(new QLabel(name), 1);
	*outDelBtn = makeIconBtn("×", "Delete");
	layout->addWidget(*outDelBtn);
	return composite;
}
} // namespace

void EIQtEditorWindow::refreshContexts()
{
	_contextList->blockSignals(true);
	_contextList->clear();
	auto *registry = EISystem::get()->getContextRegistry();
	for (int i = 0; i < registry->getCount(); ++i)
	{
		const QString name = registry->getName(i);
		auto *item = new QTreeWidgetItem(QStringList(name));
		_contextList->addTopLevelItem(item);
		QToolButton *delBtn = nullptr;
		auto *row = makeAssetRowWidget(name, &delBtn);
		_contextList->setItemWidget(item, 0, row);
		connect(delBtn, &QToolButton::clicked, this, [this, i]() { onDeleteContext(i); });
	}
	_contextList->blockSignals(false);
	applyContextFilter();
	// Row labels are fresh; re-stamp the dirty mark if the active context
	// is still pending unsaved changes.
	if (_dirty && _currentContext)
		if (auto *lbl = getActiveAssetLabel())
			if (!lbl->text().endsWith(" *"))
				lbl->setText(lbl->text() + " *");
}

void EIQtEditorWindow::refreshActions()
{
	_actionList->blockSignals(true);
	_actionList->clear();
	auto *registry = EISystem::get()->getActionRegistry();
	for (int i = 0; i < registry->getCount(); ++i)
	{
		const QString name = registry->getName(i);
		auto *item = new QTreeWidgetItem(QStringList(name));
		_actionList->addTopLevelItem(item);
		QToolButton *delBtn = nullptr;
		auto *row = makeAssetRowWidget(name, &delBtn);
		_actionList->setItemWidget(item, 0, row);
		connect(delBtn, &QToolButton::clicked, this, [this, i]() { onDeleteAction(i); });
	}
	_actionList->blockSignals(false);
	applyActionFilter();
	if (_dirty && _currentAction)
		if (auto *lbl = getActiveAssetLabel())
			if (!lbl->text().endsWith(" *"))
				lbl->setText(lbl->text() + " *");
}

void EIQtEditorWindow::applyContextFilter()
{
	const QString text = _contextFilter ? _contextFilter->text() : QString();
	for (int i = 0; i < _contextList->topLevelItemCount(); ++i)
	{
		auto *item = _contextList->topLevelItem(i);
		bool match = text.isEmpty() || item->text(0).contains(text, Qt::CaseInsensitive);
		item->setHidden(!match);
	}
}

void EIQtEditorWindow::applyActionFilter()
{
	const QString text = _actionFilter ? _actionFilter->text() : QString();
	for (int i = 0; i < _actionList->topLevelItemCount(); ++i)
	{
		auto *item = _actionList->topLevelItem(i);
		bool match = text.isEmpty() || item->text(0).contains(text, Qt::CaseInsensitive);
		item->setHidden(!match);
	}
}

void EIQtEditorWindow::onCreateContext()
{
	auto *registry = EISystem::get()->getContextRegistry();
	const char *ext = registry->getExtension();

	String path = WindowManager::dialogSaveFile("", ext);
	if (path == "")
		return;
	if (path.extension() != ext)
		path = String::format("%s.%s", path.get(), ext);

	if (registry->saveDummy(path))
		refreshContexts();
}

void EIQtEditorWindow::onCreateAction()
{
	auto *registry = EISystem::get()->getActionRegistry();
	const char *ext = registry->getExtension();

	String path = WindowManager::dialogSaveFile("", ext);
	if (path == "")
		return;
	if (path.extension() != ext)
		path = String::format("%s.%s", path.get(), ext);

	if (registry->saveDummy(path))
		refreshActions();
}

void EIQtEditorWindow::onDeleteContext(int row)
{
	auto *registry = EISystem::get()->getContextRegistry();
	if (row < 0 || row >= registry->getCount())
		return;
	String path = registry->getPath(row);
	String name = registry->getName(row);

	if (QMessageBox::question(this, "Delete context",
			QString("Delete context \"%1\"?").arg(name.get()),
			QMessageBox::Yes | QMessageBox::No)
		!= QMessageBox::Yes)
		return;

	if (!UnigineEditor::AssetManager::removeAssetSync(path.get()))
		return;

	if (_currentContext)
		releaseCurrent();
	registry->refresh();
	refreshContexts();
}

void EIQtEditorWindow::onDeleteAction(int row)
{
	auto *registry = EISystem::get()->getActionRegistry();
	if (row < 0 || row >= registry->getCount())
		return;
	String path = registry->getPath(row);
	String name = registry->getName(row);

	if (QMessageBox::question(this, "Delete action",
			QString("Delete action \"%1\"?").arg(name.get()),
			QMessageBox::Yes | QMessageBox::No)
		!= QMessageBox::Yes)
		return;

	if (!UnigineEditor::AssetManager::removeAssetSync(path.get()))
		return;

	if (_currentAction)
		releaseCurrent();
	registry->refresh();
	refreshActions();
}

void EIQtEditorWindow::onRenameContext(int row)
{
	auto *registry = EISystem::get()->getContextRegistry();
	if (row < 0 || row >= registry->getCount())
		return;
	String path = registry->getPath(row);
	String oldName = registry->getName(row);

	bool ok = false;
	QString newName = QInputDialog::getText(this, "Rename context",
		"New name:", QLineEdit::Normal, oldName.get(), &ok);
	if (!ok)
		return;
	auto trimmed = newName.trimmed();
	if (trimmed.isEmpty() || trimmed == oldName.get())
		return;

	if (!UnigineEditor::AssetManager::renameAssetSync(path.get(), trimmed.toUtf8().constData()))
		return;

	if (_currentContext)
		releaseCurrent();
	registry->refresh();
	refreshContexts();
}

void EIQtEditorWindow::onRenameAction(int row)
{
	auto *registry = EISystem::get()->getActionRegistry();
	if (row < 0 || row >= registry->getCount())
		return;
	String path = registry->getPath(row);
	String oldName = registry->getName(row);

	bool ok = false;
	QString newName = QInputDialog::getText(this, "Rename action",
		"New name:", QLineEdit::Normal, oldName.get(), &ok);
	if (!ok)
		return;
	auto trimmed = newName.trimmed();
	if (trimmed.isEmpty() || trimmed == oldName.get())
		return;

	if (!UnigineEditor::AssetManager::renameAssetSync(path.get(), trimmed.toUtf8().constData()))
		return;

	if (_currentAction)
		releaseCurrent();
	registry->refresh();
	refreshActions();
}

void EIQtEditorWindow::onContextSelected(int row)
{
	// Same row re-selected — nothing to do.
	if (row == _currentContextRow && _currentContext)
		return;

	if (!promptUnsavedChanges())
	{
		// Defer the revert — Qt is still mid-selection update inside its own
		// currentItemChanged plumbing, so a synchronous setCurrentItem here
		// gets overwritten when the click handler unwinds. Bouncing back via
		// QTimer::singleShot(0) lets the selection settle on the new row, then
		// we snap it back.
		int prev = _currentContextRow;
		QTimer::singleShot(0, this, [this, prev]() {
			_contextList->blockSignals(true);
			_contextList->setCurrentItem(prev >= 0
				? _contextList->topLevelItem(prev) : nullptr);
			_contextList->blockSignals(false);
		});
		return;
	}

	releaseCurrent();
	if (row < 0)
		return;

	_actionList->blockSignals(true);
	_actionList->setCurrentItem(nullptr);
	_actionList->blockSignals(false);

	auto *registry = EISystem::get()->getContextRegistry();
	_currentContext = registry->create(row);
	if (!_currentContext)
		return;
	_currentContextRow = row;
	clearDirty();

	if (_middleWidget) _middleWidget->show();
	populateMiddleForContext();
	populateInspectorForContext();
}

void EIQtEditorWindow::onActionSelected(int row)
{
	if (row == _currentActionRow && _currentAction)
		return;

	if (!promptUnsavedChanges())
	{
		int prev = _currentActionRow;
		QTimer::singleShot(0, this, [this, prev]() {
			_actionList->blockSignals(true);
			_actionList->setCurrentItem(prev >= 0
				? _actionList->topLevelItem(prev) : nullptr);
			_actionList->blockSignals(false);
		});
		return;
	}

	releaseCurrent();
	if (row < 0)
		return;

	_contextList->blockSignals(true);
	_contextList->setCurrentItem(nullptr);
	_contextList->blockSignals(false);

	auto *registry = EISystem::get()->getActionRegistry();
	_currentAction = registry->create(row);
	if (!_currentAction)
		return;
	_currentActionRow = row;
	clearDirty();

	// Actions edit entirely in the inspector — hide the middle pane.
	if (_middleWidget) _middleWidget->hide();
	populateInspectorForAction();
}

void EIQtEditorWindow::populateInspectorForContext()
{
	if (!_currentContext)
		return;

	auto *box = _rightLayout->parentWidget();
	if (box) box->setUpdatesEnabled(false);
	clearLayout(_rightLayout);
	_rightLayout->addWidget(createInspectorTitle("Inspector"), 0, Qt::AlignTop);

	auto *form = new QFormLayout;
	form->setContentsMargins(0, 0, 0, 0);
	form->addRow("Name", new QLabel(_currentContext->name.get()));
	EIQtInspectorSerializer s(form, [this] { markDirty(); });
	s.ioMultiline("Description", _currentContext->description);
	s.io("Auto register", _currentContext->autoRegistration);
	_rightLayout->addLayout(form);

	_rightLayout->addStretch();
	addSaveDiscardRow();

	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::populateInspectorForAction()
{
	if (!_currentAction)
		return;

	auto *box = _rightLayout->parentWidget();
	if (box) box->setUpdatesEnabled(false);
	clearLayout(_rightLayout);
	_rightLayout->addWidget(createInspectorTitle("Inspector"), 0, Qt::AlignTop);

	auto *form = new QFormLayout;
	form->setContentsMargins(0, 0, 0, 0);
	form->addRow("Name", new QLabel(_currentAction->name.get()));
	EIQtInspectorSerializer s(form, [this] { markDirty(); });
	s.ioMultiline("Description", _currentAction->description);
	s.io("Value type", _currentAction->valueType);
	s.io("Accumulation", _currentAction->accumulationBehavior);
	_rightLayout->addLayout(form);

	auto onChanged = [this]() {
		markDirty();
		populateInspectorForAction();
	};

	auto onDirty = [this] { markDirty(); };
	renderCreatorList<EIModifier>(this, _rightLayout, "Modifiers", _currentAction->modifiers,
		EISystem::get()->getModifierRegistry(), onChanged, onDirty);
	renderCreatorList<EITrigger>(this, _rightLayout, "Triggers", _currentAction->triggers,
		EISystem::get()->getTriggerRegistry(), onChanged, onDirty);

	_rightLayout->addStretch();
	addSaveDiscardRow();

	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::populateInspectorForActionEntry()
{
	if (!_currentContext || _currentActionIdx < 0)
		return;
	auto &actions = _currentContext->getActionMappings();
	if (_currentActionIdx >= actions.size())
		return;
	auto &entry = actions[_currentActionIdx];

	auto *box = _rightLayout->parentWidget();
	if (box) box->setUpdatesEnabled(false);
	clearLayout(_rightLayout);
	_rightLayout->addWidget(createInspectorTitle("Action"), 0, Qt::AlignTop);

	auto *form = new QFormLayout;
	form->setContentsMargins(0, 0, 0, 0);

	auto *reg = EISystem::get()->getActionRegistry();
	auto *combo = new EIComboBox;
	combo->addItem("(none)");
	for (int i = 0; i < reg->getCount(); ++i)
		combo->addItem(reg->getName(i));
	int cur = 0;
	if (entry.action)
	{
		int byName = reg->getIndexByName(entry.action->name);
		if (byName >= 0)
			cur = byName + 1;
	}
	combo->setCurrentIndex(cur);
	const int ai = _currentActionIdx;
	connect(combo, qOverload<int>(&QComboBox::activated), this,
		[this, ai](int newIdx) {
			if (!_currentContext)
				return;
			auto &as = _currentContext->getActionMappings();
			if (ai < 0 || ai >= as.size())
				return;
			auto *r = EISystem::get()->getActionRegistry();
			as[ai].action = newIdx == 0 ? nullptr : r->create(newIdx - 1);
			// Reflect the change in the tree row label without a full rebuild.
			if (_tree)
			{
				if (auto *row = _tree->topLevelItem(ai))
					row->setText(0, as[ai].action
						? as[ai].action->name.get() : "Invalid");
			}
			markDirty();
		});
	form->addRow("Action", combo);

	EIQtInspectorSerializer s(form, [this] { markDirty(); });
	s.ioMultiline("Description", entry.description);
	_rightLayout->addLayout(form);

	_rightLayout->addStretch();
	addSaveDiscardRow();

	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::populateMiddleForContext()
{
	if (!_currentContext)
		return;

	auto *box = _middleLayout->parentWidget();
	if (box) box->setUpdatesEnabled(false);
	clearLayout(_middleLayout);

	{
		auto *headerRow = new QHBoxLayout;
		headerRow->setContentsMargins(0, 0, 0, 0);
		headerRow->addWidget(new QLabel("<b>Action Mappings</b>"));
		headerRow->addStretch();
		auto *addEntryBtn = makeIconBtn("+", "Add action mapping");
		connect(addEntryBtn, &QToolButton::clicked, this, [this]() {
			if (!_currentContext)
				return;
			_currentContext->getActionMappings().append({});
			markDirty();
			refreshContextView();
		});
		headerRow->addWidget(addEntryBtn);
		_middleLayout->addLayout(headerRow);
	}

	_tree = new QTreeWidget;
	_tree->setHeaderHidden(true);
	_tree->setColumnCount(2);
	_tree->setAlternatingRowColors(true);
	_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
	_tree->setColumnWidth(1, 56);

	auto &allActions = _currentContext->getActionMappings();

	for (int ai = 0; ai < allActions.size(); ++ai)
	{
		auto &entry = allActions[ai];

		auto *actionItem = new QTreeWidgetItem();
		_tree->addTopLevelItem(actionItem);

		for (int mi = 0; mi < entry.mappings.size(); ++mi)
		{
			auto &mapping = entry.mappings[mi];

			// Combined header label "A + B + C" from all bindings.
			QString combined;
			for (int bi = 0; bi < mapping.bindings.size(); ++bi)
			{
				if (bi > 0)
					combined += " + ";
				combined += mapping.bindings[bi].key.getName().get();
			}

			auto *mappingItem = new QTreeWidgetItem(QStringList(combined));
			actionItem->addChild(mappingItem);

			// Mapping row col 1: + (add binding) and × (remove mapping).
			auto *cell = new QWidget;
			cell->setAttribute(Qt::WA_TranslucentBackground);
			cell->setAttribute(Qt::WA_NoSystemBackground);
			cell->setAutoFillBackground(false);
			auto *cellLayout = new QHBoxLayout(cell);
			cellLayout->setContentsMargins(0, 0, 4, 0);
			cellLayout->setSpacing(2);
			cellLayout->addStretch();

			auto *addBindingBtn = makeIconBtn("+", "Add binding");
			cellLayout->addWidget(addBindingBtn);
			connect(addBindingBtn, &QToolButton::clicked, this, [this, ai, mi]() {
				if (!_currentContext)
					return;
				auto &actions = _currentContext->getActionMappings();
				if (ai < 0 || ai >= actions.size() || mi < 0 || mi >= actions[ai].mappings.size())
					return;
				auto &bindings = actions[ai].mappings[mi].bindings;
				bindings.append({});
				if (auto *t = EISystem::get()->getTriggerRegistry()->create("Down"))
					bindings.last().triggers.append(SPtr<EITrigger>(t));
				markDirty();
				refreshContextView();
			});

			auto *removeMappingBtn = makeIconBtn("×", "Remove mapping");
			cellLayout->addWidget(removeMappingBtn);
			connect(removeMappingBtn, &QToolButton::clicked, this, [this, ai, mi]() {
				if (!_currentContext)
					return;
				auto &actions = _currentContext->getActionMappings();
				if (ai < 0 || ai >= actions.size() || mi < 0 || mi >= actions[ai].mappings.size())
					return;
				actions[ai].mappings.remove(mi);
				if (_currentActionIdx == ai)
				{
					if (_currentMappingIdx == mi)
					{
						_currentActionIdx = -1;
						_currentMappingIdx = -1;
						_currentBindingIdx = -1;
					}
					else if (_currentMappingIdx > mi)
					{
						--_currentMappingIdx;
					}
				}
				markDirty();
				refreshContextView();
			});
			_tree->setItemWidget(mappingItem, 1, cell);

			// Single-binding mapping == regular binding: no children, the mapping
			// row itself represents the binding (its label is the key name).
			if (mapping.bindings.size() <= 1)
				continue;

			// AND mapping: list each binding as a child row.
			for (int bi = 0; bi < mapping.bindings.size(); ++bi)
			{
				auto &binding = mapping.bindings[bi];
				auto *bindingItem = new QTreeWidgetItem(QStringList(binding.key.getName().get()));
				mappingItem->addChild(bindingItem);

				auto *bCell = new QWidget;
				bCell->setAttribute(Qt::WA_TranslucentBackground);
				bCell->setAttribute(Qt::WA_NoSystemBackground);
				bCell->setAutoFillBackground(false);
				auto *bCellLayout = new QHBoxLayout(bCell);
				bCellLayout->setContentsMargins(0, 0, 4, 0);
				bCellLayout->setSpacing(2);
				bCellLayout->addStretch();
				auto *removeBindingBtn = makeIconBtn("×", "Remove binding");
				bCellLayout->addWidget(removeBindingBtn);
				connect(removeBindingBtn, &QToolButton::clicked, this, [this, ai, mi, bi]() {
					if (!_currentContext)
						return;
					auto &actions = _currentContext->getActionMappings();
					if (ai < 0 || ai >= actions.size()
						|| mi < 0 || mi >= actions[ai].mappings.size()
						|| bi < 0 || bi >= actions[ai].mappings[mi].bindings.size())
						return;
					actions[ai].mappings[mi].bindings.remove(bi);
					if (_currentActionIdx == ai && _currentMappingIdx == mi)
					{
						if (_currentBindingIdx == bi)
							_currentBindingIdx = -1; // fall back to mapping row
						else if (_currentBindingIdx > bi)
							--_currentBindingIdx;
					}
					markDirty();
					refreshContextView();
				});
				_tree->setItemWidget(bindingItem, 1, bCell);
			}
		}

		// Column 0: plain action name (or "Invalid" if unset). The picker lives
		// in the inspector — keeping the row widget-free lets clicks bubble.
		actionItem->setText(0,
			entry.action ? entry.action->name.get() : "Invalid");

		// Column 1: + (add mapping) and ✕ (remove action) buttons, right-aligned.
		auto *cell = new QWidget;
		cell->setAttribute(Qt::WA_TranslucentBackground);
		cell->setAttribute(Qt::WA_NoSystemBackground);
		cell->setAutoFillBackground(false);
		auto *cellLayout = new QHBoxLayout(cell);
		cellLayout->setContentsMargins(0, 0, 4, 0);
		cellLayout->setSpacing(2);
		cellLayout->addStretch();

		auto *addMappingBtn = makeIconBtn("+", "Add mapping");
		cellLayout->addWidget(addMappingBtn);
		connect(addMappingBtn, &QToolButton::clicked, this, [this, ai]() {
			if (!_currentContext)
				return;
			auto &actions = _currentContext->getActionMappings();
			if (ai < 0 || ai >= actions.size())
				return;
			actions[ai].mappings.append({});
			// Seed the new mapping with one default binding + Down trigger.
			auto &lastMapping = actions[ai].mappings.last();
			lastMapping.bindings.append({});
			if (auto *t = EISystem::get()->getTriggerRegistry()->create("Down"))
				lastMapping.bindings.last().triggers.append(SPtr<EITrigger>(t));
			markDirty();
			refreshContextView();
		});

		auto *removeActionBtn = makeIconBtn("×", "Remove action mapping");
		cellLayout->addWidget(removeActionBtn);
		connect(removeActionBtn, &QToolButton::clicked, this, [this, ai]() {
			if (!_currentContext)
				return;
			auto &actions = _currentContext->getActionMappings();
			if (ai < 0 || ai >= actions.size())
				return;
			actions.remove(ai);
			if (_currentActionIdx == ai)
			{
				_currentActionIdx = -1;
				_currentMappingIdx = -1;
				_currentBindingIdx = -1;
			}
			else if (_currentActionIdx > ai)
			{
				--_currentActionIdx;
			}
			markDirty();
			refreshContextView();
		});

		_tree->setItemWidget(actionItem, 1, cell);
	}
	_tree->expandAll();

	_middleLayout->addWidget(_tree, 1);

	connect(_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
		if (!item)
			return;
		auto *parent = item->parent();
		if (!parent)
		{
			// Top-level action-entry row — inspector shows the entry description.
			int ai = _tree->indexOfTopLevelItem(item);
			if (ai < 0)
				return;
			_currentActionIdx = ai;
			_currentMappingIdx = -1;
			_currentBindingIdx = -1;
			_currentMapping = nullptr;
			populateInspectorForActionEntry();
			return;
		}
		auto *grandparent = parent->parent();
		if (!grandparent)
		{
			// Mapping row (parent is action item).
			int ai = _tree->indexOfTopLevelItem(parent);
			int mi = parent->indexOfChild(item);
			if (ai < 0 || mi < 0)
				return;
			_currentActionIdx = ai;
			_currentMappingIdx = mi;
			_currentBindingIdx = -1;
		}
		else
		{
			// Binding row (parent = mapping, grandparent = action).
			int ai = _tree->indexOfTopLevelItem(grandparent);
			int mi = grandparent->indexOfChild(parent);
			int bi = parent->indexOfChild(item);
			if (ai < 0 || mi < 0 || bi < 0)
				return;
			_currentActionIdx = ai;
			_currentMappingIdx = mi;
			_currentBindingIdx = bi;
		}
		_currentMapping = lookupCurrentMapping();
		if (_currentMapping)
			populateInspectorForMapping(_currentMapping);
	});

	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::refreshContextView()
{
	setUpdatesEnabled(false);
	populateMiddleForContext();
	_currentMapping = lookupCurrentMapping();
	if (_currentMapping)
	{
		// _currentBindingIdx: -1 = mapping row, 0..N-1 = bindings[idx].
		if (_currentBindingIdx >= _currentMapping->bindings.size())
			_currentBindingIdx = -1;
		populateInspectorForMapping(_currentMapping);
	}
	else if (_currentContext && _currentActionIdx >= 0
		&& _currentActionIdx < _currentContext->getActionMappings().size())
	{
		populateInspectorForActionEntry();
	}
	else
	{
		_currentActionIdx = -1;
		_currentMappingIdx = -1;
		_currentBindingIdx = -1;
		populateInspectorForContext();
	}
	setUpdatesEnabled(true);
}

void EIQtEditorWindow::refreshInspector()
{
	setUpdatesEnabled(false);
	if (_currentMapping)
		populateInspectorForMapping(_currentMapping);
	else if (_currentContext && _currentActionIdx >= 0
		&& _currentActionIdx < _currentContext->getActionMappings().size())
		populateInspectorForActionEntry();
	else if (_currentAction)
		populateInspectorForAction();
	else if (_currentContext)
		populateInspectorForContext();
	setUpdatesEnabled(true);
}

void EIQtEditorWindow::updateSelectionItemText()
{
	if (!_tree || !_currentContext)
		return;
	auto *actionItem = _tree->topLevelItem(_currentActionIdx);
	if (!actionItem)
		return;
	auto *mappingItem = actionItem->child(_currentMappingIdx);
	if (!mappingItem)
		return;

	auto *mapping = lookupCurrentMapping();
	if (!mapping)
		return;

	// Always refresh the combined mapping-row label "A + B + C".
	QString combined;
	for (int bi = 0; bi < mapping->bindings.size(); ++bi)
	{
		if (bi > 0)
			combined += " + ";
		combined += mapping->bindings[bi].key.getName().get();
	}
	mappingItem->setText(0, combined);

	// If a binding child is selected, also refresh its own row label.
	if (_currentBindingIdx >= 0 && _currentBindingIdx < mapping->bindings.size())
	{
		auto *childItem = mappingItem->child(_currentBindingIdx);
		if (childItem)
			childItem->setText(0, mapping->bindings[_currentBindingIdx].key.getName().get());
	}
}

EIMapping *EIQtEditorWindow::lookupCurrentMapping()
{
	if (!_currentContext)
		return nullptr;
	auto &actions = _currentContext->getActionMappings();
	if (_currentActionIdx < 0 || _currentActionIdx >= actions.size())
		return nullptr;
	auto &mappings = actions[_currentActionIdx].mappings;
	if (_currentMappingIdx < 0 || _currentMappingIdx >= mappings.size())
		return nullptr;
	return &mappings[_currentMappingIdx];
}

void EIQtEditorWindow::populateInspectorForMapping(EIMapping *mapping)
{
	if (!mapping)
		return;

	auto *box = _rightLayout->parentWidget();
	if (box) box->setUpdatesEnabled(false);
	clearLayout(_rightLayout);

	auto onStructure = [this]() {
		markDirty();
		updateSelectionItemText();
		refreshInspector();
	};
	auto onKeyOnly = [this]() {
		markDirty();
		updateSelectionItemText();
	};

	// Editing rules:
	// - AND mapping (size > 1), mapping row: edit only mapping-level (description, consumeInput).
	// - AND mapping, binding child: edit only that binding (key, triggers, modifiers).
	//   Description + consumeInput are shared — edit them on the mapping row, not here.
	// - Single-binding mapping: the mapping IS the binding — edit everything together.
	EIKeyBinding *binding = nullptr;
	bool showMappingProps = false;
	if (_currentBindingIdx >= 0)
	{
		if (_currentBindingIdx >= mapping->bindings.size())
		{
			_currentBindingIdx = -1;
			populateInspectorForMapping(mapping);
			if (box) box->setUpdatesEnabled(true);
			return;
		}
		binding = &mapping->bindings[_currentBindingIdx];
	}
	else if (mapping->bindings.size() == 1)
	{
		binding = &mapping->bindings[0];
		showMappingProps = true;
	}
	else
	{
		showMappingProps = true;
	}

	_rightLayout->addWidget(
		createInspectorTitle(binding ? "Binding" : "Mapping"), 0, Qt::AlignTop);

	auto *form = new QFormLayout;
	form->setContentsMargins(0, 0, 0, 0);

	if (binding)
	{
		auto *picker = new EIKeyPicker;
		picker->setKey(binding->key);
		connect(picker, &EIKeyPicker::keyChanged, this, [binding, onKeyOnly](const EIKey &k) {
			binding->key = k;
			onKeyOnly();
		});
		form->addRow("Key", picker);
	}

	EIQtInspectorSerializer s(form, [this] { markDirty(); });
	if (showMappingProps)
		s.io("Consume input", mapping->consumeInput);
	_rightLayout->addLayout(form);

	if (binding)
	{
		auto onDirty = [this] { markDirty(); };
		renderCreatorList<EITrigger>(this, _rightLayout, "Triggers", binding->triggers,
			EISystem::get()->getTriggerRegistry(), onStructure, onDirty);
		renderCreatorList<EIModifier>(this, _rightLayout, "Modifiers", binding->modifiers,
			EISystem::get()->getModifierRegistry(), onStructure, onDirty);
	}

	_rightLayout->addStretch();
	addSaveDiscardRow();
	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::onSaveContext()
{
	if (_currentContext)
		EISystem::get()->getContextRegistry()->save(_currentContext);
	clearDirty();
}

void EIQtEditorWindow::onSaveAction()
{
	if (_currentAction)
		EISystem::get()->getActionRegistry()->save(_currentAction);
	clearDirty();
}

void EIQtEditorWindow::onDiscard()
{
	// Drop in-memory mutations and reload from disk. The context registry is
	// uncached so destroy + create works; the action registry is cached and
	// shares pointers with anyone else holding the action — reload() rewrites
	// the live instance in place so those references stay valid.
	bool wasContext = _currentContext != nullptr;
	bool wasAction = _currentAction != nullptr;
	int ctxRow = _currentContextRow;
	int savedActionIdx = _currentActionIdx;
	int savedMappingIdx = _currentMappingIdx;
	int savedBindingIdx = _currentBindingIdx;

	if (wasContext)
	{
		releaseCurrent();
		onContextSelected(ctxRow);
		if (_currentContext)
		{
			_currentActionIdx = savedActionIdx;
			_currentMappingIdx = savedMappingIdx;
			_currentBindingIdx = savedBindingIdx;
			refreshContextView();
		}
	}
	else if (wasAction)
	{
		// Reload in place — pointer survives, so we can keep _currentAction
		// without going through release/recreate (which would also force a
		// dirty-state flip during the re-selection round-trip).
		EISystem::get()->getActionRegistry()->reload(_currentAction);
		clearDirty();
		populateInspectorForAction();
	}
}

void EIQtEditorWindow::markDirty()
{
	_dirty = true;
	if (_saveBtn) _saveBtn->setEnabled(true);
	if (_discardBtn) _discardBtn->setEnabled(true);
	if (auto *lbl = getActiveAssetLabel())
	{
		const QString cur = lbl->text();
		if (!cur.endsWith(" *"))
			lbl->setText(cur + " *");
	}
}

void EIQtEditorWindow::clearDirty()
{
	_dirty = false;
	if (_saveBtn) _saveBtn->setEnabled(false);
	if (_discardBtn) _discardBtn->setEnabled(false);
	if (auto *lbl = getActiveAssetLabel())
	{
		const QString cur = lbl->text();
		if (cur.endsWith(" *"))
			lbl->setText(cur.left(cur.length() - 2));
	}
}

QLabel *EIQtEditorWindow::createInspectorTitle(const QString &base)
{
	auto *lbl = new QLabel(QString("<b>%1</b>").arg(base));
	return lbl;
}

QLabel *EIQtEditorWindow::getActiveAssetLabel() const
{
	// The active asset is either a context (row in _contextList) or an
	// action (row in _actionList) — never both, since selecting one clears
	// the other. Look up the corresponding row widget and find its label.
	QTreeWidget *list = nullptr;
	int row = -1;
	if (_currentContext)
	{
		list = _contextList;
		row = _currentContextRow;
	}
	else if (_currentAction)
	{
		list = _actionList;
		row = _currentActionRow;
	}
	if (!list || row < 0)
		return nullptr;
	auto *item = list->topLevelItem(row);
	if (!item)
		return nullptr;
	auto *w = list->itemWidget(item, 0);
	if (!w)
		return nullptr;
	return w->findChild<QLabel *>();
}

bool EIQtEditorWindow::promptUnsavedChanges()
{
	if (!_dirty)
		return true;
	auto btn = QMessageBox::question(this,
		"Unsaved changes",
		"There are unsaved changes in the current asset. Save them?",
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	if (btn == QMessageBox::Cancel)
		return false;
	if (btn == QMessageBox::Save)
	{
		if (_currentAction)
			onSaveAction();
		else
			onSaveContext();
	}
	else if (btn == QMessageBox::Discard)
	{
		// Force-restore disk state on the cached instance. Contexts are
		// non-cached so destroy+create in the caller already reloads them;
		// actions are cached and their mutated pointer would survive the
		// upcoming releaseCurrent, polluting the next create() of the same
		// asset (or any other holder).
		if (_currentAction)
			EISystem::get()->getActionRegistry()->reload(_currentAction);
	}
	return true;
}

void EIQtEditorWindow::addSaveDiscardRow()
{
	// Save dispatches based on what's loaded; Discard reloads from disk.
	_saveBtn = new QPushButton("Save");
	_saveBtn->setFixedSize(80, 28);
	_saveBtn->setEnabled(_dirty);
	connect(_saveBtn.data(), &QPushButton::clicked, this, [this] {
		if (_currentAction) onSaveAction();
		else onSaveContext();
	});

	_discardBtn = new QPushButton("Discard");
	_discardBtn->setFixedSize(80, 28);
	_discardBtn->setEnabled(_dirty);
	connect(_discardBtn.data(), &QPushButton::clicked, this, &EIQtEditorWindow::onDiscard);

	auto *row = new QHBoxLayout;
	row->setContentsMargins(0, 0, 0, 0);
	row->addStretch();
	row->addWidget(_discardBtn.data());
	row->addWidget(_saveBtn.data());
	_rightLayout->addLayout(row);
}

void EIQtEditorWindow::releaseCurrent()
{
	if (_middleWidget) _middleWidget->show();

	// Strip the dirty " *" marker from the active list label BEFORE clearing
	// the current-asset pointers — clearDirty needs both to locate the row.
	clearDirty();

	clearLayout(_middleLayout);
	_middleLayout->addWidget(new QLabel("Select a context or action"), 0, Qt::AlignTop);
	_middleLayout->addStretch();

	clearLayout(_rightLayout);
	_rightLayout->addWidget(new QLabel("<b>Inspector</b>"), 0, Qt::AlignTop);
	_rightLayout->addStretch();

	_tree = nullptr;
	_currentMapping = nullptr;
	_currentActionIdx = -1;
	_currentMappingIdx = -1;
	_currentBindingIdx = -1;
	_currentContextRow = -1;
	_currentActionRow = -1;

	if (_currentContext)
	{
		EISystem::get()->getContextRegistry()->destroy(_currentContext);
		_currentContext = nullptr;
	}
	_currentAction = nullptr;
}

void EIQtEditorWindow::clearLayout(QLayout *layout)
{
	// Note: check count() first — QFormLayout::takeAt(0) on empty layout warns
	// ("Invalid index 0"), unlike QBoxLayout which silently returns nullptr.
	// deleteLater (not delete) so we can call this synchronously from a slot
	// whose emitter widget is among the children being torn down.
	while (layout->count() > 0)
	{
		QLayoutItem *item = layout->takeAt(0);
		if (!item)
			break;
		if (auto *w = item->widget())
		{
			w->hide();
			w->setParent(nullptr);
			w->deleteLater();
		}
		else if (auto *l = item->layout())
		{
			clearLayout(l);
		}
		delete item;
	}
}
