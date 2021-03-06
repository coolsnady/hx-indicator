#include "OnchainOrderPage.h"
#include "ui_OnchainOrderPage.h"

#include "wallet.h"
#include "commondialog.h"
#include "BuyOrderWidget.h"
#include "ToolButtonWidget.h"
#include "control/BlankDefaultWidget.h"
#include "poundage/PageScrollWidget.h"

#include <QtMath>

static const int ROWNUMBER = 6;
OnchainOrderPage::OnchainOrderPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OnchainOrderPage)
{
    ui->setupUi(this);

    connect( HXChain::getInstance(), SIGNAL(jsonDataUpdated(QString)), this, SLOT(jsonDataUpdated(QString)));

    ui->ordersTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->ordersTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->ordersTableWidget->setFocusPolicy(Qt::NoFocus);
//    ui->ordersTableWidget->setFrameShape(QFrame::NoFrame);
    ui->ordersTableWidget->setMouseTracking(true);
    ui->ordersTableWidget->setShowGrid(false);//隐藏表格线
    ui->ordersTableWidget->horizontalHeader()->setSectionsClickable(true);
//    ui->ordersTableWidget->horizontalHeader()->setFixedHeight(35);
    ui->ordersTableWidget->horizontalHeader()->setVisible(true);
    ui->ordersTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->ordersTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->ordersTableWidget->verticalHeader()->setVisible(false);
    ui->ordersTableWidget->setColumnWidth(0,190);
    ui->ordersTableWidget->setColumnWidth(1,180);
    ui->ordersTableWidget->setColumnWidth(2,130);
    ui->ordersTableWidget->setColumnWidth(3,90);

    ui->ordersTableWidget->setStyleSheet(TABLEWIDGET_STYLE_1);

    pageWidget = new PageScrollWidget();
    ui->stackedWidget->addWidget(pageWidget);
    connect(pageWidget,&PageScrollWidget::currentPageChangeSignal,this,&OnchainOrderPage::pageChangeSlot);

    blankWidget = new BlankDefaultWidget(ui->ordersTableWidget);
    blankWidget->setTextTip(tr("There are no orders!"));

    HXChain::getInstance()->mainFrame->installBlurEffect(ui->ordersTableWidget);
    init();
}

OnchainOrderPage::~OnchainOrderPage()
{
    delete ui;
}

void OnchainOrderPage::init()
{
    QStringList accounts = HXChain::getInstance()->accountInfoMap.keys();
    ui->accountComboBox->addItems(accounts);

    if(accounts.contains(HXChain::getInstance()->currentAccount))
    {
        ui->accountComboBox->setCurrentText(HXChain::getInstance()->currentAccount);
    }

    QStringList assetIds = HXChain::getInstance()->assetInfoMap.keys();
    foreach (QString assetId, assetIds)
    {
        ui->assetComboBox->addItem(HXChain::getInstance()->assetInfoMap.value(assetId).symbol, assetId);
        ui->assetComboBox2->addItem(HXChain::getInstance()->assetInfoMap.value(assetId).symbol, assetId);
    }

    if(assetIds.contains(HXChain::getInstance()->currentSellAssetId))
    {
        ui->assetComboBox->setCurrentText(HXChain::getInstance()->assetInfoMap.value(HXChain::getInstance()->currentSellAssetId).symbol);
    }

    if(assetIds.contains(HXChain::getInstance()->currentBuyAssetId))
    {
        ui->assetComboBox2->setCurrentText(HXChain::getInstance()->assetInfoMap.value(HXChain::getInstance()->currentBuyAssetId).symbol);
    }


    connect(&httpManager,SIGNAL(httpReplied(QByteArray,int)),this,SLOT(httpReplied(QByteArray,int)));

    inited = true;

    on_accountComboBox_currentIndexChanged(ui->accountComboBox->currentText());
}

void OnchainOrderPage::onBack()
{
    if(currentWidget)
    {
        currentWidget->close();
        currentWidget = NULL;
    }
}

void OnchainOrderPage::jsonDataUpdated(QString id)
{

}

