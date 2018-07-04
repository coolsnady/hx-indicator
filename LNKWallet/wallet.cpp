﻿#include "wallet.h"

#include "websocketmanager.h"
#include "commondialog.h"

#ifdef WIN32
#include <Windows.h>
#endif
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QFrame>
#include <QDesktopWidget>


UBChain* UBChain::goo = 0;
//QMutex mutexForJsonData;
//QMutex mutexForPending;
//QMutex mutexForConfigFile;
//QMutex mutexForMainpage;
//QMutex mutexForPendingFile;
//QMutex mutexForDelegateList;
//QMutex mutexForRegisterMap;
//QMutex mutexForBalanceMap;
//QMutex mutexForAddressMap;
//QMutex mutexForRpcReceiveOrNot;

UBChain::UBChain()
{
    nodeProc = new QProcess;
    clientProc = new QProcess;
    isExiting = false;
    isBlockSyncFinish = false;
    wsManager = NULL;

    getSystemEnvironmentPath();

    currentDialog = NULL;
    hasDelegateSalary = false;
    currentPort = CLIENT_RPC_PORT;
    currentAccount = "";
    transactionFee = 0;

    configFile = new QSettings( walletConfigPath + "/config.ini", QSettings::IniFormat);
    if( configFile->value("/settings/lockMinutes").toInt() == 0)   // 如果第一次，没有config.ini
    {
        configFile->setValue("/settings/lockMinutes",5);
        lockMinutes     = 5;
        configFile->setValue("/settings/notAutoLock",false);
        notProduce      =  true;
        configFile->setValue("/settings/language","English");
        language = "English";
        configFile->setValue("/settings/feeType","LNK");
        feeType = "LNK";
        configFile->setValue("/settings/feeOrderID","");
        feeOrderID = "";
        configFile->setValue("/settings/backupNeeded",false);
        IsBackupNeeded = false;
        configFile->setValue("/settings/autoDeposit",false);
        autoDeposit = false;
        minimizeToTray  = false;
        configFile->setValue("/settings/minimizeToTray",false);
        closeToMinimize = false;
        configFile->setValue("/settings/closeToMinimize",false);
        resyncNextTime = false;
        configFile->setValue("settings/resyncNextTime",false);
        contractFee = 1;
        configFile->setValue("settings/contractFee",1);
        middlewarePath = "http://117.78.44.37:5005/api";
        configFile->setValue("settings/middlewarePath",middlewarePath);
    }
    else
    {
        lockMinutes     = configFile->value("/settings/lockMinutes").toInt();
        notProduce      = !configFile->value("/settings/notAutoLock").toBool();
        minimizeToTray  = configFile->value("/settings/minimizeToTray").toBool();
        closeToMinimize = configFile->value("/settings/closeToMinimize").toBool();
        language        = configFile->value("/settings/language").toString();
        feeType         = configFile->value("/settings/feeType").toString();
        feeOrderID      = configFile->value("/settings/feeOrderID").toString();
        IsBackupNeeded  = configFile->value("/settings/backupNeeded").toBool();
        autoDeposit     = configFile->value("/settings/autoDeposit").toBool();
        resyncNextTime  = configFile->value("/settings/resyncNextTime",false).toBool();
        contractFee     = configFile->value("/settings/contractFee",1).toULongLong();
        middlewarePath  = configFile->value("/settings/middlewarePath","http://117.78.44.37:5005/api").toString();
        importedWalletNeedToAddTrackAddresses = configFile->value("/settings/importedWalletNeedToAddTrackAddresses",false).toBool();
    }

    QFile file( walletConfigPath + "/log.txt");       // 每次启动清空 log.txt文件
    file.open(QIODevice::Truncate | QIODevice::WriteOnly);
    file.close();

//    contactsFile = new QFile( "contacts.dat");
    contactsFile = NULL;

    pendingFile  = new QFile( walletConfigPath + "/pending.dat");

    //初始化手续费
    InitFeeCharge();

    isUpdateNeeded = false;
//   updateProcess = new UpdateProcess();
//   connect(updateProcess,&UpdateProcess::updateFinish,[this](){
//        this->isUpdateNeeded = true;
//   });
//   connect(updateProcess,&UpdateProcess::updateWrong,[this](){
//       this->isUpdateNeeded = false;
//  });
}

UBChain::~UBChain()
{


    if (configFile)
    {
        delete configFile;
        configFile = NULL;
    }

    if( contactsFile)
    {
        delete contactsFile;
        contactsFile = NULL;
    }

    if( wsManager)
    {
        delete wsManager;
        wsManager = NULL;
    }
}

UBChain*   UBChain::getInstance()
{
    if( goo == NULL)
    {
        goo = new UBChain;
    }
    return goo;
}

void UBChain:: startExe()
{
    connect(nodeProc,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(onNodeExeStateChanged()));

    QStringList strList;
    strList << "--data-dir=" + UBChain::getInstance()->configFile->value("/settings/chainPath").toString().replace("\\","/")
            << QString("--rpc-endpoint=127.0.0.1:%1").arg(NODE_RPC_PORT);

    if( UBChain::getInstance()->configFile->value("/settings/resyncNextTime",false).toBool())
    {
        strList << "--replay";
    }
    UBChain::getInstance()->configFile->setValue("/settings/resyncNextTime",false);

    nodeProc->start("lnk_node.exe",strList);
    qDebug() << "start lnk_node.exe " << strList;


//    UBChain::getInstance()->initWebSocketManager();
//    emit exeStarted();
}

