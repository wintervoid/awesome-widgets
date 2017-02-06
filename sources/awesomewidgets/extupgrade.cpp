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

#include "extupgrade.h"
#include "ui_extupgrade.h"

#include <KI18n/KLocalizedString>

#include <QDir>
#include <QRegExp>
#include <QSettings>
#include <QTextCodec>

#include "awdebug.h"


ExtUpgrade::ExtUpgrade(QWidget *parent, const QString filePath)
    : AbstractExtItem(parent, filePath)
    , ui(new Ui::ExtUpgrade)
{
    qCDebug(LOG_LIB) << __PRETTY_FUNCTION__;

    if (!filePath.isEmpty())
        readConfiguration();
    ui->setupUi(this);
    translate();

    m_values[tag(QString("pkgcount"))] = 0;

    m_process = new QProcess(nullptr);
    connect(m_process, SIGNAL(finished(int)), this, SLOT(updateValue()));
    m_process->waitForFinished(0);

    connect(this, SIGNAL(socketActivated()), this, SLOT(startProcess()));
}


ExtUpgrade::~ExtUpgrade()
{
    qCDebug(LOG_LIB) << __PRETTY_FUNCTION__;

    m_process->kill();
    m_process->deleteLater();
    disconnect(this, SIGNAL(socketActivated()), this, SLOT(startProcess()));
    delete ui;
}


ExtUpgrade *ExtUpgrade::copy(const QString _fileName, const int _number)
{
    qCDebug(LOG_LIB) << "File" << _fileName << "with number" << _number;

    ExtUpgrade *item
        = new ExtUpgrade(static_cast<QWidget *>(parent()), _fileName);
    copyDefaults(item);
    item->setExecutable(executable());
    item->setFilter(filter());
    item->setNumber(_number);
    item->setNull(null());

    return item;
}


QString ExtUpgrade::executable() const
{
    return m_executable;
}


QString ExtUpgrade::filter() const
{
    return m_filter;
}


int ExtUpgrade::null() const
{
    return m_null;
}


QString ExtUpgrade::uniq() const
{
    return executable();
}


void ExtUpgrade::setExecutable(const QString _executable)
{
    qCDebug(LOG_LIB) << "Executable" << _executable;

    m_executable = _executable;
}


void ExtUpgrade::setFilter(const QString _filter)
{
    qCDebug(LOG_LIB) << "Filter" << _filter;

    m_filter = _filter;
}


void ExtUpgrade::setNull(const int _null)
{
    qCDebug(LOG_LIB) << "Null lines" << _null;
    if (_null < 0)
        return;

    m_null = _null;
}


void ExtUpgrade::readConfiguration()
{
    AbstractExtItem::readConfiguration();

    QSettings settings(fileName(), QSettings::IniFormat);

    settings.beginGroup(QString("Desktop Entry"));
    setExecutable(settings.value(QString("Exec"), executable()).toString());
    setNull(settings.value(QString("X-AW-Null"), null()).toInt());
    // api == 3
    setFilter(settings.value(QString("X-AW-Filter"), filter()).toString());
    settings.endGroup();

    bumpApi(AW_EXTUPGRADE_API);
}


QVariantHash ExtUpgrade::run()
{
    if (!isActive())
        return m_values;

    if (m_times == 1)
        startProcess();

    // update value
    if (m_times >= interval())
        m_times = 0;
    m_times++;

    return m_values;
}


int ExtUpgrade::showConfiguration(const QVariant args)
{
    Q_UNUSED(args)

    ui->lineEdit_name->setText(name());
    ui->lineEdit_comment->setText(comment());
    ui->label_numberValue->setText(QString("%1").arg(number()));
    ui->lineEdit_command->setText(executable());
    ui->lineEdit_filter->setText(filter());
    ui->checkBox_active->setCheckState(isActive() ? Qt::Checked
                                                  : Qt::Unchecked);
    ui->spinBox_null->setValue(null());
    ui->spinBox_interval->setValue(interval());

    int ret = exec();
    if (ret != 1)
        return ret;
    setName(ui->lineEdit_name->text());
    setComment(ui->lineEdit_comment->text());
    setNumber(ui->label_numberValue->text().toInt());
    setApiVersion(AW_EXTUPGRADE_API);
    setExecutable(ui->lineEdit_command->text());
    setFilter(ui->lineEdit_filter->text());
    setActive(ui->checkBox_active->checkState() == Qt::Checked);
    setNull(ui->spinBox_null->value());
    setInterval(ui->spinBox_interval->value());

    writeConfiguration();
    return ret;
}


void ExtUpgrade::writeConfiguration() const
{
    AbstractExtItem::writeConfiguration();

    QSettings settings(writtableConfig(), QSettings::IniFormat);
    qCInfo(LOG_LIB) << "Configuration file" << settings.fileName();

    settings.beginGroup(QString("Desktop Entry"));
    settings.setValue(QString("Exec"), executable());
    settings.setValue(QString("X-AW-Filter"), filter());
    settings.setValue(QString("X-AW-Null"), null());
    settings.endGroup();

    settings.sync();
}


void ExtUpgrade::startProcess()
{
    QString cmd = QString("sh -c \"%1\"").arg(executable());
    qCInfo(LOG_LIB) << "Run cmd" << cmd;
    m_process->start(cmd);
}


void ExtUpgrade::updateValue()
{
    qCInfo(LOG_LIB) << "Cmd returns" << m_process->exitCode();
    qCInfo(LOG_LIB) << "Error" << m_process->readAllStandardError();

    QString qoutput = QTextCodec::codecForMib(106)
                          ->toUnicode(m_process->readAllStandardOutput())
                          .trimmed();
    m_values[tag(QString("pkgcount"))] = [this](QString output) {
        return filter().isEmpty()
                   ? output.split(QChar('\n'), QString::SkipEmptyParts).count()
                         - null()
                   : output.split(QChar('\n'), QString::SkipEmptyParts)
                         .filter(QRegExp(filter()))
                         .count();
    }(qoutput);

    emit(dataReceived(m_values));
}


bool ExtUpgrade::canRun()
{
    return ((isActive()) && (m_process->state() == QProcess::NotRunning)
            && (socket().isEmpty()));
}


void ExtUpgrade::translate()
{
    ui->label_name->setText(i18n("Name"));
    ui->label_comment->setText(i18n("Comment"));
    ui->label_number->setText(i18n("Tag"));
    ui->label_command->setText(i18n("Command"));
    ui->label_filter->setText(i18n("Filter"));
    ui->checkBox_active->setText(i18n("Active"));
    ui->label_null->setText(i18n("Null"));
    ui->label_interval->setText(i18n("Interval"));
}
