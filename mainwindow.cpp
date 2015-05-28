#include <QProcess>
#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace Utils
{
QString formatDiskSize(unsigned long long s)
{
    int i = 0;
    static QStringList units{"B", "Kb", "Mb", "Gb", "Tb"};
    double res_size = s;

    while (res_size > 1024 && i < units.size())
    {
        res_size /= 1024;
        i++;
    }

    return QString::number(res_size, 'f', 2) + units.at(i);
}

QString getDeviceTypeStr(const DeviceInfo &d)
{
    //HDD, USB, FLOPPY, OPTICAL, OTHER
    static QString dev_names[] = { "Internal disk", "USB disk", "Floppy disk", "Optical disk", "Unknown device" };
    return dev_names[d.type];
}

QString formatDeviceStr(QString str, DeviceInfo dev)
{
    int from = 0;
    int f;

    while ((f = str.indexOf("%", from)) != -1)
    {
        if (f - 1 >= 0 && str.at(f - 1) == '\\')
        {
            ++from;
            continue;
        }

        QString spec = str.mid(f + 1, 1);
        QString val;

        if ("n" == spec)
            val = dev.name;
        else if ("u" == spec)
            val = dev.uuid;
        else if ("s" == spec)
            val = formatDiskSize(dev.sizeBytes);
        else if ("a" == spec)
            val = dev.isMounted ? "mounted" : "unmounted";
        else if ("i" == spec)
            val = dev.isSystem ? "internal disk" : "external disk";
        else if ("m" == spec)
            val = dev.mountPoint;
        else if ("p" == spec)
            val = dev.udisksPath;
        else if ("f" == spec)
            val = dev.fileName;
        else if ("e" == spec)
            val = dev.fileSystem;
        else val = getDeviceTypeStr(dev);

        str.remove(f, 2);
        str.insert(f, val);
        from = f + val.length() + 1;
    }
    return str;
}

QString mapErrorText(ErrorCode err)
{
    //DBusError, NotAuthorized, Busy, Failed, Cancelled, UnknownFileSystem, InvalidRequest
    static QString err_names[] = { "DBus error occured.",  "User is not authorized to perform this operation.",
                               "Device is busy.", "Operation failed.", "Request cancelled.",
                               "Unknown file system.", "Unknown error." };
    return err_names[err];
}

}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setVisible(false);

    m_ptrayIcon = new QSystemTrayIcon(this);
    m_pSettingsDialog = new SettingsDialog(this);
    QObject::connect(m_pSettingsDialog, SIGNAL(settingsAccepted()), this, SLOT(slotSettingsDialogAccepted()));

    m_ptrayMenu = new QMenu(this);

    m_pactExit = new QAction("Exit", this);
    m_pactSettings = new QAction("Settings", this);
    m_pAbout = new QAction("About", this);
    QObject::connect(m_pactExit, SIGNAL(triggered()), qApp, SLOT(quit()));
    QObject::connect(m_pactSettings, SIGNAL(triggered()), this, SLOT(slotSettingsDialog()));
    QObject::connect(m_pAbout, SIGNAL(triggered()), this, SLOT(slotAbout()));


    m_ptrayIcon->setContextMenu(m_ptrayMenu);
    m_ptrayIcon->setIcon(QIcon(":/icons/icon.png"));
    m_ptrayIcon->show();

    m_pdevWatcher = new DeviceWatcher(this);
    QObject::connect(m_pdevWatcher, SIGNAL(deviceAdded(DeviceInfo)), this, SLOT(slotDeviceAdded(DeviceInfo)));
    QObject::connect(m_pdevWatcher, SIGNAL(deviceRemoved(DeviceInfo)), this, SLOT(slotDeviceRemoved(DeviceInfo)));
    QObject::connect(m_pdevWatcher, SIGNAL(deviceChanged(DeviceInfo)), this, SLOT(slotDeviceChanged(DeviceInfo)));
    QObject::connect(m_pdevWatcher, SIGNAL(deviceMounted(DeviceInfo, QString, ErrorCode)),
                     this, SLOT(slotDeviceMounted(DeviceInfo, QString, ErrorCode)));
    QObject::connect(m_pdevWatcher, SIGNAL(deviceUnmounted(DeviceInfo, ErrorCode)),
                     this, SLOT(slotDeviceUnmounted(DeviceInfo, ErrorCode)));

    reloadDevices();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotSettingsDialog()
{
    m_pSettingsDialog->show();
}

void MainWindow::slotSettingsDialogAccepted()
{
    reloadDevices();
}

void MainWindow::slotMountUnmount()
{
    QAction * act = qobject_cast<QAction*> (sender());
    QString dev_path = act->data().toString();

    DeviceWatcher::DeviceInfoPtr dev = m_pdevWatcher->getDevice(dev_path);

    if (0 != dev)
    {
        if (dev->isMounted)
            m_pdevWatcher->unmountDevice(dev_path, m_pSettingsDialog->settings()->value("/Settings/Actions/ForceUnmount").toBool());
        else m_pdevWatcher->mountDevice(dev_path);
    }
    else
        qCritical() << "Unknown device passed.";

}

void MainWindow::slotView()
{
    QAction * act = qobject_cast<QAction*>(sender());
    QString dev_path = act->data().toString();

    DeviceWatcher::DeviceInfoPtr dev = m_pdevWatcher->getDevice(dev_path);

    if (0 != dev)
    {
        QString str = Utils::formatDeviceStr(m_pSettingsDialog->settings()->value("/Settings/Actions/ViewCommand").toString(),
                                                        *dev);
        QProcess::execute(str);
    }
    else
       qCritical() << "Unknown device passed.";
}

