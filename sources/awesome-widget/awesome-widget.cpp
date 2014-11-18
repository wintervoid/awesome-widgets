/***************************************************************************
 *   This file is part of awesome-widgets                                  *
 *                                                                         *
 *   awesome-widgets is free software: you can redistribute it and/or      *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   awesome-widgets is distributed in the hope that it will be useful,    *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with awesome-widgets. If not, see http://www.gnu.org/licenses/  *
 ***************************************************************************/

#include "awesome-widget.h"

#include <KStandardDirs>
#include <Plasma/ToolTipManager>
#include <QDesktopServices>
#include <QDir>
#include <QGraphicsGridLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QTextCodec>
#include <QTimer>

#include "customlabel.h"
#include "graphicalitem.h"
#include <pdebug/pdebug.h>
#include <task/taskadds.h>
#include "version.h"


AwesomeWidget::AwesomeWidget(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
    // debug
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QString debugEnv = environment.value(QString("DEBUG"), QString("no"));
    if (debugEnv == QString("yes"))
        debug = true;
    else
        debug = false;

    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    // text format init
    formatLine.append(QString(""));
    formatLine.append(QString(""));
}


AwesomeWidget::~AwesomeWidget()
{
    if (debug) qDebug() << PDEBUG;
}


void AwesomeWidget::checkUpdates()
{
    if (debug) qDebug() << PDEBUG;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyRecieved(QNetworkReply *)));
    manager->get(QNetworkRequest(QUrl(VERSION_API)));
}


QList<QAction *> AwesomeWidget::contextualActions()
{
    if (debug) qDebug() << PDEBUG;

    return contextMenu;
}


void AwesomeWidget::createActions()
{
    if (debug) qDebug() << PDEBUG;

    contextMenu.clear();
    contextMenu.append(new QAction(QIcon::fromTheme(QString("utilities-system-monitor")),
                                   i18n("Run ksysguard"), this));
    connect(contextMenu[0], SIGNAL(triggered(bool)), this, SLOT(showKsysguard()));
    contextMenu.append(new QAction(QIcon::fromTheme(QString("text-x-readme")),
                                   i18n("Show README"), this));
    connect(contextMenu[1], SIGNAL(triggered(bool)), this, SLOT(showReadme()));
    contextMenu.append(new QAction(QIcon::fromTheme(QString("stock-refresh")),
                                   i18n("Update text"), this));
    connect(contextMenu[2], SIGNAL(triggered(bool)), this, SLOT(updateNetworkDevice()));
    connect(contextMenu[2], SIGNAL(triggered(bool)), extsysmonEngine, SLOT(updateAllSources()));
    connect(contextMenu[2], SIGNAL(triggered(bool)), sysmonEngine, SLOT(updateAllSources()));
    contextMenu.append(new QAction(QIcon::fromTheme(QString("system-software-update")),
                                   i18n("Check for updates"), this));
    connect(contextMenu[3], SIGNAL(triggered(bool)), this, SLOT(checkUpdates()));
}


QString AwesomeWidget::getNetworkDevice()
{
    if (debug) qDebug() << PDEBUG;

    QString device = QString("lo");
    if (configuration[QString("useCustomNetdev")].toInt() == 2)
        device = configuration[QString("customNetdev")];
    else {
        QList<QNetworkInterface> rawInterfaceList = QNetworkInterface::allInterfaces();
        for (int i=0; i<rawInterfaceList.count(); i++)
            if ((rawInterfaceList[i].flags().testFlag(QNetworkInterface::IsUp)) &&
                    (!rawInterfaceList[i].flags().testFlag(QNetworkInterface::IsLoopBack)) &&
                    (!rawInterfaceList[i].flags().testFlag(QNetworkInterface::IsPointToPoint)))
                device = rawInterfaceList[i].name();
    }

    return device;
}


int AwesomeWidget::getNumberCpus()
{
    if (debug) qDebug() << PDEBUG;

    QString cmd = QString("grep -c ^processor /proc/cpuinfo");
    if (debug) qDebug() << PDEBUG << ":" << "cmd" << cmd;
    TaskResult process = runTask(cmd);
    if (debug) qDebug() << PDEBUG << ":" << "Cmd returns" << process.exitCode;
    if (process.exitCode != 0)
        if (debug) qDebug() << PDEBUG << ":" << "Error" << process.error;
    return QTextCodec::codecForMib(106)->toUnicode(process.output).trimmed().toInt();
}