void UBChain::onNodeExeStateChanged()
{
    qDebug() << "node exe state " << nodeProc->state();
    if(isExiting)   return;

    if(nodeProc->state() == QProcess::Starting)
    {
        qDebug() << QString("%1 is starting").arg("lnk_node.exe");
    }
    else if(nodeProc->state() == QProcess::Running)
    {
        qDebug() << QString("%1 is running").arg("lnk_node.exe");
        connect(&timerForStartExe,SIGNAL(timeout()),this,SLOT(checkNodeExeIsReady()));
        timerForStartExe.start(1000);
    }
    else if(nodeProc->state() == QProcess::NotRunning)
    {
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("Fail to launch %1 !").arg("lnk_node.exe"));
        commonDialog.pop();
    }
}

void UBChain::onClientExeStateChanged()
{
    if(isExiting)   return;

    if(clientProc->state() == QProcess::Starting)
    {
        qDebug() << QString("%1 is starting").arg("lnk_client.exe");
    }
    else if(clientProc->state() == QProcess::Running)
    {
        qDebug() << QString("%1 is running").arg("lnk_client.exe");

        UBChain::getInstance()->initWebSocketManager();
        emit exeStarted();
    }
    else if(clientProc->state() == QProcess::NotRunning)
    {
        qDebug() << "client not running" + clientProc->errorString();
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("Fail to launch %1 !").arg("lnk_client.exe"));
        commonDialog.pop();
    }
}

void UBChain::delayedLaunchClient()
{
    connect(clientProc,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(onClientExeStateChanged()));

    QStringList strList;
    strList << "--wallet-file=" + UBChain::getInstance()->configFile->value("/settings/chainPath").toString().replace("\\","/") + "/wallet.json"
            << QString("--server-rpc-endpoint=ws://127.0.0.1:%1").arg(NODE_RPC_PORT)
            << QString("--rpc-endpoint=127.0.0.1:%1").arg(CLIENT_RPC_PORT);

    clientProc->start("lnk_client.exe",strList);
}

void UBChain::checkNodeExeIsReady()
{
    QString str = nodeProc->readAllStandardError();
    qDebug() << "node exe standardError: " << str ;
    if(str.contains("Chain ID is"))
    {
        timerForStartExe.stop();
        QTimer::singleShot(1000,this,SLOT(delayedLaunchClient()));
    }
}

void UBChain::ShowBubbleMessage(const QString &title, const QString &context, int msecs, QSystemTrayIcon::MessageIcon icon)
{
    emit showBubbleSignal(title,context,icon,msecs);
}

QString UBChain::getMinerNameFromId(QString _minerId)
{
    QString name;
    foreach (Miner m, minersVector)
    {
        if(m.minerId == _minerId)
        {
            name = m.name;
        }
    }

    return name;
}

QString UBChain::currentContractFee()
{
    return getBigNumberString(contractFee,ASSET_PRECISION);
}

UBChain::TotalContractFee UBChain::parseTotalContractFee(QString result)
{
    result.prepend("{");
    result.append("}");

    QTextCodec* utfCodec = QTextCodec::codecForName("UTF-8");
    QByteArray ba = utfCodec->fromUnicode(result);

    QJsonDocument parse_doucment = QJsonDocument::fromJson(ba);
    QJsonObject object = parse_doucment.object();
    QJsonArray array = object.take("result").toArray();

    UBChain::TotalContractFee totalFee;
    totalFee.baseAmount = jsonValueToULL(array.at(0).toObject().take("amount"));
    totalFee.step = array.at(1).toInt();

    return totalFee;
}

qint64 UBChain::write(QString cmd)
{
    QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
    QByteArray cmdBa = gbkCodec->fromUnicode(cmd);  // 转为gbk的bytearray
    clientProc->readAll();
    return clientProc->write(cmdBa.data());
}