void MainWindow::slotDeviceAdded(const DeviceInfo &d)
{
    if (m_pSettingsDialog->settings()->value("/Settings/Notifications/ShowAdded").toBool())
        m_ptrayIcon->showMessage(Utils::getDeviceTypeStr(d) + " connected.", Utils::formatDeviceStr("%n (%f)", d));

    if (m_pSettingsDialog->settings()->value("/Settings/Actions/MountAdded").toBool())
        m_pdevWatcher->mountDevice(d.udisksPath);

    reloadDevices();
}

void MainWindow::slotDeviceRemoved(const DeviceInfo &d)
{
    if (m_pSettingsDialog->settings()->value("/Settings/Notifications/ShowRemoved").toBool())
        m_ptrayIcon->showMessage(Utils::getDeviceTypeStr(d) + " disconnected",  Utils::formatDeviceStr("%n (%f)", d));
    reloadDevices();
}

void MainWindow::slotDeviceChanged(const DeviceInfo &d)
{
    reloadDevices();
}

void MainWindow::slotDeviceMounted(const DeviceInfo &d, QString mount_path, ErrorCode err_code)
{
    if (OK != err_code)
    {
        qDebug() << "Mounting error! (" << d.udisksPath << ") " << Utils::mapErrorText(err_code);
        QMessageBox::critical(this, Utils::getDeviceTypeStr(d) + " mount error.",
                              "Device can't be mounted. " + Utils::mapErrorText(err_code),
                              QMessageBox::Ok);
        return;
    }

    qDebug() << d.udisksPath << " mounted to " << mount_path;

    if (m_pSettingsDialog->settings()->value("/Settings/Notifications/ShowMounted").toBool())
        m_ptrayIcon->showMessage(Utils::getDeviceTypeStr(d) + " mounted",
                                 Utils::formatDeviceStr("%n (%f) mounted to %m", d));

    if (m_pSettingsDialog->settings()->value("/Settings/Actions/ExecuteViewMounted").toBool())
    {
        QString command = Utils::formatDeviceStr(m_pSettingsDialog->settings()->value("/Settings/Actions/ViewCommand").toString(),
                                                 d);
        QProcess::execute(command);
    }

    reloadDevices();
}

void MainWindow::slotDeviceUnmounted(const DeviceInfo &d, ErrorCode err_code)
{
    if (OK == err_code)
    {
       if (m_pSettingsDialog->settings()->value("/Settings/Notifications/ShowUnmounted").toBool())
        m_ptrayIcon->showMessage(Utils::getDeviceTypeStr(d) + " unmounted",
                                     Utils::formatDeviceStr("%n (%f) unmounted", d));
       reloadDevices();
    }
    else if (Busy == err_code)
    {
        qDebug() << "Device " + d.udisksPath + " busy.";
    }
    else
    {
        qDebug() << "Unmounting error! (" << d.udisksPath << ") " << Utils::mapErrorText(err_code);
        QMessageBox::critical(this, Utils::getDeviceTypeStr(d) + " unmount error.",
                              "Device can't be unmounted. " + Utils::mapErrorText(err_code),
                              QMessageBox::Ok);
        return;
    }

}

void MainWindow::slotAbout()
{
    QMessageBox::about(this, "About MOUNTain 0.1", "Copyright 2015 Vladislav Nickolaev \
                       <br>Use and redistribute only under GNU GPL license terms</br>");

}

void MainWindow::reloadDevices()
{
    if (!m_pdevWatcher->good())
    {
        qCritical() << "ERROR: DBus\' system bus not found. Check thar DBus daemon is correctly installed and running.";
        QApplication::quit();

    }
    else
    {
        m_ptrayMenu->clear();

        bool show_system = m_pSettingsDialog->settings()->value("/Settings/Actions/ShowSystemInternal").toBool();

        foreach (const DeviceWatcher::DeviceInfoPtr& dev, m_pdevWatcher->devices())
        {
           if (!show_system && dev->isSystem)
                continue;

           QString str = Utils::formatDeviceStr(m_pSettingsDialog->settings()->value("/Settings/Actions/DeviceFormatString").toString(),
                                                *dev);

           QMenu * dev_menu = new QMenu(str, m_ptrayMenu);

           QAction * mnt_act = new QAction(this);
           mnt_act->setText( dev->isMounted ? "Unmount" : "Mount" );
           mnt_act->setData(dev->udisksPath);
           QObject::connect(mnt_act, SIGNAL(triggered()), this, SLOT(slotMountUnmount()));
           dev_menu->addAction(mnt_act);

           if (dev->isMounted)
           {
               QAction * view_act = new QAction("View", this);
               view_act->setData(dev->udisksPath);
               QObject::connect(view_act, SIGNAL(triggered()), this, SLOT(slotView()));
               dev_menu->addAction(view_act);
           }

           m_ptrayMenu->addMenu(dev_menu);

        }

        m_ptrayMenu->addSeparator();
        m_ptrayMenu->addAction(m_pactSettings);
        m_ptrayMenu->addSeparator();
        m_ptrayMenu->addAction(m_pAbout);
        m_ptrayMenu->addSeparator();
        m_ptrayMenu->addAction(m_pactExit);

    }
}