float AwesomeWidget::getTemp(const float temp)
{
    if (debug) qDebug() << PDEBUG;

    float convertedTemp = temp;
    if (configuration[QString("tempUnits")] == QString("Celsius"))
        ;
    else if (configuration[QString("tempUnits")] == QString("Fahrenheit"))
        convertedTemp = temp * 9.0 / 5.0 + 32.0;
    else if (configuration[QString("tempUnits")] == QString("Kelvin"))
        convertedTemp = temp + 273.15;
    else if (configuration[QString("tempUnits")] == QString("Reaumur"))
        convertedTemp = temp * 0.8;
    else if (configuration[QString("tempUnits")] == QString("cm^-1"))
        convertedTemp = (temp + 273.15) * 0.695;
    else if (configuration[QString("tempUnits")] == QString("kJ/mol"))
        convertedTemp = (temp + 273.15) * 8.31;
    else if (configuration[QString("tempUnits")] == QString("kcal/mol"))
        convertedTemp = (temp + 273.15) * 1.98;

    return convertedTemp;
}


QStringList AwesomeWidget::getTimeKeys()
{
    if (debug) qDebug() << PDEBUG;

    QStringList timeKeys;
    timeKeys.append(QString("dddd"));
    timeKeys.append(QString("ddd"));
    timeKeys.append(QString("dd"));
    timeKeys.append(QString("d"));
    timeKeys.append(QString("MMMM"));
    timeKeys.append(QString("MMM"));
    timeKeys.append(QString("MM"));
    timeKeys.append(QString("M"));
    timeKeys.append(QString("yyyy"));
    timeKeys.append(QString("yy"));
    timeKeys.append(QString("hh"));
    timeKeys.append(QString("h"));
    timeKeys.append(QString("mm"));
    timeKeys.append(QString("m"));
    timeKeys.append(QString("ss"));
    timeKeys.append(QString("s"));

    return timeKeys;
}


void AwesomeWidget::init()
{
    if (debug) qDebug() << PDEBUG;

    // dataengines
    extsysmonEngine = dataEngine(QString("ext-sysmon"));
    sysmonEngine = dataEngine(QString("systemmonitor"));
    connect(sysmonEngine, SIGNAL(sourceAdded(QString)), this, SLOT(addDiskDevice(QString)));
    timeEngine = dataEngine(QString("time"));

    // tooltip
    toolTip = Plasma::ToolTipContent();
    toolTip.setMainText(QString("Awesome Widget"));
    toolTip.setSubText(QString(""));
    toolTipScene = new QGraphicsScene();
    toolTipView = new QGraphicsView(toolTipScene);
    toolTipView->setStyleSheet(QString("background: transparent"));
    toolTipView->setContentsMargins(0, 0, 0, 0);
    toolTipView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    toolTipView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    Plasma::ToolTipManager::self()->setContent(this, toolTip);

    // body
    createActions();
    mainLayout = new QGraphicsGridLayout();
    mainLayout->setContentsMargins(1, 1, 1, 1);
    setLayout(mainLayout);
    textLabel = new CustomLabel(this, debug);
    textLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    QGraphicsProxyWidget *pw = new QGraphicsProxyWidget(this);
    pw->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    textLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    pw->setWidget(textLabel);
    mainLayout->addItem(pw, 0, 0);

    // read variables
    configChanged();
    timer = new QTimer(this);
    timer->setSingleShot(false);
    timer->setInterval(configuration[QString("interval")].toInt());
    connect(timer, SIGNAL(timeout()), this, SLOT(updateText()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateTooltip()));
    timer->start();
    // check for updates
    connect(this, SIGNAL(thereIsUpdates(QString)), this, SLOT(showUpdates(QString)));
    if (configuration[QString("checkUpdates")].toInt() == 2)
        checkUpdates();
}


void AwesomeWidget::replyRecieved(QNetworkReply *reply)
{
    if (debug) qDebug() << PDEBUG;

    QString answer = reply->readAll();
    if (!answer.contains(QString("tag_name"))) return;
    QString version = QString(VERSION);
    if (debug) qDebug() << PDEBUG << answer;
    for (int i=0; i<answer.split(QString("tag_name")).count(); i++) {
        version = answer.split(QString("tag_name"))[1].split(QChar(','))[0];
        version.remove(QChar('"'));
        version.remove(QChar(':'));
        version.remove(QString("V."));
        break;
    }
    if (version != QString(VERSION))
        emit(thereIsUpdates(version));
}