QString UBChain::read()
{
    QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
    QString result;
    QString str;
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    while ( !result.contains(">>>"))        // 确保读取全部输出
    {
        clientProc->waitForReadyRead(50);
        str = gbkCodec->toUnicode(clientProc->readAll());
        result += str;
//        if( str.right(2) == ": " )  break;

        //  手动调用处理未处理的事件，防止界面阻塞
//        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    QApplication::restoreOverrideCursor();
    return result;

}



void UBChain::deleteAccountInConfigFile(QString accountName)
{

    mutexForConfigFile.lock();
    configFile->beginGroup("/accountInfo");
    QStringList keys = configFile->childKeys();

    int i = 0;
    for( ; i < keys.size(); i++)
    {
        if( configFile->value( keys.at(i)) == accountName)
        {
            break;
        }
    }

    for( ; i < keys.size() - 1; i++)
    {
        configFile->setValue( keys.at(i) , configFile->value( keys.at(i + 1)));
    }

    configFile->remove( keys.at(i));

    configFile->endGroup();
    mutexForConfigFile.unlock();


}


TwoAddresses UBChain::getAddress(QString name)
{
    TwoAddresses twoAddresses;

    if( name.isEmpty())
    {
        return twoAddresses;
    }

    QString result = jsonDataValue("id_wallet_list_my_addresses");

    int pos = result.indexOf( "\"name\":\"" + name + "\",") ;
    if( pos != -1)  // 如果 wallet_list_my_addresses 中存在
    {

        int pos2 = result.indexOf( "\"owner_address\":", pos) + 17;
        twoAddresses.ownerAddress = result.mid( pos2, result.indexOf( "\"", pos2) - pos2);

        pos2 = result.indexOf( "\"active_address\":", pos) + 18;
        twoAddresses.activeAddress = result.mid( pos2, result.indexOf( "\"", pos2) - pos2);
    }

    return twoAddresses;
}



void UBChain::getSystemEnvironmentPath()
{
    QStringList environment = QProcess::systemEnvironment();
    QString str;

#ifdef WIN32
    foreach(str,environment)
    {
        if (str.startsWith("APPDATA="))
        {
#ifdef TEST_WALLET
            walletConfigPath = str.mid(8) + "\\LNKWallet_test";
#else
            walletConfigPath = str.mid(8) + "\\LNKWallet";
#endif
            appDataPath = walletConfigPath + "\\chaindata";
            qDebug() << "appDataPath:" << appDataPath;
            break;
        }
    }
#elif defined(TARGET_OS_MAC)
    foreach(str,environment)
    {
        if (str.startsWith("HOME="))
        {
#ifdef TEST_WALLET
            walletConfigPath = str.mid(5) + "/Library/Application Support/LNKWallet_test";
#else
            walletConfigPath = str.mid(5) + "/Library/Application Support/LNKWallet";
#endif
            appDataPath = walletConfigPath + "/chaindata";
            qDebug() << "appDataPath:" << appDataPath;
            break;
        }
    }
#else
    foreach(str,environment)
    {
        if (str.startsWith("HOME="))
        {
#ifdef TEST_WALLET
            walletConfigPath = str.mid(5) + "/LNKWallet_test";
#else
            walletConfigPath = str.mid(5) + "/LNKWallet";
#endif
            appDataPath = walletConfigPath + "/chaindata";
            qDebug() << "appDataPath:" << appDataPath;
            break;
        }
    }
#endif
}


void UBChain::getContactsFile()
{
    QString path;
    if( configFile->contains("/settings/chainPath"))
    {
        path = configFile->value("/settings/chainPath").toString();
    }
    else
    {
        path = appDataPath;
    }

    contactsFile = new QFile(path + "\\contacts.dat");
    if( contactsFile->exists())
    {
        // 如果数据路径下 有contacts.dat 则使用数据路径下的
        return;
    }
//    else
//    {
//        QFile file2("contacts.dat");
//        if( file2.exists())
//        {
//            // 如果数据路径下没有 钱包路径下有 将钱包路径下的剪切到数据路径下
//       qDebug() << "contacts.dat copy" << file2.copy(path + "\\contacts.dat");
//       qDebug() << "contacts.dat copy" << file2.remove();
//            return;
//        }
//        else
//        {
//            // 如果都没有 则使用钱包路径下的
//        }
    //    }
}

void UBChain::parseAccountInfo()
{
    QString jsonResult = jsonDataValue("id-list_my_accounts");
    jsonResult.prepend("{");
    jsonResult.append("}");


    QJsonDocument parse_doucment = QJsonDocument::fromJson(jsonResult.toLatin1());
    QJsonObject jsonObject = parse_doucment.object();
    if( jsonObject.contains("result"))
    {
        QJsonValue resultValue = jsonObject.take("result");
        if( resultValue.isArray())
        {
//            accountInfoMap.clear();
            QStringList accountNameList;

            QJsonArray resultArray = resultValue.toArray();
            for( int i = 0; i < resultArray.size(); i++)
            {
                QJsonObject object = resultArray.at(i).toObject();
                QString name = object.take("name").toString();

                AccountInfo accountInfo;
                if(accountInfoMap.contains(name))
                {
                    accountInfo = accountInfoMap.value(name);
                }

                accountInfo.id = object.take("id").toString();
                accountInfo.name = name;
                accountInfo.address = object.take("addr").toString();

                if(formalGuardMap.contains(accountInfo.name))
                {
                    accountInfo.guardId = formalGuardMap.value(accountInfo.name).guardId;
                    accountInfo.isFormalGuard = true;
                }
                else if(allGuardMap.contains(accountInfo.name))
                {
                    accountInfo.guardId = allGuardMap.value(accountInfo.name);
                }

                accountInfoMap.insert(accountInfo.name,accountInfo);

                fetchAccountBalances(accountInfo.name);

                accountNameList.append(accountInfo.name);
            }

            foreach (QString key, accountInfoMap.keys())
            {
                if(!accountNameList.contains(key))
                {
                    accountInfoMap.remove(key);
                }
            }
        }
    }
}

void UBChain::fetchAccountBalances(QString _accountName)
{
    postRPC( "id-get_account_balances-" + _accountName, toJsonFormat( "get_account_balances", QJsonArray() << _accountName));
}

QString UBChain::getAccountBalance(QString _accountName, QString _assetSymbol)
{
    AssetAmountMap map = accountInfoMap.value(_accountName).assetAmountMap;
    QString assetId = getAssetId(_assetSymbol);
    AssetInfo assetInfo = UBChain::getInstance()->assetInfoMap.value(assetId);

    return getBigNumberString(map.value(assetId).amount, assetInfo.precision);
}

QStringList UBChain::getRegisteredAccounts()
{
    QStringList keys = accountInfoMap.keys();
    QStringList accounts;
    foreach (QString key, keys)
    {
        if(accountInfoMap.value(key).id != "0.0.0")
        {
            accounts += key;
        }
    }

    return accounts;
}

QStringList UBChain::getUnregisteredAccounts()
{
    QStringList keys = accountInfoMap.keys();
    QStringList accounts;
    foreach (QString key, keys)
    {
        if(accountInfoMap.value(key).id == "0.0.0")
        {
            accounts += key;
        }
    }

    return accounts;
}

QString UBChain::getExchangeContractAddress(QString _accountName)
{
    QString result;

    foreach (ContractInfo info, accountInfoMap.value(_accountName).contractsVector)
    {
        if(info.hashValue == EXCHANGE_CONTRACT_HASH)
        {
            result = info.contractAddress;
            break;
        }
    }

    return result;
}

QString UBChain::getExchangeContractState(QString _accountName)
{
    QString result;
    foreach (ContractInfo info, accountInfoMap.value(_accountName).contractsVector)
    {
        if(info.hashValue == EXCHANGE_CONTRACT_HASH)
        {
            result = info.state;
            break;
        }
    }

    return result;
}


QString UBChain::getAssetId(QString symbol)
{
    QString id;
    foreach (QString key, UBChain::getInstance()->assetInfoMap.keys())
    {
        AssetInfo info = UBChain::getInstance()->assetInfoMap.value(key);
        if( info.symbol == symbol)
        {
            id = key;
            continue;
        }
    }

    return id;
}

bool UBChain::ValidateOnChainOperation()
{
    if(GetBlockSyncFinish()) return true;
    //当前状态不允许链上操作时，弹出警告框
    if(mainFrame)
    {
        CommonDialog dia(CommonDialog::OkOnly);
        dia.setText(tr("You have not synchronized the latest block. The transaction you create will be outdated and not confirmed!"));
        dia.pop();
    }
    return false;
}

void UBChain::InitFeeCharge()
{//读取手续费文件
    QFile fileExit(":/config/feeCharge.config");
    if(!fileExit.open(QIODevice::ReadOnly)) return ;

    QString jsonStr(fileExit.readAll());
    fileExit.close();

    QTextCodec* utfCodec = QTextCodec::codecForName("UTF-8");
    QByteArray ba = utfCodec->fromUnicode(jsonStr);

    QJsonParseError json_error;
    QJsonDocument parse_doucment = QJsonDocument::fromJson(ba, &json_error);

    if(json_error.error != QJsonParseError::NoError || !parse_doucment.isObject()) return ;

    QJsonObject jsonObject = parse_doucment.object();
    //开始分析
    QJsonObject feeObj = jsonObject.value("LNKFee").toObject();
    feeChargeInfo.tunnelBindFee = feeObj.value("BindTunnel").toString();
    feeChargeInfo.minerIncomeFee = feeObj.value("MinerIncome").toString();
    feeChargeInfo.minerForeCloseFee = feeObj.value("MinerForeClose").toString();
    feeChargeInfo.minerRedeemFee = feeObj.value("MinerRedeem").toString();
    feeChargeInfo.minerRegisterFee = feeObj.value("MinerRegister").toString();
    feeChargeInfo.poundagePublishFee = feeObj.value("PoundagePublish").toString();
    feeChargeInfo.poundageCancelFee = feeObj.value("PoundageCancel").toString();
    feeChargeInfo.transferFee = feeObj.value("Transfer").toString();

    //其他费用
    QJsonObject crossfeeObj = jsonObject.value("CrossFee").toObject();
    feeChargeInfo.withDrawFee = crossfeeObj.value("WithDraw").toString();
    feeChargeInfo.capitalFee = crossfeeObj.value("Capital").toString();
}

void UBChain::addTrackAddress(QString _address)
{
    QFile file(UBChain::getInstance()->appDataPath + "/config.ini");
    if(file.exists())
    {
        if(file.open(QIODevice::ReadWrite))
        {
            QString str = file.readAll();
#ifdef WIN32
            QString str2 = "track-address = \"" + _address + "\"\r\n";
#else
            QString str2 = "track-address = \"" + _address + "\"\n";
#endif
            str.prepend(str2);

            file.resize(0);
            QTextStream ts(&file);
            ts << str.toLatin1();
            file.close();
        }
    }
    else
    {
        qDebug() << "can not find chaindata/config.ini ";
    }
}

void UBChain::autoSaveWalletFile()
{
    QFileInfo fileInfo(appDataPath + "/wallet.json");
    if(fileInfo.size() > 0)
    {
        QFile autoSaveFile(appDataPath + "/wallet.json.autobak");
        qDebug() << "auto save remove " << autoSaveFile.remove();

        QFile file(appDataPath + "/wallet.json");
        qDebug() << "auto save wallet.json " << file.copy(appDataPath + "/wallet.json.autobak");
    }
}

void UBChain::fetchTransactions()
{
    postRPC( "id-list_transactions", toJsonFormat( "list_transactions", QJsonArray() << 0 << -1));

    checkPendingTransactions();
}

void UBChain::parseTransaction(QString result)
{
    result.prepend("{");
    result.append("}");

    QJsonDocument parse_doucment = QJsonDocument::fromJson(result.toLatin1());
    QJsonObject jsonObject = parse_doucment.object();
    QJsonObject object = jsonObject.take("result").toObject();

    TransactionStruct ts;
    ts.transactionId = object.take("trxid").toString();
    ts.blockNum = object.take("block_num").toInt();
    ts.expirationTime = object.take("expiration").toString();

    QJsonArray array = object.take("operations").toArray().at(0).toArray();
    ts.type = array.at(0).toInt();
    QJsonObject operationObject = array.at(1).toObject();
    ts.operationStr = QJsonDocument(operationObject).toJson();
    ts.feeAmount = jsonValueToULL( operationObject.take("fee").toObject().take("amount"));

    transactionDB.insertTransactionStruct(ts.transactionId,ts);
    qDebug() << "ttttttttttttt " << ts.transactionId << ts.type << ts.feeAmount;

    TransactionTypeId typeId;
    typeId.type = ts.type;
    typeId.transactionId = ts.transactionId;
    switch (typeId.type)
    {
    case TRANSACTION_TYPE_NORMAL:
    {
        // 普通交易
        QString fromAddress = operationObject.take("from_addr").toString();
        QString toAddress   = operationObject.take("to_addr").toString();


        if(isMyAddress(fromAddress))
        {
            transactionDB.addAccountTransactionId(fromAddress, typeId);
        }

        if(isMyAddress(toAddress))
        {
            transactionDB.addAccountTransactionId(toAddress, typeId);
        }
    }
        break;
    case TRANSACTION_TYPE_REGISTER_ACCOUNT:
    {
        // 注册账户
        QString payer = operationObject.take("payer").toString();

        transactionDB.addAccountTransactionId(payer, typeId);
    }
        break;
    case TRANSACTION_TYPE_BIND_TUNNEL:
    {
        // 绑定tunnel地址
        QString addr = operationObject.take("addr").toString();

        transactionDB.addAccountTransactionId(addr, typeId);
    }
        break;
    case TRANSACTION_TYPE_UNBIND_TUNNEL:
    {
        // 解绑tunnel地址
        QString addr = operationObject.take("addr").toString();

        transactionDB.addAccountTransactionId(addr, typeId);
    }
        break;
    case TRANSACTION_TYPE_LOCKBALANCE:
    {
        // 质押资产给miner
        QString addr = operationObject.take("lock_balance_addr").toString();

        transactionDB.addAccountTransactionId(addr, typeId);
    }
        break;
    case TRANSACTION_TYPE_FORECLOSE:
    {
        // 赎回质押资产
        QString addr = operationObject.take("foreclose_addr").toString();

        transactionDB.addAccountTransactionId(addr, typeId);
    }
        break;
    case TRANSACTION_TYPE_DEPOSIT:
    {
        // 充值交易
        QJsonObject crossChainTrxObject = operationObject.take("cross_chain_trx").toObject();
        QString fromTunnelAddress = crossChainTrxObject.take("from_account").toString();
        QString depositAddress = operationObject.take("deposit_address").toString();

        transactionDB.addAccountTransactionId(fromTunnelAddress, typeId);
        transactionDB.addAccountTransactionId(depositAddress, typeId);
    }
        break;
    case TRANSACTION_TYPE_WITHDRAW:
    {
        // 提现交易
        QString withdrawAddress = operationObject.take("withdraw_account").toString();

        if(isMyAddress(withdrawAddress))
        {
            transactionDB.addAccountTransactionId(withdrawAddress, typeId);
        }

    }
        break;
    case TRANSACTION_TYPE_MINE_INCOME:
    {
        // 质押挖矿收入
        QString owner = operationObject.take("pay_back_owner").toString();

        if(isMyAddress(owner))
        {
            transactionDB.addAccountTransactionId(owner, typeId);
        }

    }
        break;
    case TRANSACTION_TYPE_CONTRACT_REGISTER:
    {
        // 注册合约
        QString ownerAddr = operationObject.take("owner_addr").toString();

        transactionDB.addAccountTransactionId(ownerAddr, typeId);
    }
        break;
    case TRANSACTION_TYPE_CONTRACT_INVOKE:
    {
        // 调用合约
        QString callerAddr = operationObject.take("caller_addr").toString();

        transactionDB.addAccountTransactionId(callerAddr, typeId);
    }
        break;
    case TRANSACTION_TYPE_CONTRACT_TRANSFER:
    {
        // 转账到合约
        QString callerAddr = operationObject.take("caller_addr").toString();

        transactionDB.addAccountTransactionId(callerAddr, typeId);
    }
        break;
    case TRANSACTION_TYPE_CREATE_GUARANTEE:
    {
        // 创建承兑单
        QString ownerAddr = operationObject.take("owner_addr").toString();

        transactionDB.addAccountTransactionId(ownerAddr, typeId);
    }
        break;
    case TRANSACTION_TYPE_CANCEL_GUARANTEE:
    {
        // 撤销承兑单
        QString ownerAddr = operationObject.take("owner_addr").toString();

        transactionDB.addAccountTransactionId(ownerAddr, typeId);
    }
        break;
    default:
        break;
    }
}

void UBChain::checkPendingTransactions()
{
    QStringList trxIds = transactionDB.getPendingTransactions();
    foreach (QString trxId, trxIds)
    {
        postRPC( "id-get_transaction-" + trxId, toJsonFormat( "get_transaction", QJsonArray() << trxId));
    }
}

void UBChain::fetchFormalGuards()
{
    postRPC( "id-list_guard_members", toJsonFormat( "list_guard_members", QJsonArray() << "A" << 100));
}

void UBChain::fetchAllGuards()
{
    postRPC( "id-list_all_guards", toJsonFormat( "list_all_guards", QJsonArray() << "A" << 100));
}

QStringList UBChain::getMyFormalGuards()
{
    QStringList result;
    QStringList guards = formalGuardMap.keys();
    foreach (QString key, accountInfoMap.keys())
    {
        if(guards.contains(key))
        {
            result += key;
        }
    }

    return result;
}

GuardMultisigAddress UBChain::getGuardMultisigByPairId(QString assetSymbol, QString guardName, QString pairId)
{
    QString guardAccountId = formalGuardMap.value(guardName).accountId;
    QVector<GuardMultisigAddress> v = guardMultisigAddressesMap.value(assetSymbol + "-" + guardAccountId);
qDebug() << "11111111111 " << assetSymbol + "-" + guardAccountId << v.size();
    GuardMultisigAddress result;
    foreach (GuardMultisigAddress gma, v)
    {
        qDebug() << "222222222 " << gma.pairId ;

        if(gma.pairId == pairId)
        {
            result = gma;
        }
    }

    return result;
}

void UBChain::fetchGuardAllMultisigAddresses(QString accountId)
{
    QStringList keys = assetInfoMap.keys();
    foreach (QString key, keys)
    {
        if(assetInfoMap.value(key).symbol == ASSET_NAME)   continue;

        QString assetSymbol = assetInfoMap.value(key).symbol;
        postRPC( "id-get_multi_address_obj-" + assetSymbol + "-" + accountId,
                 toJsonFormat( "get_multi_address_obj", QJsonArray() << assetSymbol << accountId));
    }
}

QStringList UBChain::getAssetMultisigUpdatedGuards(QString assetSymbol)
{
    QStringList result;
    QStringList keys = formalGuardMap.keys();
    foreach (QString key, keys)
    {
        QString accountId = formalGuardMap.value(key).accountId;
        QVector<GuardMultisigAddress> vector = guardMultisigAddressesMap.value(assetSymbol + "-" + accountId);
        foreach (GuardMultisigAddress gma, vector)
        {
            if(gma.pairId == "2.7.0")
            {
                result += accountId;
                break;
            }
        }
    }

    return result;
}

QString UBChain::guardAccountIdToName(QString guardAccountId)
{
    QString result;
    foreach (QString account, formalGuardMap.keys())
    {
        if( formalGuardMap.value(account).accountId == guardAccountId)
        {
            result = account;
            break;
        }
    }

    return result;
}

QString UBChain::guardAddressToName(QString guardAddress)
{
    QString result;
    foreach (QString account, formalGuardMap.keys())
    {
        if( formalGuardMap.value(account).address == guardAddress)
        {
            result = account;
            break;
        }
    }

    return result;
}

void UBChain::fetchProposals()
{
    if(formalGuardMap.size() < 1)   return;
    postRPC( "id-get_proposal_for_voter", toJsonFormat( "get_proposal_for_voter", QJsonArray() << formalGuardMap.keys().at(0)));
}

void UBChain::fetchMyContracts()
{
    foreach (QString accountName, accountInfoMap.keys())
    {
        postRPC( "id-get_contracts_hash_entry_by_owner-" + accountName, toJsonFormat( "get_contracts_hash_entry_by_owner", QJsonArray() << accountName));
    }
}

bool UBChain::isMyAddress(QString _address)
{
    QStringList keys = accountInfoMap.keys();
    bool result = false;
    foreach (QString key, keys)
    {
        if(accountInfoMap.value(key).address == _address)   result = true;
    }

    return result;
}

QString UBChain::addressToName(QString _address)
{
    QStringList keys = accountInfoMap.keys();

    foreach (QString key, keys)
    {
        if( accountInfoMap.value(key).address == _address)
        {
            return key;
        }
    }

    return _address;
}

void UBChain::parseMultiSigTransactions(QString result, QString multiSigAddress)
{
    multiSigTransactionsMap.remove(multiSigAddress);

    result.prepend("{");
    result.append("}");

    QTextCodec* utfCodec = QTextCodec::codecForName("UTF-8");
    QByteArray ba = utfCodec->fromUnicode(result);

    QJsonParseError json_error;
    QJsonDocument parse_doucment = QJsonDocument::fromJson(ba, &json_error);
    if(json_error.error == QJsonParseError::NoError)
    {
        if( parse_doucment.isObject())
        {
            QJsonObject jsonObject = parse_doucment.object();
            if( jsonObject.contains("result"))
            {
                QJsonValue resultValue = jsonObject.take("result");
                if( resultValue.isArray())
                {
                    TransactionsInfoVector transactionsInfoVector;
                    QJsonArray resultArray = resultValue.toArray();

                    for( int i = 0; i < resultArray.size(); i++)
                    {
                        TransactionInfo transactionInfo;

                        QJsonObject object          = resultArray.at(i).toObject();
                        transactionInfo.isConfirmed = object.take("is_confirmed").toBool();
                        if ( !transactionInfo.isConfirmed)
                        {
                            // 包含error  则为失效交易
                            if( object.contains("error"))   continue;
                        }

                        transactionInfo.trxId       = object.take("trx_id").toString();
                        transactionInfo.isMarket    = object.take("is_market").toBool();
                        transactionInfo.isMarketCancel = object.take("is_market_cancel").toBool();
                        transactionInfo.blockNum    = object.take("block_num").toInt();
                        transactionInfo.timeStamp   = object.take("timestamp").toString();

                        QJsonArray entriesArray       = object.take("ledger_entries").toArray();
                        for( int j = 0; j < entriesArray.size(); j++)
                        {
                            QJsonObject entryObject = entriesArray.at(j).toObject();
                            Entry   entry;
                            entry.fromAccount       = entryObject.take("from_account").toString();
                            entry.toAccount         = entryObject.take("to_account").toString();
                            QJsonValue v = entryObject.take("memo");
                            entry.memo              = v.toString();

                            QJsonObject amountObject = entryObject.take("amount").toObject();
                            entry.amount.assetId     = amountObject.take("asset_id").toInt();
                            QJsonValue amountValue   = amountObject.take("amount");
                            if( amountValue.isString())
                            {
                                entry.amount.amount  = amountValue.toString().toULongLong();
                            }
                            else
                            {
                                entry.amount.amount  = QString::number(amountValue.toDouble(),'g',10).toULongLong();
                            }

                            QJsonArray runningBalanceArray  = entryObject.take("running_balances").toArray().at(0).toArray().at(1).toArray();
                            for( int k = 0; k < runningBalanceArray.size(); k++)
                            {
                                QJsonObject amountObject2    = runningBalanceArray.at(k).toArray().at(1).toObject();
                                AssetAmount assetAmount;
                                assetAmount.assetId = amountObject2.take("asset_id").toInt();
                                QJsonValue amountValue2   = amountObject2.take("amount");
                                if( amountValue2.isString())
                                {
                                    assetAmount.amount  = amountValue2.toString().toULongLong();
                                }
                                else
                                {
                                    assetAmount.amount  = QString::number(amountValue2.toDouble(),'g',10).toULongLong();
                                }
                                entry.runningBalances.append(assetAmount);
                            }

                            transactionInfo.entries.append(entry);
                        }

                        QJsonObject object5         = object.take("fee").toObject();
                        QJsonValue amountValue3     = object5.take("amount");
                        if( amountValue3.isString())
                        {
                            transactionInfo.fee     = amountValue3.toString().toULongLong();
                        }
                        else
                        {
                            transactionInfo.fee     = QString::number(amountValue3.toDouble(),'g',10).toULongLong();
                        }
                        transactionInfo.feeId       = object5.take("asset_id").toInt();

                        transactionsInfoVector.append(transactionInfo);

                    }

                    multiSigTransactionsMap.insert(multiSigAddress,transactionsInfoVector);
                }
            }
        }
    }
}

void UBChain::quit()
{
    isExiting = true;
    if (clientProc)
    {
//        clientProc->close();
        qDebug() << "clientProc: delete";

//        AttachConsole((uint)clientProc->processId());
//        SetConsoleCtrlHandler(NULL, true);
//        GenerateConsoleCtrlEvent( CTRL_C_EVENT ,0);
        clientProc->close();
//        delete clientProc;
        clientProc = NULL;
    }


    if (nodeProc)
    {
//        nodeProc->close();
        qDebug() << "nodeProc: delete";

//        AttachConsole((uint)nodeProc->processId());
//        SetConsoleCtrlHandler(NULL, true);
//        GenerateConsoleCtrlEvent( CTRL_C_EVENT ,0);

        nodeProc->close();
//        delete nodeProc;
        nodeProc = NULL;
    }



    if(isUpdateNeeded)
    {
        //启动外部复制程序
        QProcess *copproc = new QProcess();
        copproc->start("Copy.exe");
    }
}

void UBChain::updateJsonDataMap(QString id, QString data)
{
    mutexForJsonData.lock();

    jsonDataMap.insert( id, data);
    emit jsonDataUpdated(id);

    mutexForJsonData.unlock();

}

QString UBChain::jsonDataValue(QString id)
{

    mutexForJsonData.lock();

    QString value = jsonDataMap.value(id);

    mutexForJsonData.unlock();

    return value;
}

double UBChain::getPendingAmount(QString name)
{
    mutexForConfigFile.lock();
    if( !pendingFile->open(QIODevice::ReadOnly))
    {
        qDebug() << "pending.dat not exist";
        return 0;
    }
    QString str = QByteArray::fromBase64( pendingFile->readAll());
    pendingFile->close();
    QStringList strList = str.split(";");
    strList.removeLast();

    mutexForConfigFile.unlock();

    double amount = 0;
    foreach (QString ss, strList)
    {
        QStringList sList = ss.split(",");
        if( sList.at(1) == name)
        {
            amount += sList.at(2).toDouble() + sList.at(3).toDouble();
        }
    }

    return amount;
}

QString UBChain::getPendingInfo(QString id)
{
    mutexForConfigFile.lock();
    if( !pendingFile->open(QIODevice::ReadOnly))
    {
        qDebug() << "pending.dat not exist";
        return 0;
    }
    QString str = QByteArray::fromBase64( pendingFile->readAll());
    pendingFile->close();
    QStringList strList = str.split(";");
    strList.removeLast();

    mutexForConfigFile.unlock();

    QString info;
    foreach (QString ss, strList)
    {
        QStringList sList = ss.split(",");
        if( sList.at(0) == id)
        {
            info = ss;
            break;
        }
    }

    return info;
}

QString doubleTo5Decimals(double number)
{
        QString num = QString::number(number,'f', 5);
        int pos = num.indexOf('.') + 6;
        return num.mid(0,pos);
}

QString UBChain::registerMapValue(QString key)
{
    mutexForRegisterMap.lock();
    QString value = registerMap.value(key);
    mutexForRegisterMap.unlock();

    return value;
}

void UBChain::registerMapInsert(QString key, QString value)
{
    mutexForRegisterMap.lock();
    registerMap.insert(key,value);
    mutexForRegisterMap.unlock();
}

int UBChain::registerMapRemove(QString key)
{
    mutexForRegisterMap.lock();
    int number = registerMap.remove(key);
    mutexForRegisterMap.unlock();
    return number;
}

QString UBChain::balanceMapValue(QString key)
{
    mutexForBalanceMap.lock();
    QString value = balanceMap.value(key);
    mutexForBalanceMap.unlock();

    return value;
}

void UBChain::balanceMapInsert(QString key, QString value)
{
    mutexForBalanceMap.lock();
    balanceMap.insert(key,value);
    mutexForBalanceMap.unlock();
}

int UBChain::balanceMapRemove(QString key)
{
    mutexForBalanceMap.lock();
    int number = balanceMap.remove(key);
    mutexForBalanceMap.unlock();
    return number;
}

TwoAddresses UBChain::addressMapValue(QString key)
{
    mutexForAddressMap.lock();
    TwoAddresses value = addressMap.value(key);
    mutexForAddressMap.unlock();

    return value;
}

void UBChain::addressMapInsert(QString key, TwoAddresses value)
{
    mutexForAddressMap.lock();
    addressMap.insert(key,value);
    mutexForAddressMap.unlock();
}

int UBChain::addressMapRemove(QString key)
{
    mutexForAddressMap.lock();
    int number = addressMap.remove(key);
    mutexForAddressMap.unlock();
    return number;
}

//bool UBChain::rpcReceivedOrNotMapValue(QString key)
//{
//    mutexForRpcReceiveOrNot.lock();
//    bool received = rpcReceivedOrNotMap.value(key);
//    mutexForRpcReceiveOrNot.unlock();
//    return received;
//}

//void UBChain::rpcReceivedOrNotMapSetValue(QString key, bool received)
//{
//    mutexForRpcReceiveOrNot.lock();
//    rpcReceivedOrNotMap.insert(key, received);
//    mutexForRpcReceiveOrNot.unlock();
//}


void UBChain::appendCurrentDialogVector( QWidget * w)
{
    currentDialogVector.append(w);
}

void UBChain::removeCurrentDialogVector( QWidget * w)
{
    currentDialogVector.removeAll(w);
}

void UBChain::hideCurrentDialog()
{
    foreach (QWidget* w, currentDialogVector)
    {
        w->hide();
    }
}

void UBChain::showCurrentDialog()
{
    foreach (QWidget* w, currentDialogVector)
    {
        qDebug() << "showCurrentDialog :" << w;
        w->show();
        w->move( mainFrame->pos());  // lock界面可能会移动，重新显示的时候要随之移动
    }
}

void UBChain::resetPosOfCurrentDialog()
{
    foreach (QWidget* w, currentDialogVector)
    {
        w->move( mainFrame->pos());
    }
}

void UBChain::initWebSocketManager()
{
    wsManager = new WebSocketManager;
    wsManager->start();
    wsManager->moveToThread(wsManager);

    connect(this, SIGNAL(rpcPosted(QString,QString)), wsManager, SLOT(processRPCs(QString,QString)));

}

void UBChain::postRPC(QString _rpcId, QString _rpcCmd)
{
//    if( rpcReceivedOrNotMap.contains( _rpcId))
//    {
//        if( rpcReceivedOrNotMap.value( _rpcId) == true)
//        {
//            rpcReceivedOrNotMapSetValue( _rpcId, false);
//            emit rpcPosted(_rpcId, _rpcCmd);
//        }
//        else
//        {
//            // 如果标识为未返回 则不重复排入事件队列
//            return;
//        }
//    }
//    else
//    {
//        rpcReceivedOrNotMapSetValue( _rpcId, false);

//        emit rpcPosted(_rpcId, _rpcCmd);
//    }

    emit rpcPosted(_rpcId, _rpcCmd);
}

double roundDown(double decimal, int precision)
{
    int precisonFigureNum   = QString::number( precision, 'g', 15).count() - 1;

    double result = QString::number(decimal,'f',precisonFigureNum).toDouble();
    if( result > decimal)
    {
        if( precision == 0)     precision = 1;
        result = result - 1.0 / precision;
    }

    return result;
}

bool isExistInWallet(QString strName)
{
    mutexForAddressMap.lock();
    for (QMap<QString,TwoAddresses>::const_iterator i = UBChain::getInstance()->addressMap.constBegin(); i != UBChain::getInstance()->addressMap.constEnd(); ++i)
    {
        if(i.key() == strName)
        {
            mutexForAddressMap.unlock();
            return true;
        }
    }
    mutexForAddressMap.unlock();
    return false;
}

QString removeLastZeros(QString number)     // 去掉小数最后面的0
{
    if( !number.contains('.'))  return number;

    while (number.endsWith('0'))
    {
        number.chop(1);
    }

    if( number.endsWith('.'))   number.chop(1);

    return number;
}

QString removeFrontZeros(QString number)     // 去掉整数最前面的0
{
    while (number.startsWith('0'))
    {
        number.remove(0,1);
    }

    if( number.isEmpty())   number = "0";

    return number;
}

QString getBigNumberString(unsigned long long number, int precision)
{
    QString str = QString::number(number);
    int size = precision;
    if( size < 0)  return "";
    if( size == 0) return str;

    if( str.size() > size)
    {
        str.insert(str.size() - size, '.');
    }
    else
    {
        // 前面需要补0
        QString zeros;
        zeros.fill('0',size - str.size() + 1);
        str.insert(0,zeros);
        str.insert(1,'.');
    }

    return removeLastZeros(str);
}


QString decimalToIntegerStr(QString number, int precision)
{
    int pos = number.indexOf(".");
    if( pos == -1)  pos = number.size();

    number.remove(".");
    int size = number.size();

    QString zeroStr;
    zeroStr.fill('0', precision - (size - pos) );

    number.append(zeroStr);
    number = number.mid(0, pos + precision);     // 万一原数据小数位数超过precision  舍去超过的部分

    return removeFrontZeros(number);
}

AddressType checkAddress(QString address, AddressFlags type)
{
    if(type & AccountAddress)
    {
        if(address.startsWith(ACCOUNT_ADDRESS_PREFIX)  && (address.size() == QString(ACCOUNT_ADDRESS_PREFIX).size() + 33 ||
                                                           address.size() == QString(ACCOUNT_ADDRESS_PREFIX).size() + 32))
        {
            return AccountAddress;
        }
    }

    if(type & ContractAddress)
    {
        if(address.startsWith("C")  && address.size() == QString("C").size() + 33)
        {
            return ContractAddress;
        }
    }

    if(type & MultiSigAddress)
    {
        if(address.startsWith("M")  && address.size() == QString("M").size() + 33)
        {
            return MultiSigAddress;
        }
    }

    if(type & ScriptAddress)
    {
        if(address.startsWith("S")  && address.size() == QString("S").size() + 33)
        {
            return ScriptAddress;
        }
    }

    return InvalidAddress;
}

void moveWidgetToScreenCenter(QWidget *w)
{
    QRect rect = QApplication::desktop()->availableGeometry();
    w->setGeometry( (rect.width() - w->width()) / 2, (rect.height() - w->height()) / 2,
                 w->width(), w->height());
}

QString toJsonFormat(QString instruction,
                      QJsonArray parameters)
{
    QJsonObject object;
    object.insert("id", 32800);
    object.insert("method", instruction);
    object.insert("params",parameters);

    return QJsonDocument(object).toJson();
}

QDataStream &operator >>(QDataStream &in, TransactionStruct &data)
{
    in >> data.transactionId >> data.type >> data.blockNum >> data.expirationTime >> data.operationStr >> data.feeAmount;
    return in;
}

QDataStream &operator <<(QDataStream &out, const TransactionStruct &data)
{
    out << data.transactionId << data.type << data.blockNum << data.expirationTime << data.operationStr << data.feeAmount;
    return out;
}

QDataStream &operator >>(QDataStream &in, TransactionTypeId &data)
{
    in >> data.type >> data.transactionId;
    return in;

}

QDataStream &operator <<(QDataStream &out, const TransactionTypeId &data)
{
    out << data.type << data.transactionId;
    return out;

}

unsigned long long jsonValueToULL(QJsonValue v)
{
    unsigned long long result = 0;
    if(v.isString())
    {
        result = v.toString().toULongLong();
    }
    else
    {
        result = QString::number(v.toDouble(),'g',12).toULongLong();
    }

    return result;
}

bool operator ==(const ContractInfo &c1, const ContractInfo &c2)
{
    return c1.contractAddress == c2.contractAddress;
}