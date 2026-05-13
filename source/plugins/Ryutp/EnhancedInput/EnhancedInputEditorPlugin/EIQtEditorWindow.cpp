#include "EIQtEditorWindow.h"
#include "EIQtInspectorSerializer.h"
#include "EIKeyPicker.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

#include <QComboBox>
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
	std::function<void()> onChanged)
{
	auto defer = [owner, onChanged]() { QTimer::singleShot(0, owner, onChanged); };

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
		auto *combo = new QComboBox;
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

		// Child item with parameter form (only when a real trigger/modifier).
		if (items[i])
		{
			auto *paramItem = new QTreeWidgetItem();
			headerItem->addChild(paramItem);
			paramItem->setFirstColumnSpanned(true);

			auto *paramWidget = new QWidget;
			paramWidget->setAttribute(Qt::WA_TranslucentBackground);
			auto *form = new QFormLayout(paramWidget);
			form->setContentsMargins(8, 4, 8, 4);
			EIQtInspectorSerializer s(form);
			items[i]->serialize(s);

			paramWidget->adjustSize();
			paramItem->setSizeHint(0, QSize(0, paramWidget->sizeHint().height()));
			tree->setItemWidget(paramItem, 0, paramWidget);
		}
	}

	// On expand/collapse, ask the tree to recompute its size hint so the
	// outer QScrollArea grows/shrinks accordingly.
	QObject::connect(tree, &QTreeWidget::itemExpanded, owner,
		[tree](QTreeWidgetItem *) { tree->updateGeometry(); });
	QObject::connect(tree, &QTreeWidget::itemCollapsed, owner,
		[tree](QTreeWidgetItem *) { tree->updateGeometry(); });

	layout->addWidget(tree);
}

} // namespace