void AwesomeWidget::addDiskDevice(const QString source)
{
    if (debug) qDebug() << PDEBUG;

    QRegExp diskRegexp = QRegExp(QString("disk/(?:md|sd|hd)[a-z|0-9]_.*/Rate/(?:rblk)"));
    if (diskRegexp.indexIn(source) > -1) {
        QStringList splitSource = source.split(QChar('/'));
        QString device = splitSource[0] + QString("/") + splitSource[1];
        diskDevices.append(device);
    }
}


QStringList AwesomeWidget::findKeys()
{
    if (debug) qDebug() << PDEBUG;

    QStringList selectedKeys;
    for (int i=0; i<keys.count(); i++)
        if (configuration[QString("text")].contains(QString("$") + keys[i])) {
            if (debug) qDebug() << PDEBUG << ":" << "Found key" << keys[i];
            selectedKeys.append(keys[i]);
        }

    return selectedKeys;
}


QStringList AwesomeWidget::getKeys()
{
    if (debug) qDebug() << PDEBUG;

    QStringList allKeys;
    // time
    allKeys.append(QString("time"));
    allKeys.append(QString("isotime"));
    allKeys.append(QString("shorttime"));
    allKeys.append(QString("longtime"));
    allKeys.append(QString("ctime"));
    // uptime
    allKeys.append(QString("uptime"));
    allKeys.append(QString("cuptime"));
    // cpuclock
    for (int i=counts[QString("cpu")]-1; i>=0; i--)
        allKeys.append(QString("cpucl") + QString::number(i));
    allKeys.append(QString("cpucl"));
    // cpu
    for (int i=counts[QString("cpu")]-1; i>=0; i--)
        allKeys.append(QString("cpu") + QString::number(i));
    allKeys.append(QString("cpu"));
    // temperature
    for (int i=counts[QString("temp")]-1; i>=0; i--)
        allKeys.append(QString("temp") + QString::number(i));
    for (int i=counts[QString("fan")]-1; i>=0; i--)
        allKeys.append(QString("fan") + QString::number(i));
    // gputemp
    allKeys.append(QString("gputemp"));
    // gpu
    allKeys.append(QString("gpu"));
    // memory
    allKeys.append(QString("memmb"));
    allKeys.append(QString("memgb"));
    allKeys.append(QString("memfreemb"));
    allKeys.append(QString("memfreegb"));
    allKeys.append(QString("memtotmb"));
    allKeys.append(QString("memtotgb"));
    allKeys.append(QString("memusedmb"));
    allKeys.append(QString("memusedgb"));
    allKeys.append(QString("mem"));
    // swap
    allKeys.append(QString("swapmb"));
    allKeys.append(QString("swapgb"));
    allKeys.append(QString("swapfreemb"));
    allKeys.append(QString("swapfreegb"));
    allKeys.append(QString("swaptotmb"));
    allKeys.append(QString("swaptotgb"));
    allKeys.append(QString("swap"));
    // hdd
    for (int i=counts[QString("mount")]-1; i>=0; i--) {
        allKeys.append(QString("hddmb") + QString::number(i));
        allKeys.append(QString("hddgb") + QString::number(i));
        allKeys.append(QString("hddfreemb") + QString::number(i));
        allKeys.append(QString("hddfreegb") + QString::number(i));
        allKeys.append(QString("hddtotmb") + QString::number(i));
        allKeys.append(QString("hddtotgb") + QString::number(i));
        allKeys.append(QString("hdd") + QString::number(i));
    }
    // hdd speed
    for (int i=counts[QString("disk")]-1; i>=0; i--) {
        allKeys.append(QString("hddr") + QString::number(i));
        allKeys.append(QString("hddw") + QString::number(i));
    }
    // hdd temp
    for (int i=counts[QString("hddtemp")]-1; i>=0; i--) {
        allKeys.append(QString("hddtemp") + QString::number(i));
        allKeys.append(QString("hddtemp") + QString::number(i));
    }
    // network
    allKeys.append(QString("down"));
    allKeys.append(QString("up"));
    allKeys.append(QString("netdev"));
    // battery
    allKeys.append(QString("ac"));
    for (int i=counts[QString("bat")]-1; i>=0; i--)
        allKeys.append(QString("bat") + QString::number(i));
    allKeys.append(QString("bat"));
    // player
    allKeys.append(QString("album"));
    allKeys.append(QString("artist"));
    allKeys.append(QString("duration"));
    allKeys.append(QString("progress"));
    allKeys.append(QString("title"));
    // ps
    allKeys.append(QString("pscount"));
    allKeys.append(QString("pstotal"));
    allKeys.append(QString("ps"));
    // package manager
    for (int i=counts[QString("pkg")]-1; i>=0; i--)
        allKeys.append(QString("pkgcount") + QString::number(i));
    // custom
    for (int i=counts[QString("custom")]-1; i>=0; i--)
        allKeys.append(QString("custom") + QString::number(i));
    // desktop
    allKeys.append(QString("desktop"));
    allKeys.append(QString("ndesktop"));
    allKeys.append(QString("tdesktops"));

    return allKeys;
}


