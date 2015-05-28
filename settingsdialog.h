#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <tr1/memory>
#include <QSettings>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    const QSettings *settings();

protected:
    virtual void showEvent(QShowEvent * pe);

private slots:
    void slotSettingsAccepted();
    void slotSettingsRejected();

signals:
    void settingsAccepted();

private:
    Ui::SettingsDialog *ui;
    QSettings * m_pSettings;


    void writeSettings();
    void readSettings();
};

#endif // SETTINGSDIALOG_H
