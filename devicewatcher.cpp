#include "devicewatcher.h"
#include "interfaces/udisksdeviceinterface.h"

const char * UDISKS_SERVICE = "org.freedesktop.UDisks";
const char * UDISKS_PATH = "/org/freedesktop/UDisks";
const char * DEVPATH_PROPERTY = "DevicePath";

DeviceWatcher::DeviceWatcher(QObject *parent) :
    QObject(parent)
{
    m_interface = new UdisksInterface(UDISKS_SERVICE, UDISKS_PATH, QDBusConnection::systemBus(), this);
    m_good = false;

    QObject::connect(m_interface, SIGNAL(DeviceAdded(QDBusObjectPath)), this, SLOT(slotDeviceAdded(QDBusObjectPath)));
    QObject::connect(m_interface, SIGNAL(DeviceRemoved(QDBusObjectPath)), this, SLOT(slotDeviceRemoved(QDBusObjectPath)));
    QObject::connect(m_interface, SIGNAL(DeviceChanged(QDBusObjectPath)), this, SLOT(slotDeviceChanged(QDBusObjectPath)));

    QDBusPendingReply< QList<QDBusObjectPath> > devs = m_interface->EnumerateDevices();
    devs.waitForFinished();

    if (!devs.isValid())
    {
        qWarning() << devs.error();
    }
    else
    {
        m_good = true;
        DeviceInfoPtr device;
        foreach (QDBusObjectPath p, devs.value())
        {
            device = getDeviceInfoByPath(p);

            if (0 != device)
            {
                m_devices.insert(p.path(), device);
                qDebug() << "Storage device detected: " << p.path();
            }
        }

    }
}

bool DeviceWatcher::good() const
{
    return m_good;
}

void DeviceWatcher::mountDevice(const QString& dev_path)
{
    UdisksDeviceInterface device(UDISKS_SERVICE, dev_path, QDBusConnection::systemBus());

    QDBusPendingCall mount_call = device.FilesystemMount("", QStringList());
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(mount_call, this);
    watcher->setProperty(DEVPATH_PROPERTY, dev_path);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(slotDeviceMounted(QDBusPendingCallWatcher*)));
}

void DeviceWatcher::unmountDevice(const QString &dev_path, bool force)
{
    UdisksDeviceInterface device(UDISKS_SERVICE, dev_path, QDBusConnection::systemBus());
    QStringList opts;

    if (force)
        opts << "force";

    QDBusPendingCall umount_call = device.FilesystemUnmount(opts);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(umount_call, this);
    watcher->setProperty(DEVPATH_PROPERTY, dev_path);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(slotDeviceUnmounted(QDBusPendingCallWatcher*)));
}

QList<DeviceWatcher::DeviceInfoPtr> DeviceWatcher::devices() const
{
    return m_devices.values();
}

DeviceWatcher::DeviceInfoPtr DeviceWatcher::getDevice(const QString &path)
{
    DeviceMap::iterator itr = m_devices.find(path);
    if (m_devices.end() == itr)
        return DeviceInfoPtr();
    return *itr;
}

void DeviceWatcher::slotDeviceAdded(const QDBusObjectPath & p)
{
    DeviceInfoPtr dev = getDeviceInfoByPath(p);

    if (0 != dev)
    {
        m_devices.insert(p.path(), dev);
        emit deviceAdded(*dev);
        qDebug() << "Device added: " << p.path();
    }
}

void DeviceWatcher::slotDeviceChanged(const QDBusObjectPath & p)
{
    DeviceInfoPtr dev = getDeviceInfoByPath(p);
    DeviceMap::iterator itr = m_devices.find(p.path());

    if (0 != dev)
    {
        m_devices.insert(p.path(), dev);

        if (m_devices.end() == itr)
            emit deviceAdded(*dev);
        else emit deviceChanged(*dev);
    }
    else
    {
        if (m_devices.end() != itr)
        {
            DeviceInfoPtr d = *itr;
            emit deviceRemoved(*d);
            m_devices.erase(itr);
        }
    }
}

