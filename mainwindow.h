#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QApplication>
#include <QMessageBox>

#include "devicewatcher.h"
#include "settingsdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    QSystemTrayIcon * m_ptrayIcon;
    SettingsDialog * m_pSettingsDialog;

    QMenu * m_ptrayMenu;
    DeviceWatcher * m_pdevWatcher;
    QAction * m_pactExit;
    QAction * m_pactSettings;
    QAction * m_pAbout;

    void reloadDevices();

private slots:
    void slotSettingsDialog();
    void slotSettingsDialogAccepted();
    void slotMountUnmount();
    void slotView();
    void slotDeviceAdded(const DeviceInfo& d);
    void slotDeviceRemoved(const DeviceInfo& d);
    void slotDeviceChanged(const DeviceInfo& d);
    void slotDeviceMounted(const DeviceInfo& d, QString mount_path, ErrorCode err_code);
    void slotDeviceUnmounted(const DeviceInfo& d, ErrorCode err_code);
    void slotAbout();

};

#endif // MAINWINDOW_H
