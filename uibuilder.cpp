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

    if (m_pLastLayout) m_pLastLayout->deleteLater();

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

QWidget* UiBuilder::createBlock(IdentityModel::IdentityBlock *block)
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
        QWidget* pItemWidget = createBlockItem(&block->items[i]);
        pFrameLayout->addWidget(pItemWidget);
    }

    pFrame->setLayout(pFrameLayout);

    return pFrame;
}

QWidget* UiBuilder::createBlockHeader(IdentityModel::IdentityBlock *block)
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
    pBlockOptionsButton->setUserData(0, new BlockConnector(block));
    connect(pBlockOptionsButton, SIGNAL(clicked()), this, SLOT(blockOptionsButtonClicked()));
    pLayout->addWidget(pBlockOptionsButton);

    pWidget->setLayout(pLayout);

    return pWidget;
}



QWidget* UiBuilder::createBlockItem(IdentityModel::IdentityBlockItem* item)
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

    QPushButton* pEditButton = new QPushButton();
    pEditButton->setToolTip(tr("Edit value"));
    pEditButton->setMaximumWidth(30);
    pNameLable->setMinimumWidth(30);
    pEditButton->setIcon(QIcon(":/res/img/Edit_16x.png"));
    ItemConnector* pEditButtonConnector = new ItemConnector(item, pValueLineEdit);
    pEditButton->setUserData(0, pEditButtonConnector);
    connect(pEditButton, SIGNAL(clicked()), this, SLOT(editButtonClicked()));
    pLayout->addWidget(pEditButton);

    QPushButton* pCopyButton = new QPushButton();
    pCopyButton->setToolTip(tr("Copy to clipboard"));
    pCopyButton->setMaximumWidth(30);
    pCopyButton->setMinimumWidth(30);
    pCopyButton->setIcon(QIcon(":/res/img/CopyToClipboard_16x.png"));
    pCopyButton->setUserData(0, pEditButtonConnector);
    connect(pCopyButton, SIGNAL(clicked()), this, SLOT(copyButtonClicked()));
    pLayout->addWidget(pCopyButton);

    pWidget->setLayout(pLayout);

    return pWidget;
}

void UiBuilder::editButtonClicked()
{
    ItemConnector* pConnector =
            static_cast<ItemConnector*>(sender()->userData(0));

    bool ok = false;    
    QString result = QInputDialog::getMultiLineText(
                nullptr,
                tr("Edit value"),
                tr("New value for \"%1\":").arg(pConnector->item->name),
                pConnector->item->value, &ok);

    if (ok)
    {
        pConnector->item->value = result;
        pConnector->valueLabel->setText(result);
    }
}

void UiBuilder::copyButtonClicked()
{
    ItemConnector* pConnector =
            static_cast<ItemConnector*>(sender()->userData(0));

        QClipboard* pClipboard = QApplication::clipboard();
        pClipboard->setText(pConnector->item->value);

        QToolTip::showText(
                    static_cast<QWidget*>(sender())->mapToGlobal(QPoint(0,0)),
                    tr("Value copied to clipboard!"),
                    static_cast<QWidget*>(sender()));
}

void UiBuilder::blockOptionsButtonClicked()
{
    BlockConnector* pConnector =
            static_cast<BlockConnector*>(sender()->userData(0));

    QAction* pActionMoveBlockUp = new QAction(QIcon(":/res/img/DoubleUp_24x.png"), tr("Move up"));
    pActionMoveBlockUp->setUserData(0, new BlockConnector(pConnector->block, true));

    QAction* pActionMoveBlockDown = new QAction(QIcon(":/res/img/DoubleDown_24x.png"), tr("Move down"));
    pActionMoveBlockDown->setUserData(0, new BlockConnector(pConnector->block, false));

    QAction* pActionSeparator = new QAction();
    pActionSeparator->setSeparator(true);

    QAction* pActionDeleteBlock = new QAction(QIcon(":/res/img/DeleteBlock_16x.png"), tr("Delete block"));
    pActionDeleteBlock->setUserData(0, new BlockConnector(pConnector->block));

    QMenu* menu = new QMenu(static_cast<QWidget*>(sender()));
    menu->addAction(pActionMoveBlockUp);
    menu->addAction(pActionMoveBlockDown);
    menu->addAction(pActionSeparator);
    menu->addAction(pActionDeleteBlock);

    connect(pActionMoveBlockUp, &QAction::triggered, this, &UiBuilder::moveBlock);
    connect(pActionMoveBlockDown, &QAction::triggered, this, &UiBuilder::moveBlock);
    connect(pActionDeleteBlock, &QAction::triggered, this, &UiBuilder::deleteBlock);

    menu->popup(static_cast<QWidget*>(sender())->mapToGlobal(
                    QPoint(0, 0)));
}

UiBuilder::BlockConnector::BlockConnector()
{
}

UiBuilder::BlockConnector::BlockConnector(IdentityModel::IdentityBlock* block, bool moveUp)
{
    this->block = block;
    this->moveUp = moveUp;
}

UiBuilder::BlockConnector::BlockConnector(BlockConnector& other)
{
    this->block = other.block;
    this->moveUp = other.moveUp;
}

UiBuilder::BlockConnector::~BlockConnector()
{
}

UiBuilder::ItemConnector::ItemConnector()
{
}

UiBuilder::ItemConnector::ItemConnector(IdentityModel::IdentityBlockItem* item, QLineEdit* valueLabel)
{
    this->item = item;
    this->valueLabel = valueLabel;
}

UiBuilder::ItemConnector::ItemConnector(ItemConnector& other)
{
    this->item = other.item;
    this->valueLabel = other.valueLabel;
}

UiBuilder::ItemConnector::~ItemConnector()
{
}

void UiBuilder::deleteBlock()
{
    BlockConnector* pConnector =
            static_cast<BlockConnector*>(sender()->userData(0));

    m_pModel->deleteBlock(pConnector->block);
    rebuild();
}

void UiBuilder::moveBlock()
{
    BlockConnector* pConnector =
            static_cast<BlockConnector*>(sender()->userData(0));

    if (m_pModel->moveBlock(pConnector->block, pConnector->moveUp))
    {
        rebuild();
    }
}