QStringList AwesomeWidget::findGraphicalItems()
{
    if (debug) qDebug() << PDEBUG;

    QStringList orderedKeys;
    for (int i=0; i<graphicalItems.count(); i++)
        orderedKeys.append(graphicalItems[i]->getName() + graphicalItems[i]->getBar());
    orderedKeys.sort();
    QStringList selectedKeys;
    for (int i=orderedKeys.count()-1; i>=0; i--)
        if (configuration[QString("text")].contains(QString("$") + orderedKeys[i])) {
            if (debug) qDebug() << PDEBUG << ":" << "Found key" << orderedKeys[i];
            selectedKeys.append(orderedKeys[i]);
        }

    return selectedKeys;
}


void AwesomeWidget::getGraphicalItems()
{
    if (debug) qDebug() << PDEBUG;

    graphicalItems.clear();
    // create directory at $HOME
    QString localDir = KStandardDirs::locateLocal("data", "plasma_applet_awesome-widget/destkops");
    if (KStandardDirs::makeDir(localDir))
        if (debug) qDebug() << PDEBUG << ":" << "Created directory" << localDir;

    QStringList dirs = KGlobal::dirs()->findDirs("data", "plasma_applet_awesome-widget/desktops");
    QStringList names;
    for (int i=0; i<dirs.count(); i++) {
        QStringList files = QDir(dirs[i]).entryList(QDir::Files, QDir::Name);
        for (int j=0; j<files.count(); j++) {
            if (!files[j].endsWith(QString(".desktop"))) continue;
            if (names.contains(files[j])) continue;
            if (debug) qDebug() << PDEBUG << ":" << "Found file" << files[j] << "in" << dirs[i];
            names.append(files[j]);
            graphicalItems.append(new GraphicalItem(0, files[j], dirs, debug));
        }
    }
}


GraphicalItem *AwesomeWidget::getItemByTag(const QString tag)
{
    if (debug) qDebug() << PDEBUG;

    GraphicalItem *item = nullptr;
    for (int i=0; i< graphicalItems.count(); i++) {
        if ((graphicalItems[i]->getName() + graphicalItems[i]->getBar()) != tag) continue;
        item = graphicalItems[i];
        break;
    }

    return item;
}


void AwesomeWidget::showKsysguard()
{
    if (debug) qDebug() << PDEBUG;

    QString cmd = QString("ksysguard");
    if (debug) qDebug() << PDEBUG << ":" << "cmd" << cmd;
    TaskResult process = runTask(cmd);
    if (debug) qDebug() << PDEBUG << ":" << "Cmd returns" << process.exitCode;
    if (process.exitCode != 0)
        if (debug) qDebug() << PDEBUG << ":" << "Error" << process.error;
}


void AwesomeWidget::showReadme()
{
    if (debug) qDebug() << PDEBUG;

    QDesktopServices::openUrl(QString("http://arcanis.name/projects/awesome-widgets/"));
}


void AwesomeWidget::showUpdates(QString version)
{
    if (debug) qDebug() << PDEBUG;

    QString text;
    text += i18n("Current version : %1", QString(VERSION)) + QString("\n");
    text += i18n("New version : %2", version) + QString("\n");
    text += i18n("Click \"Ok\" to download");
    int select = QMessageBox::information(0, i18n("There are updates"), text, QMessageBox::Ok | QMessageBox::Cancel);
    switch(select) {
    case QMessageBox::Ok:
        QDesktopServices::openUrl(QString(RELEASES) + version);
        break;
    }
}


K_EXPORT_PLASMA_APPLET(awesome-widget, AwesomeWidget)