void OnchainOrderPage::httpReplied(QByteArray _data, int _status)
{
    QJsonObject object  = QJsonDocument::fromJson(_data).object();
    QJsonArray  array   = object.take("result").toObject().take("data").toArray();

    int size = array.size();
    ui->ordersTableWidget->setRowCount(size);
    for(int i = 0; i < size; i++)
    {
        ui->ordersTableWidget->setRowHeight(i,40);

        QJsonObject dataObject = array.at(i).toObject();
        QString contractAddress = dataObject.take("contract_address").toString();
        unsigned long long sellAmount = jsonValueToULL(dataObject.take("from_supply"));
        QString sellSymbol = dataObject.take("from_asset").toString();
        unsigned long long buyAmount = jsonValueToULL(dataObject.take("to_supply"));
        QString buySymbol = dataObject.take("to_asset").toString();
        int state = dataObject.take("state").toInt();

        AssetInfo sellAssetInfo = HXChain::getInstance()->assetInfoMap.value(HXChain::getInstance()->getAssetId(sellSymbol));
        AssetInfo buyAssetInfo  = HXChain::getInstance()->assetInfoMap.value(HXChain::getInstance()->getAssetId(buySymbol));

        ui->ordersTableWidget->setItem(i,0, new QTableWidgetItem(getBigNumberString(sellAmount,sellAssetInfo.precision)));
        ui->ordersTableWidget->item(i,0)->setData(Qt::UserRole,sellAmount);

        ui->ordersTableWidget->setItem(i,1, new QTableWidgetItem(getBigNumberString(buyAmount,buyAssetInfo.precision)));
        ui->ordersTableWidget->item(i,1)->setData(Qt::UserRole,buyAmount);

        double price = (double)sellAmount / qPow(10,sellAssetInfo.precision) / buyAmount * qPow(10,buyAssetInfo.precision);
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(price,'g',8));
        item->setData(Qt::UserRole,contractAddress);
        ui->ordersTableWidget->setItem(i,2,item);

        ui->ordersTableWidget->setItem(i,3, new QTableWidgetItem(tr("buy")));

        for(int j = 3;j < 4;++j)
        {
            ToolButtonWidgetItem *toolButtonItem = new ToolButtonWidgetItem(i,j);

            if(HXChain::getInstance()->getExchangeContractAddress(ui->accountComboBox->currentText()) == contractAddress)
            {
                toolButtonItem->setEnabled(false);
                toolButtonItem->setText(tr("my order"));
                toolButtonItem->setButtonFixSize(80,20);
            }
            else
            {
                toolButtonItem->setText(ui->ordersTableWidget->item(i,j)->text());          
            }

            ui->ordersTableWidget->setCellWidget(i,j,toolButtonItem);
            connect(toolButtonItem,SIGNAL(itemClicked(int,int)),this,SLOT(onItemClicked(int,int)));
        }
    }
    tableWidgetSetItemZebraColor(ui->ordersTableWidget);

    int page = (ui->ordersTableWidget->rowCount()%ROWNUMBER==0 && ui->ordersTableWidget->rowCount() != 0) ?
                ui->ordersTableWidget->rowCount()/ROWNUMBER : ui->ordersTableWidget->rowCount()/ROWNUMBER+1;
    pageWidget->SetTotalPage(page);
    pageWidget->setShowTip(ui->ordersTableWidget->rowCount(),ROWNUMBER);
    pageChangeSlot(0);
    pageWidget->setVisible(0 != size);

    blankWidget->setVisible(0 == size);
}

void OnchainOrderPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(QColor(229,226,240),Qt::SolidLine));
    painter.setBrush(QBrush(QColor(229,226,240),Qt::SolidPattern));

    painter.drawRect(rect());
}

void OnchainOrderPage::on_assetComboBox_currentIndexChanged(const QString &arg1)
{
    if(!inited)     return;

    HXChain::getInstance()->currentSellAssetId = ui->assetComboBox->currentData().toString();

    ui->ordersTableWidget->setRowCount(0);
    updateTableHeaders();
//    if(ui->assetComboBox->currentText().isEmpty() || ui->assetComboBox2->currentText().isEmpty()
//            || ui->assetComboBox->currentText() == ui->assetComboBox2->currentText())       return;

    queryContractOrders();
}

