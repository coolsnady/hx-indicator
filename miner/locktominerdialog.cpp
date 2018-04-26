#include "locktominerdialog.h"
#include "ui_locktominerdialog.h"

#include "wallet.h"
#include "commondialog.h"

LockToMinerDialog::LockToMinerDialog(QString _accountName, QWidget *parent) :
    QDialog(parent),
    m_accountName(_accountName),
    ui(new Ui::LockToMinerDialog)
{
    ui->setupUi(this);

    setParent(UBChain::getInstance()->mainFrame);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint);

    ui->widget->setObjectName("widget");
    ui->widget->setStyleSheet("#widget {background-color:rgba(251, 251, 254,100);}");
    ui->containerWidget->setObjectName("containerwidget");
    ui->containerWidget->setStyleSheet("#containerwidget{background-color:rgb(255,255,255);border-radius:10px;}");

    ui->closeBtn->setIconSize(QSize(12,12));
    ui->closeBtn->setIcon(QIcon(":/ui/wallet_ui/close.png"));
    ui->closeBtn->setStyleSheet("QToolButton{background-color:transparent;border:none;}"
                                "QToolButton:hover{background-color:rgb(208,228,255);}");


    setStyleSheet("QToolButton#okBtn,QToolButton#okBtn2{background-color:#5474EB; border:none;border-radius:10px;color: rgb(255, 255, 255);}"
                  "QToolButton#okBtn:hover,QToolButton#okBtn2:hover{background-color:#00D2FF;}"
                  "QToolButton#cancelBtn,QToolButton#cancelBtn2{background-color:#00D2FF; border:none;border-radius:10px;color: rgb(255, 255, 255);}"
                  "QToolButton#cancelBtn:hover,QToolButton#cancelBtn2:hover{background-color:#5474EB;}"
                  "QComboBox{    \
                  border: none;\
                  background:transparent;\
                  font-size: 10pt;\
                  font-family: MicrosoftYaHei;\
                  background-position: center left;\
                  color: black;\
                  selection-background-color: darkgray;}"
                  "QLineEdit{border:none;background:transparent;color:#5474EB;font-size:12pt;margin-left:2px;}"
                  "QLineEdit:focus{border:none;}"
                  );

    connect( UBChain::getInstance(), SIGNAL(jsonDataUpdated(QString)), this, SLOT(jsonDataUpdated(QString)));

    init();
}

LockToMinerDialog::~LockToMinerDialog()
{
    delete ui;
}

void LockToMinerDialog::pop()
{
    move(0,0);
    exec();
}

void LockToMinerDialog::init()
{
    ui->accountNameLabel->setText(m_accountName);

    QStringList miners;
    foreach (Miner m, UBChain::getInstance()->minersVector)
    {
        miners += m.name;
    }

    ui->minerComboBox->addItems(miners);

    QStringList keys = UBChain::getInstance()->assetInfoMap.keys();
    foreach (QString key, keys)
    {
        ui->assetComboBox->addItem(UBChain::getInstance()->assetInfoMap.value(key).symbol, key);
    }
}

void LockToMinerDialog::setMiner(QString _minerName)
{
    ui->minerComboBox->setCurrentText(_minerName);
}

void LockToMinerDialog::setAsset(QString _assetName)
{
    ui->assetComboBox->setCurrentText(_assetName);
}

void LockToMinerDialog::jsonDataUpdated(QString id)
{
    if( id == "id-lock_balance_to_miner")
    {
        QString result = UBChain::getInstance()->jsonDataValue(id);

        qDebug() << id << result;
        if(result.startsWith("\"result\":{"))
        {
            close();

            CommonDialog commonDialog(CommonDialog::OkOnly);
            commonDialog.setText(tr("Lock balance to miner successfully!"));
            commonDialog.pop();
        }
        else if(result.startsWith("\"error\":"))
        {
            int pos = result.indexOf("\"message\":\"") + 11;
            QString errorMessage = result.mid(pos, result.indexOf("\"", pos) - pos);

            CommonDialog commonDialog(CommonDialog::OkOnly);
            commonDialog.setText( "Lock balance to miner failed: " + errorMessage );
            commonDialog.pop();
        }
    }
}

void LockToMinerDialog::on_okBtn_clicked()
{
    UBChain::getInstance()->postRPC( "id-lock_balance_to_miner",
                                     toJsonFormat( "lock_balance_to_miner",
                                                   QJsonArray() << ui->minerComboBox->currentText() << m_accountName
                                                   << ui->amountLineEdit->text() << ui->assetComboBox->currentText()
                                                   << true ));

}

void LockToMinerDialog::on_cancelBtn_clicked()
{
    close();
}

void LockToMinerDialog::on_assetComboBox_currentIndexChanged(const QString &arg1)
{
    ui->amountLineEdit->clear();

    AssetAmount assetAmount = UBChain::getInstance()->accountInfoMap.value(m_accountName).assetAmountMap.value(ui->assetComboBox->currentData().toString());
    QString amountStr = getBigNumberString(assetAmount.amount, UBChain::getInstance()->assetInfoMap.value(ui->assetComboBox->currentData().toString()).precision);

    ui->amountLineEdit->setPlaceholderText(tr("total %1 %2").arg(amountStr).arg(ui->assetComboBox->currentText()) );
}

void LockToMinerDialog::on_closeBtn_clicked()
{
    close();
}