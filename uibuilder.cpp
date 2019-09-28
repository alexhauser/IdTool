/*
 * This file is part of the "IdTool" utility app.
 *
 * MIT License
 *
 * Copyright (c) 2019 Alexander Hauser
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "uibuilder.h"

UiBuilder::UiBuilder(QScrollArea* scrollArea, IdentityModel* model)
{
    m_pScrollArea = scrollArea;
    m_pModel = model;
}

void UiBuilder::rebuild()
{
    if (!m_pScrollArea || !m_pModel)
    {
        throw std::runtime_error(tr("Invalid container or model pointers!")
                                 .toStdString());
    }

    clear();

    QWidget* pWidget = new QWidget();
    QVBoxLayout *pLayout = new QVBoxLayout();
    pLayout->setSpacing(10);

    for (size_t i=0; i<m_pModel->blocks.size(); i++)
    {
        QWidget* pBlock = createBlock(&m_pModel->blocks[i]);
        pLayout->addWidget(pBlock);
    }

    pLayout->addStretch();

    pWidget->setLayout(pLayout);
    m_pScrollArea->setWidget(pWidget);

    m_pLastWidget = pWidget;
    m_pLastLayout = pLayout;
}

void UiBuilder::clear()
{
    if (m_pLastLayout) m_pLastLayout->deleteLater();
}

bool UiBuilder::showGetBlockTypeDialog(QString *result)
{
    QDir currentPath = QDir::currentPath();
    QDir fullPath = currentPath.filePath(QString("blockdef/"));
    QStringList fileNames = fullPath.entryList(
                QStringList() << "*.json" << "*.JSON",
                QDir::Files);

    QStringList blockDefs;
    foreach(QString fileName, fileNames) {
        blockDefs.append(
                    fileName.mid(0, fileName.indexOf('.'))
                    );
    }

    bool ok = false;
    QString sType = QInputDialog::getItem(
                nullptr,
                tr("Choose block type"),
                tr("Block type"),
                blockDefs,
                0,
                false,
                &ok);

    if (ok) *result = sType;
    return ok;
}

QWidget* UiBuilder::createBlock(IdentityBlock *block)
{
    QString objectName = "obj_" + QUuid::createUuid().toString(QUuid::Id128);

    QFrame* pFrame = new QFrame();
    pFrame->setObjectName(objectName);
    pFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    QString styleSheet = QString("QFrame#") + objectName +
            " { background: " +
            block->color +
            "; border-radius: 6px; }";
    pFrame->setStyleSheet(styleSheet);
    QGridLayout* pFrameLayout = new QGridLayout();
    pFrameLayout->setSpacing(1);

    QWidget* pBlockHeader = createBlockHeader(block);
    pFrameLayout->addWidget(pBlockHeader);

    // Add items
    for (size_t i=0; i<block->items.size(); i++)
    {
        QWidget* pItemWidget = createBlockItem(&block->items[i], block);
        pFrameLayout->addWidget(pItemWidget);
    }

    pFrame->setLayout(pFrameLayout);

    return pFrame;
}

QWidget* UiBuilder::createBlockHeader(IdentityBlock *block)
{
    QWidget* pWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout();

    pLayout->setContentsMargins(5,10,5,30);

    QLabel* pBlockDescLabel = new QLabel(block->description);
    QFont font = pBlockDescLabel->font();
    font.setPointSize(14);
    pBlockDescLabel->setFont(font);
    pBlockDescLabel->setWordWrap(true);
    pLayout->addWidget(pBlockDescLabel);

    QPushButton* pBlockOptionsButton = new QPushButton();
    pBlockOptionsButton->setToolTip(tr("Block options"));
    pBlockOptionsButton->setMaximumWidth(30);
    pBlockOptionsButton->setMinimumWidth(30);
    pBlockOptionsButton->setIcon(QIcon(":/res/img/OptionsDropdown_16x.png"));
    QVariant blockConnectorContainer = QVariant::fromValue(BlockConnector(block));
    pBlockOptionsButton->setProperty("0", blockConnectorContainer );
    connect(pBlockOptionsButton, SIGNAL(clicked()), this, SLOT(blockOptionsButtonClicked()));
    pLayout->addWidget(pBlockOptionsButton);

    pWidget->setLayout(pLayout);

    return pWidget;
}

QWidget* UiBuilder::createBlockItem(IdentityBlockItem* item, IdentityBlock* block)
{
    QWidget* pWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout();
    QString value = item->value;

    if (value.length() > 50)
    {
        value = value.left(40);
        value += "...";
    }

    pLayout->setContentsMargins(0,0,0,0);

    QLabel* pDescImageLabel = new QLabel();
    pDescImageLabel->setToolTip(item->description);
    pDescImageLabel->setMaximumWidth(30);
    pDescImageLabel->setMinimumWidth(30);
    QPixmap mypix (":/res/img/InfoRule_16x.png");
    pDescImageLabel->setPixmap(mypix);
    pLayout->addWidget(pDescImageLabel);

    QLabel* pNameLable = new QLabel(item->name);
    pNameLable->setWordWrap(true);
    pNameLable->setMaximumWidth(150);
    pNameLable->setMinimumWidth(150);
    pLayout->addWidget(pNameLable);

    QLineEdit* pValueLineEdit = new QLineEdit(value);
    pValueLineEdit->setToolTip(item->value);
    pValueLineEdit->setToolTipDuration(-1);
    pValueLineEdit->setObjectName("wDataLabel");
    pValueLineEdit->setStyleSheet("QLineEdit#wDataLabel { background: rgb(237, 237, 237); border-radius: 6px; }");
    pValueLineEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pValueLineEdit->setTextMargins(5, 0, 5, 0);
    pValueLineEdit->setReadOnly(true);
    if (item->value.length() > 0) pValueLineEdit->setCursorPosition(0);
    pLayout->addWidget(pValueLineEdit);

    ItemConnector ic(block, item, pValueLineEdit);

    QPushButton* pEditButton = new QPushButton();
    pEditButton->setToolTip(tr("Edit value"));
    pEditButton->setMaximumWidth(30);
    pNameLable->setMinimumWidth(30);
    pEditButton->setIcon(QIcon(":/res/img/Edit_16x.png"));
    QVariant itemConnectorContainer = QVariant::fromValue(ic);
    pEditButton->setProperty("0", itemConnectorContainer);
    connect(pEditButton, SIGNAL(clicked()), this, SLOT(editButtonClicked()));
    pLayout->addWidget(pEditButton);

    QPushButton* pCopyButton = new QPushButton();
    pCopyButton->setToolTip(tr("Copy to clipboard"));
    pCopyButton->setMaximumWidth(30);
    pCopyButton->setMinimumWidth(30);
    pCopyButton->setIcon(QIcon(":/res/img/CopyToClipboard_16x.png"));
    pCopyButton->setProperty("0", itemConnectorContainer);
    connect(pCopyButton, SIGNAL(clicked()), this, SLOT(copyButtonClicked()));
    pLayout->addWidget(pCopyButton);

    QPushButton* pOptionsButton = new QPushButton();
    pOptionsButton->setToolTip(tr("Item options"));
    pOptionsButton->setMaximumWidth(30);
    pOptionsButton->setMinimumWidth(30);
    pOptionsButton->setIcon(QIcon(":/res/img/OptionsDropdown_16x.png"));
    pOptionsButton->setProperty("0", itemConnectorContainer);
    connect(pOptionsButton, SIGNAL(clicked()), this, SLOT(itemOptionsButtonClicked()));
    pLayout->addWidget(pOptionsButton);

    pWidget->setLayout(pLayout);

    return pWidget;
}

void UiBuilder::editButtonClicked()
{
    ItemConnector connector =
            sender()->property("0").value<ItemConnector>();

    bool ok = false;    
    QString result = QInputDialog::getMultiLineText(
                nullptr,
                tr("Edit value"),
                tr("New value for \"%1\":").arg(connector.item->name),
                connector.item->value, &ok);

    if (ok)
    {
        connector.item->value = result;
        connector.valueLabel->setText(result);
    }
}

void UiBuilder::copyButtonClicked()
{
    ItemConnector connector =
            sender()->property("0").value<ItemConnector>();

        QClipboard* pClipboard = QApplication::clipboard();
        pClipboard->setText(connector.item->value);

        QToolTip::showText(
                    static_cast<QWidget*>(sender())->mapToGlobal(QPoint(0,0)),
                    tr("Value copied to clipboard!"),
                    static_cast<QWidget*>(sender()));
}

void UiBuilder::blockOptionsButtonClicked()
{
    BlockConnector connector =
            sender()->property("0").value<BlockConnector>();

    QAction* pActionMoveBlockUp = new QAction(QIcon(":/res/img/DoubleUp_24x.png"), tr("Move up"));
    QVariant upBlockConnectorContainer = QVariant::fromValue(BlockConnector(connector.block, true));
    pActionMoveBlockUp->setProperty("0", upBlockConnectorContainer);

    QAction* pActionMoveBlockDown = new QAction(QIcon(":/res/img/DoubleDown_24x.png"), tr("Move down"));
    QVariant downBlockConnectorContainer = QVariant::fromValue(BlockConnector(connector.block, false));
    pActionMoveBlockDown->setProperty("0", downBlockConnectorContainer);

    QAction* pActionSeparator = new QAction();
    pActionSeparator->setSeparator(true);

    QAction* pActionAddBlock = new QAction(QIcon(":/res/img/Add_16x.png"), tr("Add block"));
    QVariant addBlockConnectorContainer = QVariant::fromValue(BlockConnector(connector.block));
    pActionAddBlock->setProperty("0", addBlockConnectorContainer);

    QAction* pActionDeleteBlock = new QAction(QIcon(":/res/img/DeleteBlock_16x.png"), tr("Delete block"));
    QVariant deleteBlockConnectorContainer = QVariant::fromValue(BlockConnector(connector.block));
    pActionDeleteBlock->setProperty("0", deleteBlockConnectorContainer);

    QMenu* menu = new QMenu(static_cast<QWidget*>(sender()));
    menu->addAction(pActionMoveBlockUp);
    menu->addAction(pActionMoveBlockDown);
    menu->addAction(pActionSeparator);
    menu->addAction(pActionAddBlock);
    menu->addAction(pActionDeleteBlock);

    connect(pActionMoveBlockUp, &QAction::triggered, this, &UiBuilder::moveBlock);
    connect(pActionMoveBlockDown, &QAction::triggered, this, &UiBuilder::moveBlock);
    connect(pActionAddBlock, &QAction::triggered, this, &UiBuilder::addBlock);
    connect(pActionDeleteBlock, &QAction::triggered, this, &UiBuilder::deleteBlock);

    menu->popup(static_cast<QWidget*>(sender())->mapToGlobal(
                    QPoint(0, 0)));
}

BlockConnector::BlockConnector() :
block(nullptr), moveUp(true)
{
}

BlockConnector::BlockConnector(IdentityBlock* block, bool moveUp)
{
    this->block = block;
    this->moveUp = moveUp;
}

BlockConnector::BlockConnector(const BlockConnector& other)
{
    this->block = other.block;
    this->moveUp = other.moveUp;
}

BlockConnector::~BlockConnector()
{
}

ItemConnector::ItemConnector() :
    block(nullptr), item(nullptr), valueLabel(nullptr), moveUp(true)
{
}

ItemConnector::ItemConnector(IdentityBlock* block, IdentityBlockItem* item, QLineEdit* valueLabel, bool moveUp)
{
    this->block = block;
    this->item = item;
    this->valueLabel = valueLabel;
    this->moveUp = moveUp;
}

ItemConnector::ItemConnector(const ItemConnector& other)
{
    this->block = other.block;
    this->item = other.item;
    this->valueLabel = other.valueLabel;
    this->moveUp = other.moveUp;
}

ItemConnector::~ItemConnector()
{
}

void UiBuilder::deleteBlock()
{
    BlockConnector connector =
            sender()->property("0").value<BlockConnector>();

    m_pModel->deleteBlock(connector.block);
    rebuild();
}

void UiBuilder::moveBlock()
{
    BlockConnector connector =
            sender()->property("0").value<BlockConnector>();

    if (m_pModel->moveBlock(connector.block, connector.moveUp))
    {
        rebuild();
    }
}

void UiBuilder::addBlock()
{
    BlockConnector connector =
            sender()->property("0").value<BlockConnector>();

    bool ok = false;
    QString sBlockType;
    ok = showGetBlockTypeDialog(&sBlockType);
    if (!ok) return;

    ushort blockType = sBlockType.toUShort(&ok);
    if (!ok) return;

    IdentityBlock block = IdentityParser::createEmptyBlock(blockType);
    m_pModel->insertBlock(block, connector.block);

    rebuild();
}

bool UiBuilder::showGetRepeatCountDialog(QString itemName, int* result)
{
    bool ok;
    int repititons = QInputDialog::getInt(
                nullptr,
                tr("Item repitition"),
                tr("How many \"%1\" fields should be created?").arg(itemName),
                1,
                1,
                1024,
                1,
                &ok);

    if (ok) *result = repititons;

    return ok;
}

void UiBuilder::itemOptionsButtonClicked()
{
    ItemConnector connector =
            sender()->property("0").value<ItemConnector>();

    QAction* pActionMoveItemUp = new QAction(QIcon(":/res/img/DoubleUp_24x.png"), tr("Move up"));
    QVariant upItemConnectorContainer = QVariant::fromValue(
                ItemConnector(connector.block, connector.item, nullptr));
    pActionMoveItemUp->setProperty("0", upItemConnectorContainer);

    QAction* pActionMoveItemDown = new QAction(QIcon(":/res/img/DoubleDown_24x.png"), tr("Move down"));
    QVariant downItemConnectorContainer = QVariant::fromValue(
               ItemConnector(connector.block, connector.item, nullptr));
    pActionMoveItemDown->setProperty("0", downItemConnectorContainer);

    QAction* pActionSeparator = new QAction();
    pActionSeparator->setSeparator(true);

    QAction* pActionAddItem = new QAction(QIcon(":/res/img/Add_16x.png"), tr("Add item"));
    QVariant addItemConnectorContainer = QVariant::fromValue(
                ItemConnector(connector.block, connector.item, nullptr));
    pActionAddItem->setProperty("0", addItemConnectorContainer);

    QAction* pActionDeleteItem = new QAction(QIcon(":/res/img/DeleteBlock_16x.png"), tr("Delete item"));
    QVariant deleteItemConnectorContainer = QVariant::fromValue(
                ItemConnector(connector.block, connector.item, nullptr));
    pActionDeleteItem->setProperty("0", deleteItemConnectorContainer);

    QMenu* menu = new QMenu(static_cast<QWidget*>(sender()));
    menu->addAction(pActionMoveItemUp);
    menu->addAction(pActionMoveItemDown);
    menu->addAction(pActionSeparator);
    menu->addAction(pActionAddItem);
    menu->addAction(pActionDeleteItem);

    connect(pActionMoveItemUp, &QAction::triggered, this, &UiBuilder::moveItem);
    connect(pActionMoveItemDown, &QAction::triggered, this, &UiBuilder::moveItem);
    connect(pActionAddItem, &QAction::triggered, this, &UiBuilder::addNewItem);
    connect(pActionDeleteItem, &QAction::triggered, this, &UiBuilder::deleteItem);

    menu->popup(static_cast<QWidget*>(sender())->mapToGlobal(
                    QPoint(0, 0)));
}

void UiBuilder::deleteItem()
{
    ItemConnector connector =
            sender()->property("0").value<ItemConnector>();

    bool ok = connector.block->deleteItem(connector.item);

    if (ok) rebuild();
}

void UiBuilder::moveItem()
{
    ItemConnector connector =
            sender()->property("0").value<ItemConnector>();

    //TODO

    rebuild();
}

void UiBuilder::addNewItem()
{
    ItemConnector connector =
            sender()->property("0").value<ItemConnector>();

    //TODO

    rebuild();
}