void DeviceWatcher::slotDeviceRemoved(const QDBusObjectPath & p)
{
    DeviceInfoPtr dev = m_devices.value(p.path());

    if (0 != dev)
    {
        m_devices.remove(p.path());
        emit deviceRemoved(*dev);
        qDebug() << "Device removed: " << p.path();
    }
}

void DeviceWatcher::slotDeviceMounted(QDBusPendingCallWatcher *w)
{
    QDBusPendingReply<QString> r = *w;
    QString path = w->property(DEVPATH_PROPERTY).toString();
    const QString& mount_path = r.isValid() ? r.value() : "";

    DeviceMap::iterator itr = m_devices.find(path);
    emit deviceMounted(**itr, mount_path, codeFromError(r.error()));
    w->deleteLater();
}

void DeviceWatcher::slotDeviceUnmounted(QDBusPendingCallWatcher *w)
{
    QDBusPendingReply<> r = *w;
    QString path = w->property(DEVPATH_PROPERTY).toString();
    DeviceMap::iterator itr = m_devices.find(path);
    emit deviceUnmounted(**itr, codeFromError(r.error()));
    w->deleteLater();
}

DeviceWatcher::DeviceInfoPtr DeviceWatcher::getDeviceInfoByPath(const QDBusObjectPath & p)
{
    UdisksDeviceInterface dev_interface(UDISKS_SERVICE, p.path(), QDBusConnection::systemBus());

    DeviceInfoPtr dev;

    if ("filesystem" == dev_interface.idUsage())
    {
        dev = DeviceInfoPtr(new DeviceInfo());
        const QString& dev_label = dev_interface.idLabel();
        const QString& dev_file = dev_interface.deviceFile();

        dev->fileName = dev_file;
        dev->udisksPath = p.path();
        dev->uuid = dev_interface.idUuid();
        dev->name = dev_label.isEmpty() ? dev_file.mid(dev_file.lastIndexOf("/") + 1) : dev_label;
        dev->fileSystem = dev_interface.idType();
        dev->sizeBytes = dev_interface.deviceSize();
        dev->isMounted = dev_interface.deviceIsMounted();
        dev->isSystem = dev_interface.deviceIsSystemInternal();
        if (dev->isMounted) dev->mountPoint = dev_interface.deviceMountPaths().first();
        dev->type = detectDeviceType(dev_interface);
    }
    return dev;


}

DeviceInfo::DeviceType DeviceWatcher::detectDeviceType(const UdisksDeviceInterface& i)
{
    DeviceInfo::DeviceType type;
    if (i.deviceIsSystemInternal())
        type = DeviceInfo::HDD;
    else if (i.deviceIsOpticalDisk())
        type = DeviceInfo::OPTICAL;
    else if (i.driveMediaCompatibility().contains("floppy"))
        type = DeviceInfo::FLOPPY;
    else if (deviceIsUsb(i))
        type = DeviceInfo::USB;
    else type = DeviceInfo::OTHER;

    return type;

}

bool DeviceWatcher::deviceIsUsb(const UdisksDeviceInterface &i)
{
    QStringList l = i.driveMediaCompatibility();

    foreach (const QString& str, l)
    {
        if (str.contains("flash"))
                return true;
    }

    return false;
}

ErrorCode DeviceWatcher::codeFromError(QDBusError error)
{
    ErrorCode err = OK;

    if (error.isValid())
    {
        if (QDBusError::Other == error.type())
        {
            if  ("org.freedesktop.UDisks.Error.NotAuthorized" == error.name())
            {
                err = NotAuthorized;
            }
            else if ("org.freedesktop.UDisks.Error.Busy" == error.name())
            {
                err = Busy;
            }
            else if ("org.freedesktop.UDisks.Error.Failed" == error.name())
            {
                err = Failed;
            }
            else if ("org.freedesktop.UDisks.Error.Cancelled" == error.name())
            {
                err = Cancelled;
            }
            else if ("org.freedesktop.UDisks.Error.FilesystemDriverMissing" == error.name())
            {
                err = UnknownFileSystem;
            }
            else
            {
                err = InvalidRequest;
            }
        }
        else
        {
            err = DBusError;
        }
    }


    return err;
}

