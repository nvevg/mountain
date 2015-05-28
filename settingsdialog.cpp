#include "settingsdialog.h"
#include "ui_settingsdialog.h"

const QString DEFAULT_VIEW_COMMAND = "xdg-open %m";
const QString DEFAULT_DEVICE_FORMAT_STRING = "%n (%f) on %m";

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    m_pSettings = new QSettings("Vladislav Nickolaev", "MOUNTain", this);
    QObject::connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(slotSettingsAccepted()));
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(slotSettingsRejected()));
    setWindowTitle("Settings");
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

const QSettings * SettingsDialog::settings()
{
    return m_pSettings;
}

void SettingsDialog::showEvent(QShowEvent *pe)
{
    readSettings();
}

void SettingsDialog::slotSettingsAccepted()
{
    writeSettings();
    emit settingsAccepted();
    hide();
}

void SettingsDialog::slotSettingsRejected()
{
    hide();
}

void SettingsDialog::writeSettings()
{
    m_pSettings->beginGroup("/Settings/Notifications");

    m_pSettings->setValue("ShowAdded", ui->showAddedCBox->isChecked());
    m_pSettings->setValue("ShowRemoved", ui->showRemovedCBox->isChecked());
    m_pSettings->setValue("ShowMounted", ui->showMountedCBox->isChecked());
    m_pSettings->setValue("ShowUnmounted", ui->showUnmountedCBox->isChecked());

    m_pSettings->endGroup();

    m_pSettings->beginGroup("/Settings/Actions");

    m_pSettings->setValue("MountAdded", ui->mountAddedCBox->isChecked());
    m_pSettings->setValue("ShowSystemInternal", ui->showInternalCBox->isChecked());
    m_pSettings->setValue("ExecuteViewMounted", ui->viewMountedCBox->isChecked());
    m_pSettings->setValue("ForceUnmount", ui->forceUnmountCBox->isChecked());
    m_pSettings->setValue("ViewCommand", ui->viewCommandEdit->text().isEmpty() ? DEFAULT_VIEW_COMMAND : ui->viewCommandEdit->text());
    m_pSettings->setValue("DeviceFormatString", ui->formatStringEdit->text().isEmpty() ? DEFAULT_DEVICE_FORMAT_STRING : ui->formatStringEdit->text());

    m_pSettings->endGroup();

}

void SettingsDialog::readSettings()
{
    m_pSettings->beginGroup("/Settings/Notifications");

    ui->showAddedCBox->setChecked(m_pSettings->value("ShowAdded", true).toBool());
    ui->showRemovedCBox->setChecked(m_pSettings->value("ShowRemoved", true).toBool());
    ui->showMountedCBox->setChecked(m_pSettings->value("ShowMounted", true).toBool());
    ui->showUnmountedCBox->setChecked(m_pSettings->value("ShowUnmounted", true).toBool());

    m_pSettings->endGroup();

    m_pSettings->beginGroup("/Settings/Actions");

    ui->mountAddedCBox->setChecked(m_pSettings->value("MountAdded", true).toBool());
    ui->showInternalCBox->setChecked(m_pSettings->value("ShowSystemInternal", true).toBool());
    ui->viewMountedCBox->setChecked(m_pSettings->value("ExecuteViewMounted", true).toBool());
    ui->forceUnmountCBox->setChecked(m_pSettings->value("ForceUnmount", true).toBool());
    ui->viewCommandEdit->setText(m_pSettings->value("ViewCommand").toString());
    ui->formatStringEdit->setText(m_pSettings->value("DeviceFormatString", DEFAULT_DEVICE_FORMAT_STRING).toString());

    m_pSettings->endGroup();
}