EIQtEditorWindow::EIQtEditorWindow(QWidget *parent)
	: QWidget(parent)
{
	setObjectName("EIQtEditorWindow");

	// Visual polish: subtle hover in trees, compact combobox padding.
	setStyleSheet(
		"QTreeView::item:hover:!selected { background: palette(midlight); }"
		"QComboBox { padding: 1px 6px; min-height: 20px; }"
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

	if (_middleWidget) _middleWidget->show();
	populateMiddleForContext();
	populateInspectorForContext();
}

void EIQtEditorWindow::onActionSelected(int row)
{
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
	_rightLayout->addWidget(new QLabel("<b>Inspector</b>"), 0, Qt::AlignTop);

	auto *form = new QFormLayout;
	form->setContentsMargins(0, 0, 0, 0);
	form->addRow("Name", new QLabel(_currentContext->name.get()));
	EIQtInspectorSerializer s(form);
	s.io("Description", _currentContext->description);
	s.io("Auto register", _currentContext->autoRegistration);
	_rightLayout->addLayout(form);

	_rightLayout->addStretch();

	auto *saveBtn = new QPushButton("Save");
	saveBtn->setFixedSize(80, 28);
	connect(saveBtn, &QPushButton::clicked, this, &EIQtEditorWindow::onSaveContext);
	auto *saveRow = new QHBoxLayout;
	saveRow->setContentsMargins(0, 0, 0, 0);
	saveRow->addStretch();
	saveRow->addWidget(saveBtn);
	_rightLayout->addLayout(saveRow);

	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::populateInspectorForAction()
{
	if (!_currentAction)
		return;

	auto *box = _rightLayout->parentWidget();
	if (box) box->setUpdatesEnabled(false);
	clearLayout(_rightLayout);
	_rightLayout->addWidget(new QLabel("<b>Inspector</b>"), 0, Qt::AlignTop);

	auto *form = new QFormLayout;
	form->setContentsMargins(0, 0, 0, 0);
	form->addRow("Name", new QLabel(_currentAction->name.get()));
	EIQtInspectorSerializer s(form);
	s.io("Description", _currentAction->description);
	s.io("Value type", _currentAction->valueType);
	s.io("Accumulation", _currentAction->accumulationBehavior);
	_rightLayout->addLayout(form);

	auto onChanged = [this]() {
		QTimer::singleShot(0, this, [this]() { populateInspectorForAction(); });
	};

	renderCreatorList<EIModifier>(this, _rightLayout, "Modifiers", _currentAction->modifiers,
		EISystem::get()->getModifierRegistry(), onChanged);
	renderCreatorList<EITrigger>(this, _rightLayout, "Triggers", _currentAction->triggers,
		EISystem::get()->getTriggerRegistry(), onChanged);

	_rightLayout->addStretch();

	auto *saveBtn = new QPushButton("Save");
	saveBtn->setFixedSize(80, 28);
	connect(saveBtn, &QPushButton::clicked, this, &EIQtEditorWindow::onSaveAction);
	auto *saveRow = new QHBoxLayout;
	saveRow->setContentsMargins(0, 0, 0, 0);
	saveRow->addStretch();
	saveRow->addWidget(saveBtn);
	_rightLayout->addLayout(saveRow);

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
			QTimer::singleShot(0, this, [this]() { refreshContextView(); });
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

	auto *actionRegistry = EISystem::get()->getActionRegistry();
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
				QTimer::singleShot(0, this, [this]() { refreshContextView(); });
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
				QTimer::singleShot(0, this, [this]() { refreshContextView(); });
			});
			_tree->setItemWidget(mappingItem, 1, cell);

			// All bindings as children — uniform, all removable.
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
					QTimer::singleShot(0, this, [this]() { refreshContextView(); });
				});
				_tree->setItemWidget(bindingItem, 1, bCell);
			}
		}

		// Column 0: action picker combo only.
		auto *combo = new QComboBox;
		combo->setMinimumWidth(120);
		combo->addItem("(none)");
		for (int i = 0; i < actionRegistry->getCount(); ++i)
			combo->addItem(actionRegistry->getName(i));

		int currentIdx = 0;
		if (entry.action)
		{
			int byName = actionRegistry->getIndexByName(entry.action->name);
			if (byName >= 0)
				currentIdx = byName + 1;
		}
		combo->setCurrentIndex(currentIdx);

		connect(combo, qOverload<int>(&QComboBox::activated), this, [this, ai](int newIdx) {
			if (!_currentContext)
				return;
			auto &actions = _currentContext->getActionMappings();
			if (ai < 0 || ai >= actions.size())
				return;
			auto *reg = EISystem::get()->getActionRegistry();
			actions[ai].action = newIdx == 0 ? nullptr : reg->create(newIdx - 1);
		});

		_tree->setItemWidget(actionItem, 0, combo);

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
			QTimer::singleShot(0, this, [this]() { refreshContextView(); });
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
			QTimer::singleShot(0, this, [this]() { refreshContextView(); });
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
			return; // top-level action item — no inspector
		auto *grandparent = parent->parent();
		if (!grandparent)
		{
			// Mapping row (parent is action item) — inspector shows mapping-level.
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
			// Child 0 = primary, child >=1 = andKeys[idx-1].
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
		QTimer::singleShot(0, this, [this]() {
			updateSelectionItemText();
			refreshInspector();
		});
	};
	auto onKeyOnly = [this]() {
		updateSelectionItemText();
	};

	auto addSaveRow = [this]() {
		_rightLayout->addStretch();
		auto *saveBtn = new QPushButton("Save");
		saveBtn->setFixedSize(80, 28);
		connect(saveBtn, &QPushButton::clicked, this, &EIQtEditorWindow::onSaveContext);
		auto *saveRow = new QHBoxLayout;
		saveRow->setContentsMargins(0, 0, 0, 0);
		saveRow->addStretch();
		saveRow->addWidget(saveBtn);
		_rightLayout->addLayout(saveRow);
	};

	// Case 1: mapping row selected — show mapping-level options.
	if (_currentBindingIdx < 0)
	{
		_rightLayout->addWidget(new QLabel("<b>Mapping</b>"), 0, Qt::AlignTop);

		auto *consumeForm = new QFormLayout;
		consumeForm->setContentsMargins(0, 0, 0, 0);
		EIQtInspectorSerializer s(consumeForm);
		s.io("Consume input", mapping->consumeInput);
		_rightLayout->addLayout(consumeForm);

		addSaveRow();
		if (box) box->setUpdatesEnabled(true);
		return;
	}

	// A binding child is selected — show its key + triggers + modifiers.
	if (_currentBindingIdx >= mapping->bindings.size())
	{
		// Stale index; fall back to mapping row.
		_currentBindingIdx = -1;
		populateInspectorForMapping(mapping);
		if (box) box->setUpdatesEnabled(true);
		return;
	}
	EIKeyBinding *binding = &mapping->bindings[_currentBindingIdx];

	_rightLayout->addWidget(new QLabel("<b>Binding</b>"), 0, Qt::AlignTop);

	auto *keyForm = new QFormLayout;
	keyForm->setContentsMargins(0, 0, 0, 0);
	auto *picker = new EIKeyPicker;
	picker->setKey(binding->key);
	connect(picker, &EIKeyPicker::keyChanged, this, [binding, onKeyOnly](const EIKey &k) {
		binding->key = k;
		onKeyOnly();
	});
	keyForm->addRow("Key", picker);
	_rightLayout->addLayout(keyForm);

	renderCreatorList<EITrigger>(this, _rightLayout, "Triggers", binding->triggers,
		EISystem::get()->getTriggerRegistry(), onStructure);
	renderCreatorList<EIModifier>(this, _rightLayout, "Modifiers", binding->modifiers,
		EISystem::get()->getModifierRegistry(), onStructure);

	addSaveRow();
	if (box) box->setUpdatesEnabled(true);
}

void EIQtEditorWindow::onSaveContext()
{
	if (_currentContext)
		EISystem::get()->getContextRegistry()->save(_currentContext);
}

void EIQtEditorWindow::onSaveAction()
{
	if (_currentAction)
		EISystem::get()->getActionRegistry()->save(_currentAction);
}

void EIQtEditorWindow::releaseCurrent()
{
	if (_middleWidget) _middleWidget->show();

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
	while (layout->count() > 0)
	{
		QLayoutItem *item = layout->takeAt(0);
		if (!item)
			break;
		if (auto *w = item->widget())
		{
			w->setParent(nullptr);
			delete w;
		}
		else if (auto *l = item->layout())
		{
			clearLayout(l);
		}
		delete item;
	}
}
