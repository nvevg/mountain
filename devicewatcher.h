#ifndef DEVICEWATCHER_H
#define DEVICEWATCHER_H

#include <QObject>
#include <QtCore>
#include <QtDBus>
#include <tr1/memory>

#include "interfaces/udisksinterface.h"
#include "interfaces/udisksdeviceinterface.h"

enum ErrorCode { DBusError, Busy, Failed, Cancelled, NotAuthorized, InvalidRequest, UnknownFileSystem, OK };

struct DeviceInfo
{
    enum DeviceType
    {
        HDD, USB, FLOPPY, OPTICAL, OTHER
    };

    QString name;
    QString uuid;
    unsigned long long sizeBytes;
    QString fileSystem;
    bool isMounted;
    QString mountPoint;
    bool isSystem;
    QString udisksPath;
    QString fileName;
    DeviceType type;

};


class DeviceWatcher : public QObject
{
    Q_OBJECT
public:
    typedef std::tr1::shared_ptr<DeviceInfo> DeviceInfoPtr;
    typedef QMap<QString, DeviceInfoPtr> DeviceMap;

    explicit DeviceWatcher(QObject *parent = 0);
    bool good() const;
    void mountDevice(const QString& dev_path);
    void unmountDevice(const QString& dev_path, bool force);
    QList<DeviceInfoPtr> devices() const;
    DeviceInfoPtr getDevice(const QString& path);

signals:
    void deviceAdded(const DeviceInfo&);
    void deviceRemoved(const DeviceInfo&);
    void deviceChanged(const DeviceInfo&);
    void deviceMounted(const DeviceInfo& dev, QString mount_path, ErrorCode e);
    void deviceUnmounted(const DeviceInfo& dev, ErrorCode e);
public slots:
    void slotDeviceAdded(const QDBusObjectPath& p);
    void slotDeviceChanged(const QDBusObjectPath& p);
    void slotDeviceRemoved(const QDBusObjectPath& p);
    void slotDeviceMounted(QDBusPendingCallWatcher* w);
    void slotDeviceUnmounted(QDBusPendingCallWatcher* w);
private:
    DeviceMap m_devices;
    UdisksInterface * m_interface;
    bool m_good;

    DeviceInfoPtr getDeviceInfoByPath(const QDBusObjectPath&p);
    DeviceInfo::DeviceType detectDeviceType(const UdisksDeviceInterface &i);
    bool deviceIsUsb(const UdisksDeviceInterface &i);
    ErrorCode codeFromError(QDBusError err);
};

#endif // DEVICEWATCHER_H