void OnchainOrderPage::on_assetComboBox2_currentIndexChanged(const QString &arg1)
{
    if(!inited)     return;

    HXChain::getInstance()->currentBuyAssetId = ui->assetComboBox2->currentData().toString();

    ui->ordersTableWidget->setRowCount(0);
    updateTableHeaders();
//    if(ui->assetComboBox->currentText().isEmpty() || ui->assetComboBox2->currentText().isEmpty()
//            || ui->assetComboBox->currentText() == ui->assetComboBox2->currentText())       return;

    queryContractOrders();
}

void OnchainOrderPage::queryContractOrders()
{
    QJsonObject object;
    object.insert("jsonrpc","2.0");
    object.insert("id",45);
    object.insert("method","Zchain.Exchange.queryContracts");
    QJsonObject paramObject;
    paramObject.insert("from_asset",ui->assetComboBox->currentText());
    paramObject.insert("to_asset",ui->assetComboBox2->currentText());
    paramObject.insert("limit",10);
    object.insert("params",paramObject);
    httpManager.post(HXChain::getInstance()->middlewarePath,QJsonDocument(object).toJson());
}

void OnchainOrderPage::updateTableHeaders()
{
    if(ui->assetComboBox->currentText() == ui->assetComboBox2->currentText())
    {
        ui->ordersTableWidget->horizontalHeaderItem(0)->setText(tr("SELL"));
        ui->ordersTableWidget->horizontalHeaderItem(1)->setText(tr("BUY"));
        ui->ordersTableWidget->horizontalHeaderItem(2)->setText(tr("PRICE"));
    }
    else
    {
        ui->ordersTableWidget->horizontalHeaderItem(0)->setText(tr("SELL / %1").arg(ui->assetComboBox->currentText()));
        ui->ordersTableWidget->horizontalHeaderItem(1)->setText(tr("BUY / %1").arg(ui->assetComboBox2->currentText()));
        ui->ordersTableWidget->horizontalHeaderItem(2)->setText(tr("PRICE (%1/%2)").arg(ui->assetComboBox->currentText()).arg(ui->assetComboBox2->currentText()));
    }

}

void OnchainOrderPage::onItemClicked(int _row, int _column)
{
    if(_column == 3)
    {
        if(HXChain::getInstance()->accountInfoMap.isEmpty())
        {
            CommonDialog dia(CommonDialog::OkOnly);
            dia.setText(tr("Please Import Or Create Account First!"));
            dia.pop();
            HXChain::getInstance()->mainFrame->ShowMainPageSlot();
            return;
        }


        if(!HXChain::getInstance()->ValidateOnChainOperation()) return;

        BuyOrderWidget* buyOrderWidget = new BuyOrderWidget(this);
        buyOrderWidget->setAttribute(Qt::WA_DeleteOnClose);
        buyOrderWidget->move(0,0);
        buyOrderWidget->show();
        buyOrderWidget->raise();

        buyOrderWidget->setAccount(ui->accountComboBox->currentText());
        QString contractAddress = ui->ordersTableWidget->item(_row,2)->data(Qt::UserRole).toString();
        buyOrderWidget->setContractAddress(contractAddress);
        buyOrderWidget->setOrderInfo(ui->ordersTableWidget->item(_row,0)->data(Qt::UserRole).toULongLong(), ui->assetComboBox->currentText()
                                     ,ui->ordersTableWidget->item(_row,1)->data(Qt::UserRole).toULongLong(), ui->assetComboBox2->currentText());

        currentWidget = buyOrderWidget;

        emit backBtnVisible(true);

        return;
    }
}

void OnchainOrderPage::on_accountComboBox_currentIndexChanged(const QString &arg1)
{
    if(!inited)  return;

    HXChain::getInstance()->currentAccount = ui->accountComboBox->currentText();

    on_assetComboBox_currentIndexChanged(ui->assetComboBox->currentText());
}

void OnchainOrderPage::pageChangeSlot(unsigned int page)
{
    for(int i = 0;i < ui->ordersTableWidget->rowCount();++i)
    {
        if(i < page*ROWNUMBER)
        {
            ui->ordersTableWidget->setRowHidden(i,true);
        }
        else if(page * ROWNUMBER <= i && i < page*ROWNUMBER + ROWNUMBER)
        {
            ui->ordersTableWidget->setRowHidden(i,false);
        }
        else
        {
            ui->ordersTableWidget->setRowHidden(i,true);
        }
    }

}
